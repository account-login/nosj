// system
#include <stdio.h>
#include <math.h>
// proj
#include "j.h"
#include "j_def.h"


namespace j {

    static void _clear(_Node *ref) {
        ref->type = T_DEL;
        ref->val.clear();
        ref->values.clear();
        ref->keys.clear();
        // ref->key is reserved
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
            if (ref->type == T_DEL) {
                ref->type = T_MAP;              // newly created node
            }
            if (ref->type == T_MAP) {
                MapResult t;
                t.ref = ref;
                ref = t.key(key.c_str()).ref;   // lookup or create
            } else if (ref->type == T_ARR) {
                ArrayResult t;
                t.ref = ref;
                if (key == "-") {
                    ref = t.push_back().ref;
                } else {
                    errno = 0;
                    uint64_t idx = strtoull(key.c_str(), NULL, 10);     // NOTE: accepts non-std index
                    if (errno) {
                        // bad array index
                        return NULL;
                    }
                    ref = ((const ArrayResult)t).at(idx).ref;
                }
            } else {
                // bad node type
                return NULL;
            }
        }   // while (ref && pointer[0])

        return ref;
    }

    static void _set_u64(_Node *ref, uint64_t val) {
        _clear(ref);
        ref->type = T_NUM;
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%llu", (unsigned long long)val);
        ref->val.assign(buf, n);
    }

    static void _set_i64(_Node *ref, int64_t val) {
        _clear(ref);
        ref->type = T_NUM;
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%lld", (signed long long)val);
        ref->val.assign(buf, n);
    }

    static void _set_double(_Node *ref, double val) {
        _clear(ref);
        ref->type = T_NUM;
        int cls = fpclassify(val);
        if (cls == FP_NAN) {
            ref->val = "NaN";
        } else if (cls == FP_INFINITE) {
            ref->val = (val > 0) ? "Infinity" : "-Infinity";
        } else {
            char buf[32];
            int n = snprintf(buf, sizeof(buf), "%.17g", val);
            ref->val.assign(buf, n);
        }
    }

    // NodeResult
    void NodeResult::set_null() {
        if (!ref) {
            return;
        }

        _clear(ref);
        ref->type = T_NULL;
    }
    void NodeResult::set_bool(bool val) {
        if (!ref) {
            return;
        }

        _clear(ref);
        ref->type = val ? T_TRUE : T_FALSE;
    }
    void NodeResult::set_u64(uint64_t val) {
        if (!ref) {
            return;
        }
        _set_u64(ref, val);
    }
    void NodeResult::set_i64(int64_t val) {
        if (!ref) {
            return;
        }
        _set_i64(ref, val);
    }
    void NodeResult::set_double(double val) {
        if (!ref) {
            return;
        }
        _set_double(ref, val);
    }
    void NodeResult::set_str(const std::string &val) {
        if (!ref) {
            return;
        }

        _clear(ref);
        ref->type = T_STR;
        ref->val = val;
    }
    ArrayResult NodeResult::set_arr() {
        if (ref && ref->type != T_ARR) {
            _clear(ref);
            ref->type = T_ARR;
        }
        ArrayResult r;
        r.ref = ref;
        return r;
    }
    MapResult NodeResult::set_map() {
        if (ref && ref->type != T_MAP) {
            _clear(ref);
            ref->type = T_MAP;
        }
        MapResult r;
        r.ref = ref;
        return r;
    }

    // ArrayResult
    NodeResult ArrayResult::at(size_t i) {
        NodeResult r;
        if (ref && i < ref->values.size()) {
            r.ref = &ref->values[i];
        }
        return r;
    }
    NodeResult ArrayResult::push_back() {
        NodeResult r;
        if (ref) {
            ref->values.push_back(_Node());
            r.ref = &ref->values.back();
        }
        return r;
    }
    void ArrayResult::erase(size_t i) {
        if (ref && i < ref->values.size()) {
            ref->values.erase(ref->values.begin() + i);
        }
    }
    ArrayResult ArrayResult::clear() {
        ArrayResult r;
        if (ref) {
            ref->values.clear();
            r.ref = ref;
        }
        return r;
    }

    // MapResult
    NodeResult MapResult::point(const char *pointer) {
        NodeResult r;
        r.ref = _point(ref, pointer);
        return r;
    }
    NodeResult MapResult::key(const char *key) {
        NodeResult r;
        if (!ref) {
            return r;
        }
        std::map<std::string, size_t>::iterator it = ref->keys.find(key);
        if (it == ref->keys.end()) {
            // insert new key
            ref->keys[key] = ref->values.size();
            ref->values.push_back(_Node());
            r.ref = &ref->values.back();
            r.ref->key = key;
        } else {
            r.ref = &ref->values[it->second];
        }
        return r;
    }
    MapIterator MapResult::iter() {
        MapIterator r;
        if (ref) {
            r.ref = ref;
        }
        return r;
    }
    bool MapResult::erase(const char *key) {
        if (!ref) {
            return false;
        }
        std::map<std::string, size_t>::iterator it = ref->keys.find(key);
        if (it != ref->keys.end()) {
            _Node *node = &ref->values[it->second];
            _clear(node);
            node->type = T_DEL;
            ref->keys.erase(it);
            return true;
        } else {
            return false;
        }
    }
    MapResult MapResult::clear() {
        MapResult r;
        if (ref) {
            ref->values.clear();
            ref->keys.clear();
            r.ref = ref;
        }
        return r;
    }

    // Doc
    void Doc::set_null() {
        if (!ref) {
            ref = new _Node();
        }

        _clear(ref);
        ref->type = T_NULL;
    }
    void Doc::set_bool(bool val) {
        if (!ref) {
            ref = new _Node();
        }

        _clear(ref);
        ref->type = val ? T_TRUE : T_FALSE;
    }
    void Doc::set_u64(uint64_t val) {
        if (!ref) {
            ref = new _Node();
        }
        _set_u64(ref, val);
    }
    void Doc::set_i64(int64_t val) {
        if (!ref) {
            ref = new _Node();
        }
        _set_i64(ref, val);
    }
    void Doc::set_double(double val) {
        if (!ref) {
            ref = new _Node();
        }
        _set_double(ref, val);
    }
    void Doc::set_str(const std::string &val) {
        if (!ref) {
            ref = new _Node();
        }

        _clear(ref);
        ref->type = T_STR;
        ref->val = val;
    }
    ArrayResult Doc::set_arr() {
        if (!ref) {
            ref = new _Node();
        }

        if (ref->type != T_ARR) {
            _clear(ref);
            ref->type = T_ARR;
        }
        ArrayResult r;
        r.ref = ref;
        return r;
    }
    MapResult Doc::set_map() {
        if (!ref) {
            ref = new _Node();
        }

        if (ref->type != T_MAP) {
            _clear(ref);
            ref->type = T_MAP;
        }
        MapResult r;
        r.ref = ref;
        return r;
    }
    Doc &Doc::clear() {
        delete this->ref;
        this->ref = NULL;
        return *this;
    }
    _MovingNode Doc::move() {
        _Node *p = this->ref;
        this->ref = NULL;
        return _MovingNode(p);
    }

}   // ::j
