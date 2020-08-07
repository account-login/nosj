#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../submodules/doctest/doctest/doctest.h"

// system
#include <cmath>
// proj
#include "../j/j.h"


using std::fpclassify;


#define STR(...) #__VA_ARGS__


TEST_CASE("writer.doc.create.node") {
    j::Doc doc;
    REQUIRE_FALSE(doc.ok());
    doc.set_u64(123);       // creating node
    CHECK(doc.is_u64());
    CHECK(doc.get_u64(0) == 123);
}

TEST_CASE("writer.invalid.node") {
    j::NodeResult r1;
    REQUIRE_FALSE(r1.ok());
    r1.set_u64(134);        // can not creating node
    CHECK_FALSE(r1.ok());
    r1.set_null();
    CHECK_FALSE(r1.ok());
    r1.set_bool(true);
    CHECK_FALSE(r1.ok());
    r1.set_i64(-23);
    CHECK_FALSE(r1.ok());
    r1.set_double(1.1);
    CHECK_FALSE(r1.ok());
    r1.set_str("asdf");
    CHECK_FALSE(r1.ok());

    j::ArrayResult ra = r1.set_arr();
    CHECK_FALSE(r1.ok());
    CHECK_FALSE(ra.ok());
    j::NodeResult r2 = ra.at(0);
    CHECK_FALSE(r2.ok());
    r2 = ra.push_back();
    CHECK_FALSE(r2.ok());
    ra.erase(0);
    j::ArrayResult ra_2 = ra.clear();
    CHECK_FALSE(ra_2.ok());
    ra.clear();
    CHECK_FALSE(ra.ok());

    j::MapResult rm = r1.set_map();
    CHECK_FALSE(r1.ok());
    CHECK_FALSE(rm.ok());
    j::NodeResult r3 = rm.key("asdf");
    CHECK_FALSE(r3.ok());
    r3 = rm.point("/asdf");
    CHECK_FALSE(r3.ok());
    CHECK_FALSE(rm.erase("asdf"));
    CHECK_FALSE(rm.key("asdf").ok());       // can not read deleted node
    CHECK_FALSE(rm.point("/asdf").ok());    // can not read deleted node
    rm.clear();
    CHECK_FALSE(rm.ok());
}

TEST_CASE("writer.node.ok") {
    j::Dumper d;
    j::Doc doc;
    j::NodeResult n = doc.set_map().key("k");
    REQUIRE(doc.is_map());
    REQUIRE(n.ok());

    n.set_null();
    CHECK(n.is_null());
    n.set_bool(true);
    CHECK(n.get_bool(false));
    n.set_u64(123);
    CHECK(n.get_u64(0) == 123);
    n.set_i64(-123);
    CHECK(n.get_i64(0) == -123);
    n.set_double(-123);
    CHECK(n.get_double(0) == -123);
    n.set_str("asdf");
    CHECK(n.get_str("") == "asdf");

    n.set_arr().push_back().set_u64(12);
    CHECK(STR({"k":[12]}) == d.dump(doc));
    n.set_arr().push_back().set_u64(56);
    n.set_arr().erase(0);
    CHECK(STR({"k":[56]}) == d.dump(doc));
    n.set_arr().clear();
    CHECK(STR({"k":[]}) == d.dump(doc));
    n.set_arr().push_back().set_u64(56);

    n.set_map().key("asdf").set_u64(34);
    CHECK(STR({"k":{"asdf":34}}) == d.dump(doc));
    n.set_map().key("q").set_u64(3);
    CHECK(n.set_map().erase("asdf"));
    CHECK_FALSE(n.set_map().erase("xxxx"));
    CHECK(STR({"k":{"q":3}}) == d.dump(doc));
    n.set_map().clear();
    CHECK(STR({"k":{}}) == d.dump(doc));
}

TEST_CASE("writer.doc") {
    j::Dumper d;
    j::Doc n;

    n.set_null();
    CHECK(n.is_null());
    n.set_bool(true);
    CHECK(n.get_bool(false));
    n.set_u64(123);
    CHECK(n.get_u64(0) == 123);
    n.set_i64(-123);
    CHECK(n.get_i64(0) == -123);
    n.set_double(-123);
    CHECK(n.get_double(0) == -123);
    n.set_str("asdf");
    CHECK(n.get_str("") == "asdf");

    n.set_arr().push_back().set_u64(12);
    CHECK(STR([12]) == d.dump(n));
    n.set_map().key("asdf").set_u64(34);
    CHECK(STR({"asdf":34}) == d.dump(n));

    j::Doc n2(n.move());
    REQUIRE_FALSE(n.ok());
    CHECK(STR({"asdf":34}) == d.dump(n2));

    n.set_null();
    CHECK(n.is_null());
}

TEST_CASE("writer.doc.new") {
    j::Dumper d;
    {
        j::Doc n;
        n.set_null();
        CHECK(n.is_null());
    }
    {
        j::Doc n;
        n.set_bool(true);
        CHECK(n.get_bool(false));
    }
    {
        j::Doc n;
        n.set_u64(123);
        CHECK(n.get_u64(0) == 123);
    }
    {
        j::Doc n;
        n.set_i64(-123);
        CHECK(n.get_i64(0) == -123);
    }
    {
        j::Doc n;
        n.set_double(-123);
        CHECK(n.get_double(0) == -123);
    }
    {
        j::Doc n;
        n.set_str("asdf");
        CHECK(n.get_str("") == "asdf");
    }
    {
        j::Doc n;
        n.set_arr().push_back().set_u64(12);
        CHECK(STR([12]) == d.dump(n));
    }
    {
        j::Doc n;
        n.set_map().key("asdf").set_u64(34);
        CHECK(STR({"asdf":34}) == d.dump(n));
    }
}

TEST_CASE("writer.pointer") {
    j::Doc n;
    j::Dumper d;

    // tmp node
    CHECK(n.set_map().point("/xxx").ok());
    CHECK_FALSE(n.get_map().point("/xxx").ok());    // can not read tmp node
    CHECK(STR({}) == d.dump(n));
    // map
    n.set_map().point("/ki").set_u64(12);
    CHECK(STR({"ki":12}) == d.dump(n));
    n.set_map().point("/km/ki1").set_u64(12);
    n.set_map().point("/km/ki2").set_u64(12);
    CHECK(STR({"ki":12,"km":{"ki1":12,"ki2":12}}) == d.dump(n));
    // array init
    n.set_map().point("/km/a1").set_arr();
    CHECK(STR({"ki":12,"km":{"ki1":12,"ki2":12,"a1":[]}}) == d.dump(n));
    // array append
    n.set_map().point("/km/a1/-").set_u64(45);
    n.set_map().point("/km/a1/-").set_u64(67);
    CHECK(STR({"ki":12,"km":{"ki1":12,"ki2":12,"a1":[45,67]}}) == d.dump(n));
    // array index
    n.set_map().point("/km/a1/0").set_u64(7);
    CHECK(STR({"ki":12,"km":{"ki1":12,"ki2":12,"a1":[7,67]}}) == d.dump(n));
    // escape
    n.set_map().point("/e~0").set_u64(1);
    n.set_map().point("/e~1").set_u64(2);
    CHECK(STR({"ki":12,"km":{"ki1":12,"ki2":12,"a1":[7,67]},"e~":1,"e/":2}) == d.dump(n));

    // bad pointer
    CHECK_FALSE(n.set_map().point("ki").ok());      // not starts with /
    CHECK_FALSE(n.set_map().point("/ki~~").ok());   // bad escape
    CHECK_FALSE(n.set_map().point("/km/a1/9999999999999999999999999").ok());    // bad index
    CHECK_FALSE(n.set_map().point("/km/a1/xxx").ok());      // bad index
}

TEST_CASE("reader.map") {
    j::Doc doc;
    j::Parser p;
    REQUIRE(p.parse(STR({
        "x": "a",
        "l1": {"l2": {"l3": 5}}
    }), doc));
    CHECK_FALSE(doc.get_map().key("yyy").ok());
    CHECK("a" == doc.get_map().key("x").get_str(""));
    CHECK(5 == doc.get_map().key("l1").get_map().key("l2").get_map().key("l3").get_u64(0));
}

TEST_CASE("reader.array") {
    j::Doc doc;
    CHECK(0 == doc.get_arr().size());

    doc.set_arr().push_back().set_u64(1);
    CHECK(1 == doc.get_arr().size());
    doc.set_arr().push_back().set_u64(2);
    CHECK(2 == doc.get_arr().size());

    CHECK(1 == doc.get_arr().at(0).get_u64(0));
    CHECK(2 == doc.get_arr().at(1).get_u64(0));
    CHECK_FALSE(doc.get_arr().at(2).ok());
}

TEST_CASE("reader.map") {
    j::Doc doc;
    CHECK(0 == doc.get_map().size());

    doc.set_map().key("a").set_u64(1);
    CHECK(1 == doc.get_map().size());

    doc.set_map().key("a").set_u64(1);
    CHECK(1 == doc.get_map().size());

    doc.set_map().key("b").set_u64(2);
    CHECK(2 == doc.get_map().size());

    doc.set_map().erase("c");
    CHECK(2 == doc.get_map().size());

    doc.set_map().erase("a");
    CHECK(1 == doc.get_map().size());
    CHECK_FALSE(doc.get_map().key("a").ok());
    CHECK(2 == doc.get_map().key("b").get_u64(0));
}

TEST_CASE("reader.clone") {
    j::Dumper d;

    j::Doc doc;
    doc.set_arr().push_back().set_u64(1);
    CHECK(STR([1]) == d.dump(doc));

    j::Doc cloned(doc.clone());
    CHECK(STR([1]) == d.dump(cloned));
    CHECK(STR([1]) == d.dump(doc));
}

TEST_CASE("reader.pointer") {
    j::Doc doc;
    j::Parser p;
    REQUIRE(p.parse(STR({
        "0a": [],
        "1a": [1],
        "s~": 2,
        "s/": 3,
        "x": "a",
        "": 4,
        "e": {"": {"": 9}},
        "l1": {"l2": {"l3": 5, "": 6}}
    }), doc));
    const j::MapResult n = doc.get_map();
    // invalid pointer
    CHECK_FALSE(n.point("a").ok());
    CHECK_FALSE(n.point("/a~").ok());
    CHECK_FALSE(n.point("/1a/324354354656569999562544").ok());
    CHECK_FALSE(n.point("/1a/xxx").ok());
    CHECK_FALSE(n.point("/1a/-1").ok());
    CHECK_FALSE(n.point("/1a/-").ok());
    // valid
    CHECK(2 == n.point("/s~0").get_u64(0));
    CHECK(3 == n.point("/s~1").get_u64(0));
    CHECK(5 == n.point("/l1/l2/l3").get_u64(0));
    CHECK(1 == n.point("/1a/0").get_u64(0));
    CHECK("a" == n.point("").get_map().key("x").get_str(""));
    CHECK(4 == n.point("/").get_u64(0));                // empty key
    CHECK(6 == n.point("/l1/l2/").get_u64(0));          // empty key
    CHECK(9 == n.point("/e//").get_u64(0));             // empty key
    // not matched
    CHECK_FALSE(n.point("/1a/1").ok());
    CHECK_FALSE(n.point("/0a/0").ok());
    CHECK_FALSE(n.point("/x/x").ok());
}

TEST_CASE("reader.iterator.invalid") {
    j::ConstMapIterator it;
    CHECK_FALSE(it.next());
    CHECK("" == it.key());
    CHECK_FALSE(it.value().ok());
}

TEST_CASE("reader.iterator.loop") {
    j::Doc doc;
    j::MapResult n = doc.set_map();
    const j::MapResult mr = doc.get_map();
    const char *empty_str = j::ConstMapIterator().key().c_str();
    CHECK(empty_str[0] == '\0');

    j::ConstMapIterator it1;
    j::ConstMapIterator it2;
    j::ConstMapIterator it3;

    // empty
    {
        j::ConstMapIterator it = mr.iter();
        CHECK_FALSE(it.next());
        CHECK(empty_str == it.key().c_str());
        CHECK_FALSE(it.value().ok());

        CHECK_FALSE(it.next());     // again
    }
    // one
    {
        n.key("k1").set_u64(1);
        j::ConstMapIterator it = mr.iter();

        CHECK(it.next());
        CHECK("k1" == it.key());
        CHECK(1 == it.value().get_u64(0));
        it1 = it;

        CHECK_FALSE(it.next());
    }
    // two
    {
        n.key("k2").set_u64(2);
        j::ConstMapIterator it = mr.iter();

        CHECK(it.next());
        CHECK("k1" == it.key());
        CHECK(1 == it.value().get_u64(0));

        CHECK(it.next());
        CHECK("k2" == it.key());
        CHECK(2 == it.value().get_u64(0));
        it2 = it;

        CHECK_FALSE(it.next());
    }
    // three
    {
        n.key("k3").set_u64(3);
        j::ConstMapIterator it = mr.iter();

        CHECK(it.next());
        CHECK("k1" == it.key());
        CHECK(1 == it.value().get_u64(0));

        CHECK(it.next());
        CHECK("k2" == it.key());
        CHECK(2 == it.value().get_u64(0));

        CHECK(it.next());
        CHECK("k3" == it.key());
        CHECK(3 == it.value().get_u64(0));
        it3 = it;

        CHECK_FALSE(it.next());

        CHECK(1 == it1.value().get_u64(0));
        CHECK(2 == it2.value().get_u64(0));
        CHECK(3 == it3.value().get_u64(0));
    }
    // erase the 2nd
    {
        n.erase("k2");
        j::ConstMapIterator it = mr.iter();

        CHECK(it.next());
        CHECK("k1" == it.key());
        CHECK(1 == it.value().get_u64(0));

        CHECK(it.next());
        CHECK("k3" == it.key());
        CHECK(3 == it.value().get_u64(0));

        CHECK_FALSE(it.next());

        CHECK(1 == it1.value().get_u64(0));
        CHECK_FALSE(it2.value().ok());
        CHECK(3 == it3.value().get_u64(0));
    }
    // erase the 1st
    {
        n.erase("k1");
        j::ConstMapIterator it = mr.iter();

        CHECK(it.next());
        CHECK("k3" == it.key());
        CHECK(3 == it.value().get_u64(0));

        CHECK_FALSE(it.next());

        CHECK_FALSE(it1.value().ok());
        CHECK_FALSE(it2.value().ok());
        CHECK(3 == it3.value().get_u64(0));
    }
    // erase the last
    {
        n.erase("k3");
        j::ConstMapIterator it = mr.iter();

        CHECK_FALSE(it.next());

        CHECK_FALSE(it1.value().ok());
        CHECK_FALSE(it2.value().ok());
        CHECK_FALSE(it3.value().ok());
    }
}

TEST_CASE("writer.iterator") {
    j::Dumper d;
    j::Doc doc;
    j::MapResult n = doc.set_map();

    n.key("k1").set_u64(1);
    n.key("k2").set_u64(2);
    CHECK(STR({"k1":1,"k2":2}) == d.dump(doc));

    j::MapIterator it = n.iter();
    it.next();
    CHECK("k1" == it.key());
    it.value().set_u64(3);
    CHECK(STR({"k1":3,"k2":2}) == d.dump(doc));
}

TEST_CASE("writer.nan.inf") {
    j::Doc doc;
    j::Dumper d;

    doc.set_double(0.0 / 0.0);
    CHECK(FP_NAN == fpclassify(0.0 / 0.0));
    CHECK("NaN" == d.dump(doc));

    doc.set_double(-(0.0 / 0.0));
    CHECK("NaN" == d.dump(doc));

    doc.set_double(1.0 / 0.0);
    CHECK("Infinity" == d.dump(doc));

    doc.set_double(-1.0 / 0.0);
    CHECK("-Infinity" == d.dump(doc));
}

TEST_CASE("reader.number") {
    j::Parser p;
    j::Doc doc;

    // exact int
    REQUIRE(p.parse(STR(1.0), doc));
    CHECK(doc.is_u64());
    CHECK(doc.is_double());
    CHECK(1 == doc.get_u64(0));
    CHECK(1 == doc.get_double(0));

    // exact int, inexact float
    REQUIRE(p.parse(STR(18014398509481985.0), doc));
    CHECK(18014398509481985 == doc.get_u64(0));
    CHECK(18014398509481984.0 == doc.get_double(0));

    // overflow int
    REQUIRE(p.parse(STR(1e100), doc));
    CHECK_FALSE(doc.is_u64());
    CHECK(1e100 == doc.get_double(0));

    // overflow INT64_MAX
    REQUIRE(p.parse(STR(9223372036854775808), doc));
    CHECK_FALSE(doc.is_i64());
    CHECK((uint64_t)INT64_MAX + 1 == doc.get_u64(0));

    // overflow INT64_MIN
    REQUIRE(p.parse(STR(-9223372036854775809), doc));
    CHECK_FALSE(doc.is_i64());
    CHECK(-9223372036854775809.0 == doc.get_double(0));

    // float underflow
    REQUIRE(p.parse(STR(1e-10000), doc));
    CHECK(doc.is_double());
    CHECK(0 == doc.get_double(1));

    // float underflow
    REQUIRE(p.parse(STR(1.8011670033376514e-308), doc));
    CHECK(doc.is_double());
    CHECK(1.8011670033376514e-308 == doc.get_double(1));

    REQUIRE(p.parse(STR(2.2250738585072014E-308), doc));
    CHECK(doc.is_double());
    CHECK(2.2250738585072014E-308 == doc.get_double(1));

    // not underflow
    double f = nextafter(DBL_MIN, 1);
    char buf[32] = {};
    sprintf(buf, "%.17g", f);
    CAPTURE(buf);
    REQUIRE(p.parse(std::string(buf), doc));
    CHECK(doc.is_double());
    CHECK(f == doc.get_double(1));

    // float overflow to inf
    REQUIRE(p.parse(STR(1e1000), doc));
    CHECK(doc.is_double());
    CHECK(FP_INFINITE == fpclassify(doc.get_double(0)));
    // float overflow to -inf
    REQUIRE(p.parse(STR(-1e1000), doc));
    CHECK(doc.is_double());
    CHECK(FP_INFINITE == fpclassify(doc.get_double(0)));
    CHECK(0 > doc.get_double(0));

    // range
    REQUIRE(p.parse(STR(-1), doc));
    CHECK_FALSE(doc.is_u64());

    // special floats NaN
    REQUIRE(p.parse(STR(NaN), doc));
    CHECK(doc.is_double());
    CHECK(FP_NAN == fpclassify(doc.get_double(0.0)));
    // special floats inf
    REQUIRE(p.parse(STR(Infinity), doc));
    CHECK(doc.is_double());
    CHECK(FP_INFINITE == fpclassify(doc.get_double(0.0)));
    CHECK(0 < doc.get_double(0.0));
    // special floats -inf
    REQUIRE(p.parse(STR(-Infinity), doc));
    CHECK(doc.is_double());
    CHECK(FP_INFINITE == fpclassify(doc.get_double(0.0)));
    CHECK(0 > doc.get_double(0.0));
    // not ok, extra sign
    CHECK_FALSE(p.parse(STR(+Infinity), doc));
    CHECK_FALSE(p.parse(STR(-NaN), doc));

    // limit
    REQUIRE(p.parse(STR(1e1000000), doc));
    CHECK_FALSE(doc.is_u64());

    // valid int
#define CASE(input) \
    REQUIRE(p.parse(STR(input), doc)); \
    CHECK(uint64_t(input) == doc.get_u64(0))

    CASE(1.234e3);
    CASE(12340e-1);
    CASE(0.001234E+6);
    CASE(1234);
    CASE(1234.0000);
    CASE(1e1);
    CASE(1E1);
    CASE(10e-1);
    CASE(1.234000e3);
    CASE(1.234000e4);
    CASE(0);
    CASE(0e0);
    CASE(0.0);
    CASE(0.00);
    CASE(0.00e0);
    CASE(0.00e-1);
    CASE(0.00e-2);
    CASE(0.00e+1);
    CASE(0.00e+3);
    CASE(0.0000000000000000000000000000000000000000000000000000000000003e61);
    CASE(18446744073709551615.0);   // UINT64_MAX
#undef CASE

    // invalid int, valid float
#define CASE(input) \
    REQUIRE(p.parse(STR(input), doc)); \
    CHECK_FALSE(doc.is_u64()); \
    CHECK(input == doc.get_double(-4654.4684))

    CASE(1.1);
    CASE(1e-1);
    CASE(1.0e-1);
    CASE(1.234e1);
    CASE(1.234e2);
    // overflows
    CASE(1e100);
    CASE(104467440737095516150.0);
    CASE(1044674407370955161500e-1);
    CASE(18446744073709551616.0);   // UINT64_MAX + 1
    CASE(184467440737095516160e-1);
#undef CASE

    // int limits
    REQUIRE(p.parse(STR(18446744073709551615.0), doc));
    CHECK(UINT64_MAX == doc.get_u64(0));
    // INT64_MIN
    REQUIRE(p.parse(STR(-9223372036854775808.0), doc));
    CHECK(INT64_MIN == doc.get_i64(0));
    // INT64_MAX
    REQUIRE(p.parse(STR(9223372036854775807.0), doc));
    CHECK(INT64_MAX == doc.get_i64(0));
}
