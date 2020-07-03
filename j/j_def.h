#pragma once

// system
#include <string>
#include <deque>
#include <map>


namespace j {

    enum {
        T_DEL = 0,
        T_NULL = 1,
        T_TRUE = 2,
        T_FALSE = 3,
        T_NUM = 4,
        T_STR = 5,
        T_ARR = 6,
        T_MAP = 7,
    };

    struct _Node {
        uint32_t type = 0;
        std::string val;
        std::deque<_Node> values;
        std::map<std::string, size_t> keys;
        std::string key;
    };

}   // ::j
