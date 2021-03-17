// system
#include <string.h>
// proj
#include "j.h"
#include "j_def.h"


namespace j {

    struct _ParseError {
        const char *pos;
        std::string err;

        _ParseError(const char *pos, const std::string &err)
            : pos(pos), err(err)
        {}
    };

    static void skip_space(const char *&cur, const char *end) {
        while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r')) {
            cur++;
        }
        if (cur >= end) {
            throw _ParseError(cur, "unexpected eof");
        }
    }

    static void expect_char(const char *&cur, const char *end, char ch, const char *name) {
        if (cur >= end || *cur != ch) {
            throw _ParseError(cur, std::string("expect ") + name);
        }
        cur++;
    }

    static bool maybe_char(const char *&cur, const char *end, char ch) {
        if (cur < end && *cur == ch) {
            cur++;
            return true;
        } else {
            return false;
        }
    }

    static bool maybe_char_sp(const char *&cur, const char *end, char ch) {
        skip_space(cur, end);
        return maybe_char(cur, end, ch);
    }

    static bool maybe_tok(const char *&cur, const char *end, const char *str) {
        if (cur + strlen(str) <= end && 0 == memcmp(cur, str, strlen(str))) {
            cur += strlen(str);
            return true;
        } else {
            return false;
        }
    }

    static const uint8_t k_hex_numbers[256] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x1, 0x2, 0x3, 0x4,
        0x5, 0x6, 0x7, 0x8, 0x9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xA, 0xB,
        0xC, 0xD, 0xE, 0xF, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff
    };

    static uint32_t parse_hex(const char *&cur, const char *end) {
        if (cur + 4 > end) {
            throw _ParseError(cur, "expect 4 hex digits");
        }
        uint32_t d0 = k_hex_numbers[(uint8_t)cur[0]];
        uint32_t d1 = k_hex_numbers[(uint8_t)cur[1]];
        uint32_t d2 = k_hex_numbers[(uint8_t)cur[2]];
        uint32_t d3 = k_hex_numbers[(uint8_t)cur[3]];
        if (d0 == 0xff || d1 == 0xff || d2 == 0xff || d3 == 0xff) {
            throw _ParseError(cur, "not hex digits");
        }
        cur += 4;
        return (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
    }

    static void utf8_encode(uint32_t code, std::string &ans) {
        if (code <= 0x7f) {
            ans.push_back(code);
        } else if (code <= 0x7ff) {
            ans.push_back(0b11000000 | (code >> 6));
            ans.push_back(0b10000000 | (code & 0b00111111));
        } else if (code <= 0xffff) {
            ans.push_back(0b11100000 | (code >> 12));
            ans.push_back(0b10000000 | ((code >> 6) & 0b00111111));
            ans.push_back(0b10000000 | (code & 0b00111111));
        } else if (code <= 0x10ffff) {
            ans.push_back(0b11110000 | (code >> 18));
            ans.push_back(0b10000000 | ((code >> 12) & 0b00111111));
            ans.push_back(0b10000000 | ((code >> 6) & 0b00111111));
            ans.push_back(0b10000000 | (code & 0b00111111));
        } else {
            assert(!"Unreachable");
        }
    }

    static void parse_u(const char *&cur, const char *end, std::string &ans) {
        // decode code point
        uint32_t code = parse_hex(cur, end);
        if (0xd800 <= code && code <= 0xdbff) {
            // UTF-16 surrogate pair
            if (cur + 2 <= end && cur[0] == '\\' && cur[1] == 'u') {
                cur += 2;
                uint32_t lo_code = parse_hex(cur, end);
                if (0xdc00 <= lo_code && lo_code <= 0xdfff) {
                    code = 0x10000 + ((code & 0b1111111111) << 10) + (lo_code & 0b1111111111);
                } else {
                    // bad surrogate pair
                    utf8_encode(code, ans);
                    utf8_encode(lo_code, ans);
                    return;
                }
            }
            // else truncated surrogate pair
        }
        // encode utf-8
        utf8_encode(code, ans);
    }

    static const char k_escape_chars[256] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '/', 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, 0, 0, '\x08', 0, 0, 0,
        '\x0c', 0, 0, 0, 0, 0, 0, 0, '\n', 0, 0, 0, '\r', 0, '\t', 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    static std::string parse_str(const char *&cur, const char *end) {
        std::string ans;
        if (!maybe_char(cur, end, '"')) {
            throw _ParseError(cur, "expect string");
        }

        while (!maybe_char(cur, end, '"')) {
            if (cur >= end) {
                throw _ParseError(cur, "string not terminated");
            }
            if (maybe_char(cur, end, '\\')) {
                if (cur >= end) {
                    throw _ParseError(cur, "expect string escape");
                }
                char ch = *cur;
                cur++;
                // bfnrt "\/
                if (0 != k_escape_chars[(uint8_t)ch]) {
                    ans.push_back(k_escape_chars[(uint8_t)ch]);
                } else if (ch == 'u') {
                    parse_u(cur, end, ans);
                } else {
                    throw _ParseError(cur, "bad string escape");
                }
            } else if ((uint8_t)*cur <= 0x1F) {
                throw _ParseError(cur, "unescaped control char");
            } else {
                ans.push_back(*cur);
                cur++;
            }
        }
        // TODO: validate utf-8
        return ans;
    }

    static void expect_more_digits(
        const char *&cur, const char *end, std::string &ans, const char *err)
    {
        size_t i = ans.size();
        while (cur < end && ('0' <= *cur && *cur <= '9')) {
            ans.push_back(*cur);
            cur++;
        }
        if (ans.size() == i) {
            throw _ParseError(cur, err);
        }
    }

    static std::string verify_number(const char *&cur, const char *end) {
        std::string ans;
        // sign
        if (maybe_char(cur, end, '-')) {
            ans.push_back(cur[-1]);
            // -inf
            if (maybe_tok(cur, end, "Infinity")) {
                ans.append("Infinity");
                return ans;
            }
        }
        // first digit of int
        if (cur >= end || !('0' <= *cur && *cur <= '9')) {
            throw _ParseError(cur, "expected 0123456789");
        }
        ans.push_back(*cur);
        cur++;
        // remain of int
        if (ans[ans.size() - 1] != '0') {
            while (cur < end && ('0' <= *cur && *cur <= '9')) {
                ans.push_back(*cur);
                cur++;
            }
        }
        // frac
        if (maybe_char(cur, end, '.')) {
            ans.push_back(cur[-1]);
            expect_more_digits(cur, end, ans, "expected frac digits");
        }
        // exp
        if (maybe_char(cur, end, 'e') || maybe_char(cur, end, 'E')) {
            ans.push_back(cur[-1]);
            if (maybe_char(cur, end, '+') || maybe_char(cur, end, '-')) {
                ans.push_back(cur[-1]);
            }
            expect_more_digits(cur, end, ans, "expected exp digits");
        }
        // ok
        return ans;
    }

    // TODO: parser options
    static void parse_value(Parser &parser, const char *&cur, const char *end, _Node &node) {
        parser.depth++;
        if (parser.depth > parser.recursion_limit) {
            throw _ParseError(cur, "recursion limit");
        }

        skip_space(cur, end);
        // map
        if (maybe_char(cur, end, '{')) {
            while (!maybe_char_sp(cur, end, '}')) {
                // comma
                if (!node.values.empty()) {
                    expect_char(cur, end, ',', "comma");
                }
                // key
                skip_space(cur, end);
                std::string key = parse_str(cur, end);
                // colon
                skip_space(cur, end);
                expect_char(cur, end, ':', "colon");
                // value
                node.values.push_back(_Node());
                parse_value(parser, cur, end, node.values.back());
                // link
                std::map<std::string, size_t>::iterator it = node.keys.find(key);
                if (it != node.keys.end()) {
                    // remove previous key
                    node.values[it->second] = _Node();
                    it->second = node.values.size() - 1;
                } else {
                    node.keys[key] = node.values.size() - 1;
                }
                node.values.back().key = key;
            }
            node.type = T_MAP;
        }
        // array
        else if (maybe_char(cur, end, '[')) {
            while (!maybe_char_sp(cur, end, ']')) {
                // comma
                if (!node.values.empty()) {
                    expect_char(cur, end, ',', "comma");
                }
                // value
                node.values.push_back(_Node());
                parse_value(parser, cur, end, node.values.back());
            }
            node.type = T_ARR;
        }
        // true
        else if (maybe_tok(cur, end, "true")) {
            node.type = T_TRUE;
        }
        // false
        else if (maybe_tok(cur, end, "false")) {
            node.type = T_FALSE;
        }
        // null
        else if (maybe_tok(cur, end, "null")) {
            node.type = T_NULL;
        }
        // string
        else if (*cur == '"') {
            node.val = parse_str(cur, end);
            node.type = T_STR;
        }
        // number
        else if (('0' <= *cur && *cur <= '9') || *cur == '-') {
            node.val = verify_number(cur, end);
            node.type = T_NUM;
        }
        // nan
        else if (maybe_tok(cur, end, "NaN")) {
            node.type = T_NUM;
            node.val = "NaN";
        }
        // +inf
        else if (maybe_tok(cur, end, "Infinity")) {
            node.type = T_NUM;
            node.val = "Infinity";
        }
        // error
        else {
            throw _ParseError(cur, "not json");
        }

        parser.depth--;
    }

    bool Parser::parse(const char *begin, const char *end, Doc &doc) {
        this->depth = 0;
        this->err.clear();
        this->errpos = 0;
        if (doc.ref) {
            delete doc.ref;
            doc.ref = NULL;
        }

        try {
            doc.ref = new _Node();
            const char *cur = begin;
            parse_value(*this, cur, end, *doc.ref);
            // trailing garbage
            while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r')) {
                cur++;
            }
            if (cur < end) {
                throw _ParseError(cur, "trailing garbage");
            }
        } catch (_ParseError &exc) {
            this->err.swap(exc.err);
            this->errpos = exc.pos - begin;
            delete doc.ref;
            doc.ref = NULL;
            return false;
        }

        return true;
    }

    bool Parser::parse(const char *begin, Doc &doc) {
        return this->parse(begin, begin + strlen(begin), doc);
    }
    bool Parser::parse(const std::string &input, Doc &doc) {
        return this->parse(input.data(), input.data() + input.size(), doc);
    }

}   // ::j
