#include "../submodules/doctest/doctest/doctest.h"


// system
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <cmath>
#include <vector>
#include <set>
#include <map>
// proj
#include "../j/j_quick.h"


#define STR(...) #__VA_ARGS__


TEST_CASE("parse") {
    j::Doc doc;
    CHECK_FALSE(j::parse("", doc));
    CHECK(j::parse(STR(1), doc));
    CHECK(doc.get_i64(0) == 1);
}

inline uint64_t get_time_nsec() {
    timespec tv = {0, 0};
    clock_gettime(CLOCK_REALTIME, &tv);
    return uint64_t(tv.tv_sec) * 1000000000 + tv.tv_nsec;
}

struct TmpFile {
    explicit TmpFile(std::string content) {
        path = "tmp_test_" + std::to_string(get_time_nsec());
        fd = ::open(path.c_str(), O_WRONLY | O_CREAT, 0666);
        ssize_t nwrite = ::write(fd, content.c_str(), content.size());
        assert(nwrite == (ssize_t)content.size());
    }

    ~TmpFile() {
        ::close(fd);
        ::unlink(path.c_str());
    }

    int fd = -1;
    std::string path;
};

TEST_CASE("parse_file") {
    j::Doc doc;
    TmpFile t1(STR({"a": 1}));
    CHECK(j::parse_file(t1.path.c_str(), doc));
    CHECK(j::get(doc, "/a", 0) == 1);
}

TEST_CASE("extract.scalar") {
    int32_t s32 = 0;
    CHECK(j::extract(STR({"a": 1}), "/a", s32));
    CHECK(s32 == 1);
    CHECK(j::extract(STR({"a": -1}), "/a", s32));
    CHECK(s32 == -1);

    CHECK(j::extract(STR({"a": 2147483647}), "/a", s32));
    CHECK_FALSE(j::extract(STR({"a": 2147483648}), "/a", s32));

    std::string s;
    CHECK_FALSE(j::extract(STR({"a": 123}), "/a", s));
    CHECK(j::extract(STR({"a": "b"}), "/a", s));
    CHECK(s == "b");

    bool b = false;
    CHECK(j::extract(STR({"a": true}), "/a", b));
    CHECK(b);
    CHECK(j::extract(STR({"a": false}), "/a", b));
    CHECK(!b);
}

TEST_CASE("get.empty.pointer") {
    CHECK(j::get(STR(124), "", 0) == 124);
    CHECK(j::get(STR("b"), "", std::string("c")) == "b");
    CHECK(j::get(STR("a"), "", 1) == 1);
}

TEST_CASE("extract.array") {
    std::vector<int32_t> v32;
    CHECK(j::extract(STR({"a": []}), "/a", v32));
    CHECK(v32.empty());

    CHECK(j::extract(STR({"a": [1, 2, -1]}), "/a", v32));
    CHECK(v32 == std::vector<int32_t>{1, 2, -1});

    CHECK_FALSE(j::extract(STR({"a": [1, "b"]}), "/a", v32));
    CHECK_FALSE(j::extract(STR({"a": [1, 2147483648]}), "/a", v32));

    std::vector<std::string> s32;
    CHECK(j::extract(STR({"a": ["", "b"]}), "/a", s32));
    CHECK(s32 == std::vector<std::string>{"", "b"});

    std::vector<double> vf;
    CHECK(j::extract(STR({"a": [1, 2, -1, 0.5]}), "/a", vf));
    CHECK(vf == std::vector<double>{1, 2, -1, 0.5});

    std::set<int32_t> si;
    CHECK(j::extract(STR({"a": [1, 2, -1, 1]}), "/a", si));
    CHECK(si == std::set<int32_t>{1, 2, -1});

    // inf, nan
    CHECK(j::extract(STR({"a": [NaN, -Infinity]}), "/a", vf));
    CHECK(std::isnan(vf.at(0)));
    CHECK(std::isinf(vf.at(1)));
    CHECK(vf.at(1) < 0);
}

TEST_CASE("extract.map.simple") {
    std::map<int32_t, std::string> mis;
    CHECK(j::extract(STR({"a": {}}), "/a", mis));
    CHECK(mis.empty());

    CHECK(j::extract(STR({"a": {"-1": "a", "23": "b"}}), "/a", mis));
    CHECK(mis[-1] == "a");
    CHECK(mis[23] == "b");

    CHECK_FALSE(j::extract(STR({"a": {"2147483648": "a", "23": "b"}}), "/a", mis));
    CHECK_FALSE(j::extract(STR({"a": {"xxx": "a", "23": "b"}}), "/a", mis));

    std::map<double, double> mdd;
    CHECK(j::extract(STR({"a": {"0.5": 0.25, "-Infinity": 0.125}}), "/a", mdd));
    CHECK(mdd.size() == 2);
    CHECK(mdd.at(0.5) == 0.25);
    CHECK(mdd.begin()->second == 0.125);

    // XXX: NaN as key?
}

struct Key {
    uint64_t a = 0;
    std::string s;

    Key() {}
    Key(uint32_t a, std::string s) : a(a), s(s) {}

    bool operator<(const Key &rhs) const {
        if (this->a < rhs.a) {
            return true;
        } else if (this->a > rhs.a) {
            return false;
        }
        return this->s < rhs.s;
    }
};

namespace j {
    template <>
    inline bool extract(ConstNodeResult h, Key &out) {
        ConstNodeResult a = h.get_map().key("a");
        ConstNodeResult s = h.get_map().key("s");
        if (!a.is_u64() || !s.is_str()) {
            return false;
        }
        out.a = a.get_u64(0);
        out.s = s.get_str("");
        return true;
    }
}

TEST_CASE("extract.map.complex") {
    std::map<Key, int32_t> mki;
    CHECK(j::extract(STR({"a": [
        {"key": {"a": 1, "s": "s1"}, "value": -1},
        {"key": {"a": 2, "s": "s2"}, "value": 2}
    ]}), "/a", mki));
    CHECK(mki.size() == 2);
    CHECK(mki[Key(1, "s1")] == -1);
    CHECK(mki[Key(2, "s2")] == 2);
}
