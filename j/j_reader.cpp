// system
#include <stdlib.h>
// proj
#include "j.h"
#include "j_def.h"


namespace j {

    static bool _num_is_double(const _Node *ref) {
        for (size_t i = 0; i < ref->val.size(); ++i) {
            if (ref->val[i] == '.' || ref->val[i] == 'e' || ref->val[i] == 'E') {
                return true;
            }
        }
        return false;
    }

    static bool _parse_u64(const _Node *ref, uint64_t *out) {
        if (!ref || ref->type != T_NUM || ref->val[0] == '-') {
            return false;
        }
        if (_num_is_double(ref)) {
            errno = 0;
            double d = strtod(ref->val.data(), NULL);
            if (errno) {
                // overflow
                return false;
            }
            if ((double)(uint64_t)d != d) {
                // inexact
                return false;
            }
            if (out) {
                *out = (uint64_t)d;
            }
        } else {
            errno = 0;
            uint64_t u = strtoull(ref->val.data(), NULL, 10);
            if (errno) {
                // overflow
                return false;
            }
            if (out) {
                *out = u;
            }
        }
        return true;
    }

    static bool _parse_i64(const _Node *ref, int64_t *out) {
        if (!ref || ref->type != T_NUM) {
            return false;
        }
        if (_num_is_double(ref)) {
            errno = 0;
            double d = strtod(ref->val.data(), NULL);
            if (errno) {
                // overflow
                return false;
            }
            if ((double)(int64_t)d != d) {
                // inexact
                return false;
            }
            if (out) {
                *out = (int64_t)d;
            }
        } else {
            errno = 0;
            int64_t i = strtoll(ref->val.data(), NULL, 10);
            if (errno) {
                // overflow
                return false;
            }
            if (out) {
                *out = i;
            }
        }
        return true;
    }

    static bool _parse_double(const _Node *ref, double *out) {
        if (!ref || ref->type != T_NUM) {
            return false;
        }
        double d = 0;
        if (ref->val == "NaN") {
            d = 0.0 / 0.0;
        } else if (ref->val == "Infinity") {
            d = 1.0 / 0.0;
        } else if (ref->val == "-Infinity") {
            d = -1.0 / 0.0;
        } else {
            errno = 0;
            d = strtod(ref->val.data(), NULL);
            if (errno) {
                // overflow
                return false;
            }
        }
        if (out) {
            *out = d;
        }
        return true;
    }

    static _Node *_point(_Node *ref, const char *pointer) {
        while (ref && pointer[0]) {
            if (pointer[0] != '/') {
                // not a pointer
                return NULL;
            }
            pointer++;
            // decode key
            std::string key;
            while (pointer[0] && pointer[0] != '/') {
                if (pointer[0] == '~') {
                    if (pointer[1] == '0') {
                        key.push_back('~');
                    } else if (pointer[1] == '1') {
                        key.push_back('/');
                    } else {
                        // bad pointer escape
                        return NULL;
                    }
                    pointer += 2;
                } else {
                    key.push_back(pointer[0]);
                    pointer++;
                }
            }
            // access key
            if (ref->type == T_MAP) {
                MapResult t;
                t.ref = ref;
                ref = ((const MapResult)t).key(key.c_str()).ref;    // lookup only
            } else if (ref->type == T_ARR) {
                ArrayResult t;
                t.ref = ref;
                errno = 0;
                uint64_t idx = strtoull(key.c_str(), NULL, 10);     // NOTE: accepts non-std index
                if (errno) {
                    // bad array index
                    return NULL;
                }
                ref = t.at(idx).ref;
            } else {
                // bad node type
                return NULL;
            }
        }   // while (ref && pointer[0])

        return ref;
    }

    // NodeResult
    bool _NodeReader::is_null() const {
        return ref && ref->type == T_NULL;
    }
    bool _NodeReader::is_bool() const {
        return ref && (ref->type == T_TRUE || ref->type == T_FALSE);
    }
    bool _NodeReader::get_bool(bool def) const {
        if (!ref) {
            return def;
        }
        switch (ref->type) {
        case T_TRUE: return true;
        case T_FALSE: return false;
        default: return def;
        }
    }
    bool _NodeReader::is_number() const {
        return ref && ref->type == T_NUM;
    }
    const std::string &_NodeReader::get_number(const std::string &def) const {
        if (ref && ref->type == T_NUM) {
            return ref->val;
        } else {
            return def;
        }
    }
    bool _NodeReader::is_u64() const {
        return _parse_u64(ref, NULL);
    }
    uint64_t _NodeReader::get_u64(uint64_t def) const {
        (void)_parse_u64(ref, &def);
        return def;
    }
    bool _NodeReader::is_i64() const {
        return _parse_i64(ref, NULL);
    }
    int64_t _NodeReader::get_i64(int64_t def) const {
        (void)_parse_i64(ref, &def);
        return def;
    }
    bool _NodeReader::is_double() const {
        return _parse_double(ref, NULL);
    }
    double _NodeReader::get_double(double def) const {
        (void)_parse_double(ref, &def);
        return def;
    }
    bool _NodeReader::is_str() const {
        return ref && ref->type == T_STR;
    }
    const std::string &_NodeReader::get_str(const std::string &def) const {
        return (ref && ref->type == T_STR) ? ref->val : def;
    }
    bool _NodeReader::is_arr() const {
        return this->get_arr().ok();
    }
    const ArrayResult _NodeReader::get_arr() const {
        ArrayResult r;
        if (ref && ref->type == T_ARR) {
            r.ref = (_Node *)ref;
        }
        return r;
    }
    bool _NodeReader::is_map() const {
        return this->get_map().ok();
    }
    const MapResult _NodeReader::get_map() const {
        MapResult r;
        if (ref && ref->type == T_MAP) {
            r.ref = (_Node *)ref;
        }
        return r;
    }
    _MovingNode _NodeReader::clone() const {
        return _MovingNode(new _Node(*ref));
    }

    // ArrayResult
    size_t ArrayResult::size() const {
        return ref ? ref->values.size() : 0;
    }
    const NodeResult ArrayResult::at(size_t i) const {
        NodeResult r;
        if (ref && i < ref->values.size()) {
            r.ref = &ref->values[i];
        }
        return r;
    }

    // MapResult
    size_t MapResult::size() const {
        return ref ? ref->keys.size() : 0;
    }
    const NodeResult MapResult::point(const char *pointer) const {
        NodeResult r;
        r.ref = _point(ref, pointer);
        return r;
    }
    const NodeResult MapResult::key(const char *key) const {
        NodeResult r;
        if (!ref) {
            return r;
        }
        std::map<std::string, size_t>::iterator it = ref->keys.find(key);
        if (it != ref->keys.end()) {
            r.ref = &ref->values[it->second];
        }
        return r;
    }
    ConstMapIterator MapResult::iter() const {
        ConstMapIterator r;
        if (ref) {
            r.ref = ref;
        }
        return r;
    }

    static const std::string g_empty_str;

    // ConstMapIterator
    bool ConstMapIterator::next() {
        if (!ref) {
            return false;
        }
        // skip deleted nodes
        while (i + 1 < ref->values.size() && ref->values[i + 1].type == T_DEL) {
            i++;
        }
        // advance cursor, note i is initialized to ~0
        if (i + 1 <= ref->values.size()) {
            i++;
        }
        return i < ref->values.size();
    }
    const std::string &ConstMapIterator::key() const {
        if (!ref || i >= ref->values.size() || ref->values[i].type == T_DEL) {
            return g_empty_str;
        }
        return ref->values[i].key;
    }
    const NodeResult ConstMapIterator::value() const {
        NodeResult r;
        if (ref && i < ref->values.size() && ref->values[i].type != T_DEL) {
            r.ref = &ref->values[i];
        }
        return r;
    }

    // Doc
    Doc::Doc(const _MovingNode &move) {
        this->clear();
        this->ref = move.ref;
        move.ref = NULL;
    }
    Doc::~Doc() {
        delete this->ref;
        this->ref = NULL;
    }

}   // ::j
