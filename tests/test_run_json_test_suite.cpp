#include "../submodules/doctest/doctest/doctest.h"

// system
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <unistd.h>
#include <string.h>
#include <vector>
// proj
#include "../j/j.h"


static std::vector<std::string> glob_files(const char *pattern) {
    std::vector<std::string> ans;
    glob_t gset;
    memset(&gset, 0, sizeof(gset));
    assert(0 == glob(pattern, 0, NULL, &gset));
    for (size_t i = 0; i < gset.gl_pathc; ++i) {
        ans.push_back(gset.gl_pathv[i]);
    }
    globfree(&gset);
    return ans;
}

static std::string read_file(const char *path) {
    int fd = open(path, O_RDONLY);
    assert(fd >= 0);
    char buf[1024 * 256];
    ssize_t nread = read(fd, buf, sizeof(buf));
    assert(nread >= 0);
    return std::string(buf, (size_t)nread);
}

TEST_CASE("JSONTestSuite.y") {
    j::Parser p;
    j::Doc doc;
    j::Dumper d;
    std::vector<std::string> y_files = glob_files("./submodules/JSONTestSuite/test_parsing/y_*.json");
    for (size_t i = 0; i < y_files.size(); ++i) {
        std::string input = read_file(y_files[i].c_str());
        // must parse
        CAPTURE(y_files[i]);
        CAPTURE(input);
        REQUIRE(p.parse(input, doc));
        // dump parse dump
        std::string d1 = d.dump(doc);
        CAPTURE(d1);
        REQUIRE(p.parse(d1, doc));
        std::string d2 = d.dump(doc);
        CHECK(d1 == d2);
    }
}

TEST_CASE("JSONTestSuite.n") {
    j::Parser p;
    j::Doc doc;
    std::vector<std::string> n_files = glob_files("./submodules/JSONTestSuite/test_parsing/n_*.json");
    for (size_t i = 0; i < n_files.size(); ++i) {
        std::string input = read_file(n_files[i].c_str());
        CAPTURE(n_files[i]);
        CAPTURE(input);
        CHECK_FALSE(p.parse(input, doc));
    }
}

TEST_CASE("JSONTestSuite.i") {
    j::Parser p;
    j::Doc doc;
    j::Dumper d;
    std::vector<std::string> i_files = glob_files("./submodules/JSONTestSuite/test_parsing/i_*.json");
    for (size_t i = 0; i < i_files.size(); ++i) {
        std::string input = read_file(i_files[i].c_str());
        CAPTURE(i_files[i]);
        CAPTURE(input);
        bool ok = p.parse(input, doc);
        CAPTURE(ok);
        if (ok) {
            // dump parse dump
            std::string d1 = d.dump(doc);
            CAPTURE(d1);
            REQUIRE(p.parse(d1, doc));
            std::string d2 = d.dump(doc);
            CHECK(d1 == d2);
        }
        CHECK(true);
    }
}
