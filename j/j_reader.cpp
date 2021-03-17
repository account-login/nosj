// system
#include <stdlib.h>
// proj
#include "j.h"
#include "j_def.h"


namespace j {

    static bool _push_digits(uint64_t *out, const char *begin, const char *end) {
        uint64_t val = *out;

        for (const char *i = begin; i < end; ++i) {
            assert('0' <= *i && *i <= '9');

            if (val > ~uint64_t(0) / 10) {
                return false;
            }
            val *= 10;
            uint64_t d = *i - '0';
            if (val + d < val) {
                return false;
            }
            val += d;
        }

        *out = val;
        return true;
    }

    static bool _parse_digits(uint64_t *out, const char *begin, const char *end) {
        if (begin >= end) {
            return false;
        }
        for (const char *i = begin; i < end; ++i) {
            if (!('0' <= *i && *i <= '9')) {
                return false;
            }
        }

        return _push_digits(out, begin, end);
    }

    // lossless convertion from a json number to uint64_t
    bool __parse_decimal(const char *input, uint64_t *out) {
        // for NaN, Infinity
        if (!('0' <= input[0] && input[0] <= '9')) {
            return false;
        }

        // [0, whole_end) is the whole number part
        size_t whole_end = 0;
        while (input[whole_end] && input[whole_end] != '.' && input[whole_end] != 'e' && input[whole_end] != 'E') {
            whole_end++;
        }

        // [frac_begin, frac_end) is the fraction number part
        size_t frac_begin = whole_end;
        if (input[frac_begin] == '.') {
            frac_begin++;
        }
        size_t frac_end = frac_begin;
        while (input[frac_end] && input[frac_end] != 'e' && input[frac_end] != 'E') {
            frac_end++;
        }

        // the exp
        uint32_t exp = 0;
        bool exp_neg = false;
        if (input[frac_end] == 'e' || input[frac_end] == 'E') {
            size_t i = frac_end + 1;
            if (input[i] == '-') {
                exp_neg = true;
                i++;
            } else if (input[i] == '+') {
                i++;
            }
            while (input[i]) {
                exp *= 10;
                exp += input[i] - '0';
                i++;
                if (exp > 10000) {
                    return false;       // XXX: arbitary limit
                }
            }
        }

        // the whole number ajusted by exp
        uint64_t val = 0;
        if (exp_neg) {
            // shift left
            size_t end1 = whole_end > exp ? whole_end - exp : 0;
            // check no frac
            for (size_t i = end1; i < frac_end; ++i) {
                if (input[i] == '.') {
                    continue;
                }
                if (input[i] != '0') {
                    return false;
                }
            }
            // whole part
            if (!_push_digits(&val, input, input + end1)) {
                return false;
            }
        } else {
            // shift right
            size_t end2 = frac_begin + exp;
            // check no frac
            for (size_t i = end2; i < frac_end; ++i) {
                if (input[i] != '0') {
                    return false;
                }
            }
            // whole part
            if (!_push_digits(&val, input, input + whole_end)) {
                return false;
            }
            size_t whole_end2 = end2 < frac_end ? end2 : frac_end;
            if (!_push_digits(&val, input + frac_begin, input + whole_end2)) {
                return false;
            }
            for (size_t i = frac_end; i < end2; ++i) {
                if (val > ~uint64_t(0) / 10) {
                    return false;
                }
                val *= 10;
            }
        }

        // ok
        if (out) {
            *out = val;
        }
        return true;
    }

    static bool _parse_u64(const _Node *ref, uint64_t *out) {
        if (!ref || ref->type != T_NUM || ref->val[0] == '-') {
            return false;
        }
        return __parse_decimal(ref->val.c_str(), out);
    }

    static bool _parse_i64(const _Node *ref, int64_t *out) {
        if (!ref || ref->type != T_NUM) {
            return false;
        }
        const char *input = ref->val.c_str();
        bool neg = false;
        if (input[0] == '-') {
            neg = true;
            input++;
        }
        uint64_t abs = 0;
        if (!__parse_decimal(input, &abs)) {
            return false;
        }
        if (!neg && abs > 0x7fffffffffffffffULL) {   // INT64_MAX
            return false;
        }
        if (neg && abs > 0x8000000000000000ULL) {
            return false;
        }
        if (out) {
            *out = int64_t(neg ? -abs : abs);
        }
        return true;
    }

    bool __parse_double(const std::string &val, double *out) {
        double d = 0;
        if (val == "NaN") {
            d = 0.0 / 0.0;
        } else if (val == "Infinity") {
            d = 1.0 / 0.0;
        } else if (val == "-Infinity") {
            d = -1.0 / 0.0;
        } else {
            char *end = NULL;
            d = strtod(val.data(), &end);
            // overflow or underflow is ok
            if (end != val.data() + val.size()) {
                // bad format?
                return false;
            }
        }
        if (out) {
            *out = d;
        }
        return true;
    }

    static bool _parse_double(const _Node *ref, double *out) {
        if (!ref || ref->type != T_NUM) {
            return false;
        }
        return __parse_double(ref->val, out);
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
                ConstMapResult t;
                t.ref = ref;
                ref = t.key(key.c_str()).ref;       // lookup only
            } else if (ref->type == T_ARR) {
                ArrayResult t;
                t.ref = ref;
                uint64_t idx = 0;
                if (!_parse_digits(&idx, key.data(), key.data() + key.size())) {    // NOTE: accepts non-std index
                    // bad array index
                    return NULL;
                }
                ref = t.at(idx).ref;
            } else {
                // bad node type
                return NULL;
            }
        }   // while (ref && pointer[0])

        if (ref && ref->type == T_DEL) {
            ref = NULL;
        }
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
    ConstArrayResult _NodeReader::get_arr() const {
        ConstArrayResult r;
        if (ref && ref->type == T_ARR) {
            r.ref = (_Node *)ref;
        }
        return r;
    }
    bool _NodeReader::is_map() const {
        return this->get_map().ok();
    }
    ConstMapResult _NodeReader::get_map() const {
        ConstMapResult r;
        if (ref && ref->type == T_MAP) {
            r.ref = (_Node *)ref;
        }
        return r;
    }
    _MovingNode _NodeReader::clone() const {
        return _MovingNode(new _Node(*ref));
    }

    // ArrayResult
    size_t _ArrayReader::size() const {
        return ref ? ref->values.size() : 0;
    }
    ConstNodeResult _ArrayReader::at(size_t i) const {
        ConstNodeResult r;
        if (ref && i < ref->values.size()) {
            r.ref = &ref->values[i];
        }
        return r;
    }

    // MapResult
    size_t _MapReader::size() const {
        return ref ? ref->keys.size() : 0;
    }
    ConstNodeResult _MapReader::point(const char *pointer) const {
        ConstNodeResult r;
        r.ref = _point(ref, pointer);
        return r;
    }
    ConstNodeResult _MapReader::key(const char *key) const {
        ConstNodeResult r;
        if (!ref) {
            return r;
        }
        std::map<std::string, size_t>::iterator it = ref->keys.find(key);
        if (it != ref->keys.end()) {
            r.ref = &ref->values[it->second];
        }
        if (r.ref && r.ref->type == T_DEL) {
            r.ref = NULL;
        }
        return r;
    }
    ConstMapIterator _MapReader::iter() const {
        ConstMapIterator r;
        r.ref = ref;
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
    ConstNodeResult ConstMapIterator::value() const {
        ConstNodeResult r;
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
    ConstNodeResult Doc::get_root() const {
        ConstNodeResult r;
        if (ref && ref->type != T_DEL) {
            r.ref = ref;
        }
        return r;
    }

}   // ::j
