// system
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// proj
#include "j_quick.h"


namespace j {

    int32_t __read_file(const char *filename, uint8_t **buf, size_t *sz) {
        assert(*buf == NULL);

        int fd = ::open(filename, O_RDONLY);
        if (fd < 0) {
            return -1;
        }

        struct stat fs;
        memset(&fs, 0, sizeof(fs));

        int32_t err = ::fstat(fd, &fs);
        if (err != 0) {
            goto L_RETURN;
        }

        if (fs.st_size > 0) {
            *buf = (uint8_t *)malloc(fs.st_size + 1);           // +1 for eof detection
            if (!*buf) {
                goto L_RETURN;
            }

            size_t got = 0;
            while (true) {
                size_t todo = (size_t)fs.st_size - got + 1;     // +1 for eof detection
                ssize_t nread = ::read(fd, (void *)(*buf + got), todo);
                if (nread < 0) {
                    err = -1;       // io error
                }
                if (nread <= 0) {
                    break;
                }

                got += nread;
                if (got > (size_t)fs.st_size) {
                    err = -1;       // not eof, file size increased
                    break;
                }
            }   // while true

            // ok
            if (err == 0) {
                *sz = got;
            }
        }

    L_RETURN:
        ::close(fd);
        if (err) {
            ::free(*buf);
        }
        return err;
    }

}   // ::j
