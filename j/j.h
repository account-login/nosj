#pragma once

// system
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string>


namespace j {

    struct NodeResult;
    struct ConstArrayResult;
    struct ArrayResult;
    struct ConstMapResult;
    struct MapResult;
    struct ConstMapIterator;
    struct MapIterator;

    struct _Node;
    struct _MovingNode;

    struct _NodeReader {
        bool ok() const {
            return !!this->ref;
        }
        // reader
        bool is_null() const;
        bool is_bool() const;
        bool get_bool(bool def) const;
        bool is_number() const;
        const std::string &get_number(const std::string &def) const;
        bool is_u64() const;
        uint64_t get_u64(uint64_t def) const;
        bool is_i64() const;
        int64_t get_i64(int64_t def) const;
        bool is_double() const;
        double get_double(double def) const;
        bool is_str() const;
        const std::string &get_str(const std::string &def) const;
        bool is_arr() const;
        ConstArrayResult get_arr() const;
        bool is_map() const;
        ConstMapResult get_map() const;
        _MovingNode clone() const;

        _NodeReader() : ref(NULL) {}

        // private
        _Node *ref;
    };

    struct ConstNodeResult : _NodeReader {};

    struct NodeResult : _NodeReader {
        // writer
        void set_null();
        void set_bool(bool val);
        void set_u64(uint64_t val);
        void set_i64(int64_t val);
        void set_double(double val);
        void set_str(const std::string &val);
        ArrayResult set_arr();
        MapResult set_map();
    };

    struct _ArrayReader {
        bool ok() const {
            return !!this->ref;
        }
        // reader
        size_t size() const;
        ConstNodeResult at(size_t i) const;

        _ArrayReader() : ref(NULL) {}

        // private
        _Node *ref;
    };

    struct ConstArrayResult : _ArrayReader {};

    struct ArrayResult : _ArrayReader {
        // writer
        NodeResult at(size_t i);
        NodeResult push_back();
        void erase(size_t i);
        ArrayResult clear();
    };

    struct _MapReader {
        bool ok() const {
            return !!this->ref;
        }
        // reader
        size_t size() const;
        ConstNodeResult point(const char *pointer) const;
        ConstNodeResult key(const char *key) const;
        ConstMapIterator iter() const;

        _MapReader() : ref(NULL) {}

        // private
        _Node *ref;
    };

    struct ConstMapResult : _MapReader {};

    // NOTE: the erased key is only marked for deletion, do not insert/erase on the same key frequently
    struct MapResult : _MapReader {
        // writer
        NodeResult point(const char *pointer);
        NodeResult key(const char *key);
        MapIterator iter();
        bool erase(const char *key);
        MapResult clear();
    };

    // NOTE: the insertion order is preserved
    // NOTE: the iterator is valid until MapResult::clear()
    struct ConstMapIterator {
        bool next();
        const std::string &key() const;
        ConstNodeResult value() const;

        ConstMapIterator() : ref(NULL), i(~size_t(0)) {}

        // private
        _Node *ref;
        size_t i;
    };

    struct MapIterator {
        bool next();
        const std::string &key() const;
        NodeResult value() const;

        MapIterator() : ref(NULL), i(~size_t(0)) {}

        // private
        _Node *ref;
        size_t i;
    };

    struct _MovingNode {
        explicit _MovingNode(_Node *ref)
            : ref(ref)
        {}
        _MovingNode(const _MovingNode &other)
            : ref(other.ref)
        {
            other.ref = NULL;
        }
        ~_MovingNode() {
            assert(this->ref == NULL);
        }

        // private
        mutable _Node *ref;
    private:
        void operator=(const _MovingNode &);
    };

    // NOTE: the reader/writer method on Doc is not necessary,
    // NOTE: use the get_root()/set_root() method instead.
    struct Doc : _NodeReader {
        Doc() {}
        /* implicit */
        Doc(const _MovingNode &move);
        ~Doc();
        // reader
        ConstNodeResult get_root() const;
        // writer
        NodeResult set_root();
        void set_null() {
            return set_root().set_null();
        }
        void set_bool(bool val) {
            return set_root().set_bool(val);
        }
        void set_u64(uint64_t val) {
            return set_root().set_u64(val);
        }
        void set_i64(int64_t val) {
            return set_root().set_i64(val);
        }
        void set_double(double val) {
            return set_root().set_double(val);
        }
        void set_str(const std::string &val) {
            return set_root().set_str(val);
        }
        ArrayResult set_arr() {
            return set_root().set_arr();
        }
        MapResult set_map() {
            return set_root().set_map();
        }
        Doc &clear();
        _MovingNode move();

    private:
        Doc(const Doc &);
        Doc &operator=(const Doc &);
    };

    struct Parser {
        // options
        // bool disallow_nan = false;
        // bool allow_comment = false;
        // bool allow_extra_comma = false;
        // bool validate_string = false;
        uint32_t recursion_limit;
        // methods
        bool parse(const char *begin, const char *end, Doc &doc);
        bool parse(const char *begin, Doc &doc);
        bool parse(const std::string &input, Doc &doc);
        const char *what() const {
            return this->err.c_str();
        }
        size_t where() const {
            return this->errpos;
        }

        Parser()
            : recursion_limit(100)
            , depth(0)
            , errpos(0)
        {}

        // private
        uint32_t depth;
        std::string err;
        size_t errpos;
    };

    struct Dumper {
        // options
        // bool ensure_ascii = false;
        // const char *nan = NULL;
        // const char *positive_inf = NULL;
        // const char *negative_inf = NULL;
        bool spacing;
        uint32_t indent;
        // methods
        std::string dump(const Doc &doc) const;

        Dumper() : spacing(false), indent(0) {}
    };

}   // ::j
