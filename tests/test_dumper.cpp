#include "../submodules/doctest/doctest/doctest.h"

// system
#include <cmath>
// proj
#include "../j/j.h"


#define STR(...) #__VA_ARGS__


using std::fpclassify;


TEST_CASE("dumper.basic") {
    j::Parser p;
    j::Doc doc;
    j::Dumper d;

#define CASE(val) \
    { \
        std::string input = val; \
        REQUIRE(p.parse(input, doc)); \
        std::string out = d.dump(doc); \
        CHECK(input == out); \
    }

    CASE("null");
    CASE("true");
    CASE("false");
    CASE("[]");
    CASE("[1]");
    CASE("[1,2]");
    CASE("{}");
    CASE(STR({"a":"b"}));
    CASE(STR({"a":"b","c":"d"}));
    CASE(STR("\"\\"));
    CASE(STR(18014398509481985.0));     // inexact float
    CASE(STR(1e1000000));               // inexact float

#undef CASE

    // escape control char
    doc.set_str("\x01");
    CHECK(STR("\u0001") == d.dump(doc));
    // T_DEL
    doc.set_map().key("k1").set_u64(1);
    doc.set_map().key("k2").set_u64(2);
    doc.set_map().erase("k1");
    CHECK(STR({"k2":2}) == d.dump(doc));
    // T_DEL
    doc.set_arr().push_back().set_u64(1);
    doc.set_arr().push_back();
    doc.set_arr().push_back().set_u64(3);
    CHECK(STR([1,3]) == d.dump(doc));

    // inexact float
    doc.set_double(18014398509481985.0);
    CHECK(STR(18014398509481984) == d.dump(doc));
}

TEST_CASE("dumper.indent") {
    j::Parser p;
    j::Doc doc;
    j::Doc da;
    j::Dumper d;

    // {}
    REQUIRE(p.parse(STR({}), doc));
    REQUIRE(p.parse(STR([]), da));

    d.spacing = false;
    d.indent = 0;
    CHECK(STR({}) == d.dump(doc));
    CHECK(STR([]) == d.dump(da));

    d.spacing = true;
    d.indent = 0;
    CHECK(STR({}) == d.dump(doc));
    CHECK(STR([]) == d.dump(da));

    d.spacing = true;
    d.indent = 2;
    CHECK(STR({}) == d.dump(doc));
    CHECK(STR([]) == d.dump(da));

    d.spacing = false;
    d.indent = 2;
    CHECK(STR({}) == d.dump(doc));
    CHECK(STR([]) == d.dump(da));

    // {a: 1}
    doc.set_map().key("a").set_u64(1);
    da.set_arr().push_back().set_u64(1);

    d.spacing = true;
    d.indent = 0;
    CHECK("{\"a\": 1}" == d.dump(doc));
    CHECK("[1]" == d.dump(da));

    d.spacing = false;
    d.indent = 0;
    CHECK("{\"a\":1}" == d.dump(doc));
    CHECK("[1]" == d.dump(da));

    d.spacing = false;
    d.indent = 2;
    CHECK("{\n  \"a\":1\n}" == d.dump(doc));
    CHECK("[\n  1\n]" == d.dump(da));

    d.spacing = true;
    d.indent = 2;
    CHECK("{\n  \"a\": 1\n}" == d.dump(doc));
    CHECK("[\n  1\n]" == d.dump(da));

    // {a: 1, b: 2}
    doc.set_map().key("b").set_u64(2);
    da.set_arr().push_back().set_u64(2);

    d.spacing = false;
    d.indent = 0;
    CHECK("{\"a\":1,\"b\":2}" == d.dump(doc));
    CHECK("[1,2]" == d.dump(da));

    d.spacing = true;
    d.indent = 0;
    CHECK("{\"a\": 1, \"b\": 2}" == d.dump(doc));
    CHECK("[1, 2]" == d.dump(da));

    d.spacing = true;
    d.indent = 2;
    CHECK("{\n  \"a\": 1,\n  \"b\": 2\n}" == d.dump(doc));
    CHECK("[\n  1,\n  2\n]" == d.dump(da));

    d.spacing = false;
    d.indent = 2;
    CHECK("{\n  \"a\":1,\n  \"b\":2\n}" == d.dump(doc));
    CHECK("[\n  1,\n  2\n]" == d.dump(da));
}
