#pragma once

// system
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string>


namespace j {

    struct NodeResult;
    struct ArrayResult;
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
        const ArrayResult get_arr() const;
        bool is_map() const;
        const MapResult get_map() const;
        _MovingNode clone() const;

        // private
        _Node *ref = NULL;
    };

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

    struct ArrayResult {
        bool ok() const {
            return !!this->ref;
        }
        // reader
        size_t size() const;
        const NodeResult at(size_t i) const;
        // writer
        NodeResult at(size_t i);
        NodeResult push_back();
        void erase(size_t i);
        ArrayResult clear();

        // private
        _Node *ref = NULL;
    };

    // NOTE: the erased key is only marked for deletion, do not insert/erase on the same key frequently
    struct MapResult {
        bool ok() const {
            return !!this->ref;
        }
        // reader
        size_t size() const;
        const NodeResult point(const char *pointer) const;
        const NodeResult key(const char *key) const;
        ConstMapIterator iter() const;
        // writer
        NodeResult point(const char *pointer);
        NodeResult key(const char *key);
        MapIterator iter();
        bool erase(const char *key);
        MapResult clear();

        // private
        _Node *ref = NULL;
    };

    // NOTE: the insertion order is preserved
    // NOTE: the iterator is valid until MapResult::clear()
    struct ConstMapIterator {
        bool next();
        const std::string &key() const;
        const NodeResult value() const;

        // private
        _Node *ref = NULL;
        size_t i = ~size_t(0);
    };

    struct MapIterator {
        bool next();
        const std::string &key() const;
        NodeResult value() const;

        // private
        _Node *ref = NULL;
        size_t i = ~size_t(0);
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
        mutable _Node *ref = NULL;
    private:
        void operator=(const _MovingNode &);
    };

    struct Doc : _NodeReader {
        Doc() {}
        /* implicit */
        Doc(const _MovingNode &move);
        ~Doc();
        // writer
        void set_null();
        void set_bool(bool val);
        void set_u64(uint64_t val);
        void set_i64(int64_t val);
        void set_double(double val);
        void set_str(const std::string &val);
        ArrayResult set_arr();
        MapResult set_map();
        Doc &clear();
        _MovingNode move();

    private:
        Doc(const Doc &);
        Doc &operator=(const Doc &);
    };

    struct Parser {
        // options
        bool disallow_nan = false;
        bool allow_comment = false;
        bool allow_extra_comma = false;
        bool validate_string = false;
        uint32_t recursion_limit = 100;
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
        // private
        uint32_t depth = 0;
        std::string err;
        size_t errpos = 0;
    };

    struct Dumper {
        // options
        bool disallow_nan = false;
        bool ensure_ascii = false;
        bool spacing = false;
        uint32_t indent = 0;
        // methods
        std::string dump(const Doc &doc) const;
    };

}   // ::j
