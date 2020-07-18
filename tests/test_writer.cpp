#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../submodules/doctest/doctest/doctest.h"

// proj
#include "../j/j.h"


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
}

TEST_CASE("writer.pointer") {
    j::Doc n;
    j::Dumper d;

    n.set_map().point("/ki").set_u64(12);
    CHECK(STR({"ki":12}) == d.dump(n));
    n.set_map().point("/km/ki1").set_u64(12);
    n.set_map().point("/km/ki2").set_u64(12);
    CHECK(STR({"ki":12,"km":{"ki1":12,"ki2":12}}) == d.dump(n));
    ;
}
