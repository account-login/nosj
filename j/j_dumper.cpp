// proj
#include "j.h"
#include "j_def.h"


namespace j {

    static const char *const k_hex = "0123456789abcdef";

    static void dump_str(const Dumper &opts, const std::string &str, std::string &ans) {
        ans.push_back('"');
        for (size_t i = 0; i < str.size(); ++i) {
            char ch = str[i];
            if (ch == '"' || ch == '\\') {
                ans.push_back('\\');
                ans.push_back(ch);
            } else if ((uint8_t)ch <= 0x1F) {
                ans.append("\\u00", 4);
                ans.push_back(k_hex[(uint8_t)ch >> 4]);
                ans.push_back(k_hex[(uint8_t)ch & 0xF]);
            } else {
                ans.push_back(ch);
            }
        }
        ans.push_back('"');
    }

    static void dump_val(const Dumper &opts, _Node *ref, std::string &ans) {
        assert(ref->type != T_DEL);
        if (ref->type == T_NULL) {
            ans.append("null");
        } else if (ref->type == T_TRUE) {
            ans.append("true");
        } else if (ref->type == T_FALSE) {
            ans.append("false");
        } else if (ref->type == T_NUM) {
            ans.append(ref->val);
        } else if (ref->type == T_STR) {
            dump_str(opts, ref->val, ans);
        } else if (ref->type == T_ARR) {
            ans.push_back('[');
            bool first = true;
            for (size_t i = 0; i < ref->values.size(); ++i) {
                if (ref->values[i].type == T_DEL) {
                    continue;   // caused by usage error
                }
                if (!first) {
                    ans.push_back(',');
                }
                first = false;
                dump_val(opts, &ref->values[i], ans);
            }
            ans.push_back(']');
        } else if (ref->type == T_MAP) {
            ans.push_back('{');
            bool first = true;
            for (size_t i = 0; i < ref->values.size(); ++i) {
                if (ref->values[i].type == T_DEL) {
                    continue;   // caused by deleted map entry
                }
                if (!first) {
                    ans.push_back(',');
                }
                first = false;
                dump_str(opts, ref->values[i].key, ans);
                ans.push_back(':');
                dump_val(opts, &ref->values[i], ans);
            }
            ans.push_back('}');
        } else {
            assert(!"Unreachable");
        }
    }

    // TODO: options
    std::string Dumper::dump(const Doc &doc) const {
        std::string ans;
        if (!doc.ref || doc.ref->type == T_DEL) {
            return ans;
        }
        dump_val(*this, doc.ref, ans);
        return ans;
    }

}   // ::j
