#include "../submodules/doctest/doctest/doctest.h"

// system
#include <cmath>
// proj
#include "../j/j.h"


#define STR(...) #__VA_ARGS__


using std::isinf;
using std::isnan;


TEST_CASE("parser.coverage.ok") {
    j::Parser p;
    j::Doc doc;
    CHECK(p.parse(STR(0), doc));
    CHECK(p.parse(STR(-0), doc));
    CHECK(p.parse(STR(123), doc));
    CHECK(p.parse(STR(0.2), doc));
    CHECK(p.parse(STR(1.2), doc));
    CHECK(p.parse(STR(1.123), doc));
    CHECK(p.parse(STR(1.3e9), doc));
    CHECK(p.parse(STR(0e1), doc));
    CHECK(p.parse(STR(1e9), doc));
    CHECK(p.parse(STR(1e+1), doc));
    CHECK(p.parse(STR(1E-1), doc));
    CHECK(p.parse(STR(-1), doc));
    CHECK(p.parse(STR(""), doc));
    CHECK(p.parse(STR("asdf"), doc));
    CHECK(p.parse(STR("\u0020"), doc));
    CHECK(p.parse(STR("\b\f\n\r\t\\\/\""), doc));
    CHECK(p.parse(STR([]), doc));
    CHECK(p.parse(STR([[]]), doc));
    CHECK(p.parse(STR([1]), doc));
    CHECK(p.parse(STR(["asdf", 123]), doc));
    CHECK(p.parse(STR({}), doc));
    CHECK(p.parse(STR({"": ""}), doc));
    CHECK(p.parse(STR({"1": 1, "2": 2}), doc));
    CHECK(p.parse(STR(true), doc));
    CHECK(p.parse(STR(false), doc));
    CHECK(p.parse(STR(null), doc));
    CHECK(p.parse("0 ", doc));
}

TEST_CASE("parser.coverage.bad") {
    j::Parser p;
    j::Doc doc;
    CHECK_FALSE(p.parse(STR({"": 1 ""}), doc));
    CHECK_FALSE(p.parse(STR({1: 2}), doc));
    CHECK_FALSE(p.parse(STR({"1" 2}), doc));
    CHECK_FALSE(p.parse(STR([1 2]), doc));
    CHECK_FALSE(p.parse(STR(haha), doc));
    CHECK_FALSE(p.parse("\"\\" STR(u123), doc));
    CHECK_FALSE(p.parse(STR("\ufffg"), doc));
    CHECK_FALSE(p.parse(STR("\?"), doc));
    CHECK_FALSE(p.parse(STR(tru), doc));
    CHECK_FALSE(p.parse(STR(-.1), doc));
    CHECK_FALSE(p.parse(STR(00), doc));
    CHECK_FALSE(p.parse(STR(01), doc));
    CHECK_FALSE(p.parse(STR(1ee), doc));
    CHECK_FALSE(p.parse(STR(1.e), doc));
    CHECK_FALSE(p.parse(STR(-), doc));
    CHECK_FALSE(p.parse(STR(), doc));
    CHECK_FALSE(p.parse("\"", doc));
    CHECK_FALSE(p.parse("\"\\", doc));
    CHECK_FALSE(p.parse("\"\x01", doc));
    CHECK_FALSE(p.parse(std::string(200, '['), doc));
    CHECK_FALSE(p.parse("[][]", doc));
}

TEST_CASE("parser.surrogate.ok") {
    j::Parser p;
    j::Doc doc;
    REQUIRE(p.parse(STR("\ud83d\ude02"), doc));
    CHECK(doc.get_str("") == "\xf0\x9f\x98\x82");
}

TEST_CASE("parser.surrogate.bad") {
    j::Parser p;
    j::Doc doc;
    REQUIRE(p.parse(STR("\ud83d"), doc));
    CHECK(doc.get_str("") == "\xed\xa0\xbd");
    REQUIRE(p.parse(STR("\ud83dxxx"), doc));
    CHECK(doc.get_str("") == "\xed\xa0\xbdxxx");
    REQUIRE(p.parse(STR("\ud83d\u00aa"), doc));
    CHECK(doc.get_str("") == "\xed\xa0\xbd\xc2\xaa");
    REQUIRE(p.parse(STR("\ud83d\\"), doc));
    CHECK(doc.get_str("") == "\xed\xa0\xbd\\");
}

TEST_CASE("parser.map.dup.key") {
    j::Parser p;
    j::Doc doc;
    j::Dumper d;
    REQUIRE(p.parse(STR({"a":1, "a": 2}), doc));
    CHECK(STR({"a":2}) == d.dump(doc));
}

TEST_CASE("parser.utf8") {
    j::Parser p;
    j::Doc doc;
    j::Dumper d;
    std::string input = STR("\u07fa\u07fb\u07fc\u07fd\u07fe\u07ff\u0800\u0801\u0802\u0803\u07fa\u2193\u3b2c\u54c5\u6e5e\u87f7\ua190\ubb29\ud4c2\uee5b\ufffa\ufffb\ufffc\ufffd\ufffe\uffff\ud800\udc00\ud800\udc01\ud800\udc02\ud800\udc03\ufffa\ud86c\udf2d\ud8d9\ude60\ud946\udd93\ud9b3\udcc6\uda1f\udff9\uda8c\udf2c\udaf9\ude5f\udb66\udd92\udbd3\udcc5\udbff\udffa\udbff\udffb\udbff\udffc\udbff\udffd\udbff\udffe\udbff\udfff");
    REQUIRE(p.parse(input, doc));
    std::string expect = "\xdf\xba\xdf\xbb\xdf\xbc\xdf\xbd\xdf\xbe\xdf\xbf\xe0\xa0\x80\xe0\xa0\x81\xe0\xa0\x82\xe0\xa0\x83\xdf\xba\xe2\x86\x93\xe3\xac\xac\xe5\x93\x85\xe6\xb9\x9e\xe8\x9f\xb7\xea\x86\x90\xeb\xac\xa9\xed\x93\x82\xee\xb9\x9b\xef\xbf\xba\xef\xbf\xbb\xef\xbf\xbc\xef\xbf\xbd\xef\xbf\xbe\xef\xbf\xbf\xf0\x90\x80\x80\xf0\x90\x80\x81\xf0\x90\x80\x82\xf0\x90\x80\x83\xef\xbf\xba\xf0\xab\x8c\xad\xf1\x86\x99\xa0\xf1\xa1\xa6\x93\xf1\xbc\xb3\x86\xf2\x97\xbf\xb9\xf2\xb3\x8c\xac\xf3\x8e\x99\x9f\xf3\xa9\xa6\x92\xf4\x84\xb3\x85\xf4\x8f\xbf\xba\xf4\x8f\xbf\xbb\xf4\x8f\xbf\xbc\xf4\x8f\xbf\xbd\xf4\x8f\xbf\xbe\xf4\x8f\xbf\xbf";
    CHECK(expect == doc.get_str(""));
}

TEST_CASE("parser.cleanup") {
    j::Parser p;
    j::Doc doc;
    REQUIRE_FALSE(p.parse(STR([1,2,3,xxx]), doc));
    CHECK(!doc.ok());
}

TEST_CASE("parser.nan.inf") {
    j::Parser p;
    j::Doc doc;
    REQUIRE(p.parse("NaN", doc));
    CHECK(isnan(doc.get_double(0.0)));
    REQUIRE(p.parse("Infinity", doc));
    CHECK(isinf(doc.get_double(0.0)));
    CHECK(doc.get_double(0.0) > 0.0);
    REQUIRE(p.parse("-Infinity", doc));
    CHECK(doc.get_double(0.0) < 0.0);
    CHECK(!doc.is_i64());
    CHECK(!doc.is_u64());
    // bad
    CHECK_FALSE(p.parse("+Infinity", doc));
}

TEST_CASE("parser.num.conversion") {
    j::Parser p;
    j::Doc doc;

    // float not overflow
    REQUIRE(p.parse("1e10", doc));
    CHECK(doc.is_double());
    CHECK(doc.get_double(0.0) == 1e10);
    // u64 not overflow
    CHECK(doc.is_u64());
    CHECK(doc.get_u64(0) == 10000000000);
    CHECK(doc.get_i64(0) == 10000000000);

    // float not overflow
    REQUIRE(p.parse("1e100", doc));
    CHECK(doc.is_double());
    // u64 overflow
    CHECK_FALSE(doc.is_u64());

    // float overflow
    REQUIRE(p.parse("1e1000", doc));
    CHECK(doc.is_number());
    CHECK(doc.is_double());
    CHECK(doc.get_number("") == "1e1000");

    // exact u64
    REQUIRE(p.parse("1.0", doc));
    CHECK(doc.is_u64());
    CHECK(doc.is_i64());
    // inexact u64
    REQUIRE(p.parse("1.1", doc));
    CHECK_FALSE(doc.is_u64());

    // inexact float
    CHECK(3622009729038561421 != (uint64_t)(double)3622009729038561421);
    REQUIRE(p.parse("3622009729038561421", doc));
    CHECK(doc.is_double());
    CHECK(doc.get_double(0.0) == (double)3622009729038561421);
}

// TODO: more cases
