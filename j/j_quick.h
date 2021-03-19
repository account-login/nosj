#pragma once


// proj
#include "j.h"


namespace j {

    // API

    inline bool parse(const std::string &input, Doc &doc);
    bool parse_file(const char *filename, Doc &doc);

    inline std::string dumps(const Doc &doc) {
        return Dumper().dump(doc);
    }

    template <class T>
    inline T get(const Doc &doc, const char *pointer, const T &def);
    template <class T>
    inline T get(const std::string &input, const char *pointer, const T &def);

    template <class T>
    inline bool extract(ConstNodeResult h, T &out);     // extensible
    template <class T>
    inline bool extract(const Doc &doc, const char *pointer, T &out);
    template <class T>
    inline bool extract(const std::string &input, const char *pointer, T &out);

    template <class T>
    inline void set(NodeResult h, const T &val);        // extensible
    template <class T>
    inline void set(Doc &doc, const char *pointer, const T &val);

    // IMPL extract() BEGIN

    // NOTE: incomplete integer types
#define __DEFINE_IS_SCALAR(type) \
    template <> \
    struct __j_is_scalar<type> { \
        static const bool value = true; \
    }

    template <class T>
    struct __j_is_scalar {
        static const bool value = false;
    };

    __DEFINE_IS_SCALAR(bool);
    __DEFINE_IS_SCALAR(uint8_t);
    __DEFINE_IS_SCALAR(int8_t);
    __DEFINE_IS_SCALAR(uint16_t);
    __DEFINE_IS_SCALAR(int16_t);
    __DEFINE_IS_SCALAR(uint32_t);
    __DEFINE_IS_SCALAR(int32_t);
    __DEFINE_IS_SCALAR(uint64_t);
    __DEFINE_IS_SCALAR(int64_t);
    __DEFINE_IS_SCALAR(float);
    __DEFINE_IS_SCALAR(double);
    __DEFINE_IS_SCALAR(const char *);
    __DEFINE_IS_SCALAR(std::string);

#undef __DEFINE_IS_SCALAR

#define __DEFINE_MIN_MAX(type, vmin, vmax) \
    template <> \
    struct __j_min_max<type> { \
        static const type min = (vmin); \
        static const type max = (vmax); \
    }

    template <class T>
    struct __j_min_max {};

    __DEFINE_MIN_MAX(uint8_t, 0, uint8_t(-1));
    __DEFINE_MIN_MAX(uint16_t, 0, uint16_t(-1));
    __DEFINE_MIN_MAX(uint32_t, 0, uint32_t(-1));
    __DEFINE_MIN_MAX(uint64_t, 0, uint64_t(-1));

    __DEFINE_MIN_MAX(int8_t, -128, 127);
    __DEFINE_MIN_MAX(int16_t, -32768, 32767);
    __DEFINE_MIN_MAX(int32_t, -2147483648, 2147483647);
    __DEFINE_MIN_MAX(int64_t, -9223372036854775807 - 1, 9223372036854775807);

#undef __DEFINE_MIN_MAX

    template <class T>
    struct __j_is_map {
        template <class U> static char Test(typename U::mapped_type *);
        template <class U> static long Test(...);
        static const bool value = (sizeof(Test<T>(0)) == sizeof(char));
    };

    template <class T>
    struct __j_is_vec {
        template <class U, U> struct check_type;
        typedef void (T::*push_backer)(const typename T::value_type &);
        template <class U> static char Test(check_type<push_backer, &U::push_back> *);
        template <class U> static long Test(...);
        static const bool value = (sizeof(Test<T>(0)) == sizeof(char));
    };

    template <class T>
    struct __j_is_set {
        template <class A, class B>
        struct is_same {};

        template <class A>
        struct is_same<A, A> {
            struct type {};
        };

        template <class U> static char Test(typename is_same<typename U::key_type, typename U::value_type>::type *);
        template <class U> static long Test(...);
        static const bool value = (sizeof(Test<T>(0)) == sizeof(char));
    };

    // containers
    template <class T, bool is_vec, bool is_map, bool is_set>
    struct __extract_container {
        bool operator()(ConstNodeResult h, T &value);
    };

    // vector
    template <class T>
    struct __extract_container<T, true, false, false> {
        bool operator()(ConstNodeResult h, T &value) {
            value.clear();

            ConstArrayResult arr = h.get_arr();
            if (!arr.ok()) {
                return false;
            }

            value.reserve(arr.size());

            bool ok = true;
            for (size_t i = 0; i < arr.size(); ++i) {
                typename T::value_type item;
                if (!extract(arr.at(i), item)) {
                    ok = false;
                    continue;
                }
                value.push_back(item);
                // TODO: move
            }

            return ok;
        }
    };

    template <class T>
    inline bool __j_from_str(const std::string &input, T &out);

    template <>
    inline bool __j_from_str(const std::string &input, std::string &out) {
        out = input;
        return true;
    }

    // from j_reader.cpp
    bool __parse_decimal(const char *input, uint64_t *out);
    bool __parse_double(const std::string &val, double *out);

    template <>
    inline bool __j_from_str(const std::string &input, uint64_t &out) {
        return __parse_decimal(input.c_str(), &out);
    }

    template <class T>
    inline bool __j_from_str_pos(const std::string &input, T &out) {
        uint64_t v = 0;
        if (!__parse_decimal(input.c_str(), &v)) {
            return false;
        }
        if (v > __j_min_max<T>::max) {
            return false;
        }
        out = v;
        return true;
    }

    template <>
    inline bool __j_from_str(const std::string &input, uint32_t &out) {
        return __j_from_str_pos<uint32_t>(input, out);
    }

    template <>
    inline bool __j_from_str(const std::string &input, uint16_t &out) {
        return __j_from_str_pos<uint16_t>(input, out);
    }

    template <>
    inline bool __j_from_str(const std::string &input, uint8_t &out) {
        return __j_from_str_pos<uint8_t>(input, out);
    }

    template <class T>
    inline bool __j_from_str_neg(const std::string &input, T &out) {
        const char *data = input.c_str();
        bool neg = false;
        if (data[0] == '-') {
            neg = true;
            data++;
        }
        uint64_t v = 0;
        if (!__parse_decimal(data, &v)) {
            return false;
        }

        if ((!neg && v > (uint64_t)__j_min_max<T>::max) || (neg && (int64_t)-v < __j_min_max<T>::min)) {
            return false;
        }

        v = neg ? -v : v;
        out = v;
        return true;
    }

    template <>
    inline bool __j_from_str(const std::string &input, int64_t &out) {
        return __j_from_str_neg<int64_t>(input, out);
    }

    template <>
    inline bool __j_from_str(const std::string &input, int32_t &out) {
        return __j_from_str_neg<int32_t>(input, out);
    }

    template <>
    inline bool __j_from_str(const std::string &input, int16_t &out) {
        return __j_from_str_neg<int16_t>(input, out);
    }

    template <>
    inline bool __j_from_str(const std::string &input, int8_t &out) {
        return __j_from_str_neg<int8_t>(input, out);
    }

    template <>
    inline bool __j_from_str(const std::string &input, double &out) {
        return __parse_double(input, &out);
    }

    // map
    template <class T>
    bool __extract_map_simple(ConstNodeResult h, T &mapping) {
        mapping.clear();

        ConstMapResult src = h.get_map();
        if (!src.ok()) {
            return false;
        }

        bool ok = true;
        ConstMapIterator it = src.iter();
        while (it.next()) {
            const std::string &key = it.key();
            typename T::key_type mapkey;
            if (!__j_from_str(key, mapkey)) {
                ok = false;
                continue;
            }

            if (!extract(it.value(), mapping[mapkey])) {
                ok = false;
                mapping.erase(mapkey);
            }
        }

        return ok;
    }

    template <class T>
    bool __extract_map_complex(ConstNodeResult h, T &mapping) {
        mapping.clear();

        ConstArrayResult arr = h.get_arr();
        if (!arr.ok()) {
            return false;
        }

        bool ok = true;
        for (size_t i = 0; i < arr.size(); ++i) {
            ConstMapResult kv = arr.at(i).get_map();
            ConstNodeResult hkey = kv.key("key");
            ConstNodeResult hval = kv.key("value");
            if (!hkey.ok() || !hval.ok()) {
                ok = false;
                continue;
            }

            typename T::key_type mapkey;
            if (!extract(hkey, mapkey)) {
                ok = false;
                continue;
            }

            typename T::mapped_type mapvalue;
            if (!extract(hval, mapvalue)) {
                ok = false;
                continue;
            }

            mapping.insert(std::make_pair(mapkey, mapvalue));
            // TODO: move
        }
        return ok;
    }

    // map
    template <class T, bool simple>
    struct __extract_map {
        bool operator()(ConstNodeResult h, T &value) {
            return __extract_map_complex(h, value);
        }
    };

    template <class T>
    struct __extract_map<T, true> {
        bool operator()(ConstNodeResult h, T &value) {
            return __extract_map_simple(h, value);
        }
    };

    template <class T>
    struct __extract_container<T, false, true, false> {
        bool operator()(ConstNodeResult h, T &value) {
            return __extract_map<T, __j_is_scalar<typename T::key_type>::value>()(h, value);
        }
    };

    // set
    template <class T>
    struct __extract_container<T, false, false, true> {
        bool operator()(ConstNodeResult h, T &value) {
            value.clear();

            ConstArrayResult arr = h.get_arr();
            if (!arr.ok()) {
                return false;
            }

            bool ok = true;
            for (size_t i = 0; i < arr.size(); ++i) {
                typename T::value_type item;
                if (!extract(arr.at(i), item)) {
                    ok = false;
                    continue;
                }
                value.insert(item);
                // TODO: move
            }

            return ok;
        }
    };

    // containers
    template <class T, bool is_scalar>
    struct __extract_impl {
        bool operator()(ConstNodeResult h, T &container) {
            return __extract_container<
                T,
                __j_is_vec<T>::value,
                __j_is_map<T>::value,
                __j_is_set<T>::value
            >()(h, container);
        }
    };

    // scalar
    template <class T>
    inline bool __extract_scalar(ConstNodeResult h, T &value);

    template <>
    inline bool __extract_scalar(ConstNodeResult h, std::string &value) {
        std::string def;
        const std::string &out = h.get_str(def);
        if (&out == &def) {
            return false;
        }
        value = out;
        return true;
    }

    template <class T>
    inline bool __extract_uint(ConstNodeResult h, T &value) {
        uint64_t out = h.get_u64(42);
        if (out == 42 && !h.is_u64()) {
            return false;
        }
        if (out > T(-1)) {
            return false;
        }
        value = out;
        return true;
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, bool &value) {
        if (!h.is_bool()) {
            return false;
        }
        value = h.get_bool(false);
        return true;
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, uint64_t &value) {
        return __extract_uint<uint64_t>(h, value);
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, uint32_t &value) {
        return __extract_uint<uint32_t>(h, value);
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, uint16_t &value) {
        return __extract_uint<uint16_t>(h, value);
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, uint8_t &value) {
        return __extract_uint<uint8_t>(h, value);
    }

    template <class T>
    inline bool __extract_sint(ConstNodeResult h, T &value) {
        int64_t out = h.get_i64(42);
        if (out == 42 && !h.is_i64()) {
            return false;
        }
        if (out > __j_min_max<T>::max || out < __j_min_max<T>::min) {
            return false;
        }
        value = out;
        return true;
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, int64_t &value) {
        return __extract_sint<int64_t>(h, value);
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, int32_t &value) {
        return __extract_sint<int32_t>(h, value);
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, int16_t &value) {
        return __extract_sint<int16_t>(h, value);
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, int8_t &value) {
        return __extract_sint<int8_t>(h, value);
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, double &value) {
        double out = h.get_double(42);
        if (out == 42 && !h.is_double()) {
            return false;
        }
        value = out;
        return true;
    }

    template <>
    inline bool __extract_scalar(ConstNodeResult h, float &value) {
        double out = h.get_double(42);
        if (out == 42 && !h.is_double()) {
            return false;
        }
        value = out;
        return true;
    }

    // scalar
    template <class T>
    struct __extract_impl<T, true> {
        bool operator()(ConstNodeResult h, T &value) {
            return __extract_scalar(h, value);
        }
    };

    template <class T>
    inline bool extract(ConstNodeResult h, T &out) {
        return __extract_impl<T, __j_is_scalar<T>::value>()(h, out);
    }

    template <class T>
    inline bool extract(const Doc &doc, const char *pointer, T &out) {
        ConstNodeResult r = doc.get_root();
        if (pointer[0]) {
            r = r.get_map().point(pointer);
        }
        return extract(r, out);
    }

    template <class T>
    inline bool extract(const std::string &input, const char *pointer, T &out) {
        Doc doc;
        if (!parse(input, doc)) {
            return false;
        }
        return extract(doc, pointer, out);
    }

    // IMPL extract() END

    // IMPL set() BEGIN

    // containers
    template <class T, bool is_seq, bool is_map>
    struct __set_container {
        void operator()(NodeResult h, const T &val) const;
    };

    // set() seq
    template <class T>
    struct __set_container<T, true, false> {
        void operator()(NodeResult h, const T &val) const {
            ArrayResult r = h.set_arr();
            r.clear();
            for (typename T::const_iterator it = val.begin(); it != val.end(); ++it) {
                set(r.push_back(), *it);
            }
        }
    };

    // set() map
    // TODO: complex map
    template <class T>
    struct __set_container<T, false, true> {
        void operator()(NodeResult h, const T &val) const {
            MapResult r = h.set_map();
            r.clear();
            for (typename T::const_iterator it = val.begin(); it != val.end(); ++it) {
                // TODO: convert key to string
                set(r.key(it->first.c_str()), it->second);
            }
        }
    };

    // scalars
    template <class T>
    inline void __set_scalar(NodeResult h, const T &val);

    template <>
    inline void __set_scalar(NodeResult h, const bool &val) {
        h.set_bool(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const uint64_t &val) {
        h.set_u64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const uint32_t &val) {
        h.set_u64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const uint16_t &val) {
        h.set_u64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const uint8_t &val) {
        h.set_u64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const int64_t &val) {
        h.set_i64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const int32_t &val) {
        h.set_i64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const int16_t &val) {
        h.set_i64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const int8_t &val) {
        h.set_i64(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const float &val) {
        h.set_double(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const double &val) {
        h.set_double(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const char *const &val) {
        h.set_str(val);
    }
    template <>
    inline void __set_scalar(NodeResult h, const std::string &val) {
        h.set_str(val);
    }

    // set() container
    template <class T, bool is_scalar>
    struct __set_impl {
        void operator()(NodeResult h, const T &val) const {
            return __set_container<
                T, __j_is_vec<T>::value || __j_is_set<T>::value, __j_is_map<T>::value
            >()(h, val);
        }
    };

    template <class T>
    struct __set_impl<T, true> {
        void operator()(NodeResult h, const T &val) const {
            return __set_scalar<T>(h, val);
        }
    };

    // https://stackoverflow.com/a/6559891
    // for const char *
    template <class T>
    struct __j_array_to_pointer_decay {
        typedef T type;
    };

    template <class T, std::size_t N>
    struct __j_array_to_pointer_decay<T[N]> {
        typedef const T *type;
    };

    template <class T>
    inline void set(NodeResult h, const T &val) {
        typedef typename __j_array_to_pointer_decay<T>::type T2;
        return __set_impl<T2, __j_is_scalar<T2>::value>()(h, val);
    }

    template <class T>
    inline void set(Doc &doc, const char *pointer, const T &val) {
        NodeResult r = doc.set_root();
        if (pointer[0]) {
            r = r.set_map().point(pointer);
        }
        return set(r, val);
    }

    // IMPL set() END

    template <class T>
    inline T get(const Doc &doc, const char *pointer, const T &def) {
        T ans;
        if (!extract(doc, pointer, ans)) {
            return def;
        }
        return ans;
    }

    template <class T>
    inline T get(const std::string &input, const char *pointer, const T &def) {
        T ans;
        if (!extract(input, pointer, ans)) {
            return def;
        }
        return ans;
    }

    inline bool parse(const std::string &input, Doc &doc) {
        return Parser().parse(input, doc);
    }

}   // ::j
