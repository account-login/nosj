#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../submodules/doctest/doctest/doctest.h"

// system
#include <cmath>
// proj
#include "../j/j.h"


#define STR(...) #__VA_ARGS__


using std::fpclassify;


TEST_CASE("dumper") {
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

#undef CASE

    // escape control cahr
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
}

// NOTE: this is testing the writer actually
TEST_CASE("dumper.nan.inf") {
    j::Doc doc;
    j::Dumper d;

    doc.set_double(0.0 / 0.0);
    CHECK(FP_NAN == fpclassify(0.0 / 0.0));
    CHECK("NaN" == d.dump(doc));

    doc.set_double(1.0 / 0.0);
    CHECK("Infinity" == d.dump(doc));

    doc.set_double(-1.0 / 0.0);
    CHECK("-Infinity" == d.dump(doc));
}
