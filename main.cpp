#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <riscv_vector.h>
#include <string.h>
#include <cassert>
#include "common.h"

#define MAX_VL 256

typedef struct {
    int min_count;
    int max_count;
} Repetition;

extern "C" char* strcpy(char* dst, const char* src);
extern "C" bool str_starts_with(char* str, const char* prefix);
// extern "C" bool str_starts_with_rvv(char* str, const char* prefix);
extern "C" void test_illegal_instructions();

static bool isSpecialChar(char c) {
    switch (c) {
    case '.':
    case '^':
    case '$':
    case '?':
    case '*':
    case '+':
    case '(':
    case ')':
    case '[':
    case ']':
    case '|':
    case '\\':
        return true;
    default:
        return false;
    }
}

static bool isQuantifier(char c) {
    return (c == '+' || c == '*' || c == '?');
}

size_t strlen_vec(char *source) {
  size_t vlmax = __riscv_vsetvlmax_e8m8();
  unsigned char *src = (unsigned char *)source;
  long first_set_bit = -1;
  size_t vl;

  for (; first_set_bit < 0; src += vl) {
    vuint8m8_t vec_src = __riscv_vle8ff_v_u8m8(src, &vl, vlmax);
    vbool1_t string_terminate = __riscv_vmseq_vx_u8m8_b1(vec_src, 0, vl);
    first_set_bit = __riscv_vfirst_m_b1(string_terminate, vl);
  }
  src -= vl - first_set_bit;

  return (size_t)((char*)src - source);
}

bool str_starts_with_rvv(char* str, const char* prefix) {
    size_t vlmax = __riscv_vsetvlmax_e8m8();
    unsigned char *src = (unsigned char *)str;
    unsigned char *pre = (unsigned char *)prefix;
    long first_mismatch = -1;
    size_t vl;

    for (; first_mismatch < 0; src += vl, pre += vl) {
        vuint8m8_t vec_src = __riscv_vle8ff_v_u8m8(src, &vl, vlmax);
        vuint8m8_t vec_pre = __riscv_vle8ff_v_u8m8(pre, &vl, vlmax);
        vbool1_t mismatch = __riscv_vmsne_vv_u8m8_b1(vec_src, vec_pre, vl);
        first_mismatch = __riscv_vfirst_m_b1(mismatch, vl);
    }
    src -= vl - first_mismatch;
    pre -= vl - first_mismatch;

    return first_mismatch == std::strlen(prefix);
}

bool str_ends_with_rvv(char* str, const char* suffix) {
    size_t str_len = std::strlen(str);
    size_t suf_len = std::strlen(suffix);
    if (suf_len > str_len) {
        return false;
    }

    const unsigned char* src = 
        reinterpret_cast<const unsigned char*>(str + (str_len - suf_len));
    const unsigned char* suf = 
        reinterpret_cast<const unsigned char*>(suffix);

    size_t processed = 0;
    while (processed < suf_len) {
        size_t remain = suf_len - processed;

        size_t vl = __riscv_vsetvl_e8m8(remain);

        vuint8m8_t vec_src = __riscv_vle8_v_u8m8(src + processed, vl);
        vuint8m8_t vec_suf = __riscv_vle8_v_u8m8(suf + processed, vl);

        vbool1_t mismatch_mask = __riscv_vmsne_vv_u8m8_b1(vec_src, vec_suf, vl);

        long mismatch_index = __riscv_vfirst_m_b1(mismatch_mask, vl);
        if (mismatch_index >= 0) {
            return false;
        }

        processed += vl;
    }

    return true;
}

bool str_contains_rvv(const char* str, const char* sub) {
    size_t str_len = std::strlen(str);
    size_t sub_len = std::strlen(sub);

    if (sub_len == 0) {
        return true;
    }
    if (sub_len > str_len) {
        return false;
    }

    for (size_t i = 0; i <= str_len - sub_len; i++) {
        size_t processed = 0;
        bool mismatchFound = false;

        while (processed < sub_len) {
            size_t remain = sub_len - processed;

            size_t vl = __riscv_vsetvl_e8m8(remain);
            vuint8m8_t vec_str = __riscv_vle8_v_u8m8(
                reinterpret_cast<const uint8_t*>(str + i + processed), vl
            );
            vuint8m8_t vec_sub = __riscv_vle8_v_u8m8(
                reinterpret_cast<const uint8_t*>(sub + processed), vl
            );
            vbool1_t mismatch_mask = __riscv_vmsne_vv_u8m8_b1(vec_str, vec_sub, vl);

            long mismatch_index = __riscv_vfirst_m_b1(mismatch_mask, vl);
            if (mismatch_index >= 0) {
                mismatchFound = true;
                break;
            }
            processed += vl;
        }

        if (!mismatchFound) {
            return true;
        }
    }

    return false;
}

// []+ : one or more characters in the set
// [^]+ : one or more characters not in the set
// []* : zero or more characters
// [^]* : zero or more characters not in the set
// []{x,y} : x to y characters in the set
// [^]{x,y} : x to y characters not in the set
bool fa_pattern_rvv(const char* fa) {
  int min, max;
  // + : min = 1, max = -1
  // * : min = 0, max = -1
  // {x,y} : min = x, max = y

  return false;
}

// decompose regex pattern into FA patterns and sub-string patterns
// sub-string patterns : any characters except special characters
// FA patterns : special characters
// 
// input : ".*foo[^x]barY+"
// ss patterns : {"foo", "bar"}
// fa patterns : {".*", "[^x]", "Y+"}
//
// input : "hello[^a]{3,7}world"
// ss patterns : {"hello", "world"}
// fa patterns : {"[^a]{3,7}"}
//
// input : "hello[^a]{3,7}world[0-9]+"
// ss patterns : {"hello", "world"}
// fa patterns : {"[^a]{3,7}", "[0-9]+"}
//
// input : raphae[^\r\n]{6,}pattie[^\r\n]{5,}bewormed[^\r\n]{10,}reseize[^\r\n]{4,}underestimate
// ss patterns : {"raphae", "pattie", "bewormed", "reseize", "underestimate"}
// fa patterns : {"[^\r\n]{6,}", "[^\r\n]{5,}", "[^\r\n]{10,}", "[^\r\n]{4,}"}

// input : raphae[^\r\n]{6,}pattie[^\r\n]{5,}bewormed[^\r\n]{10,}reseize[^\r\n]{4,}underestimate
// ss patterns : {"raphae", "pattie", "bewormed", "reseize", "underestimate"}
// fa patterns : {"[^\r\n]{6,}", "[^\r\n]{5,}", "[^\r\n]{10,}", "[^\r\n]{4,}"}
std::vector<std::string> decomposeRegex(const std::string &pattern) {
    std::vector<std::string> tokens;
    size_t i = 0;
    const size_t len = pattern.size();

    while (i < len) {
        std::string token;

        if (pattern[i] == '[') {
            size_t startBracket = i;
            i++;
            while (i < len && pattern[i] != ']') {
                i++;
            }
            if (i < len && pattern[i] == ']') {
                i++;
            }
            token = pattern.substr(startBracket, i - startBracket);
        }
        else if (pattern[i] == '.') {
            token = "."; 
            i++;
        }
        else {
            size_t startText = i;
            while (i < len && pattern[i] != '[' && pattern[i] != '.') {
                i++;
            }
            token = pattern.substr(startText, i - startText);
        }

        if (i < len) {
            if (pattern[i] == '{') {
                size_t repStart = i;
                i++; 
                while (i < len && pattern[i] != '}') {
                    i++;
                }
                if (i < len && pattern[i] == '}') {
                    i++;
                }
                token += pattern.substr(repStart, i - repStart);
            }
            else if (pattern[i] == '+' || pattern[i] == '*') {
                token.push_back(pattern[i]);
                i++;
            }
        }

        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

Repetition parse_repetition(const char* rep_str) {
    Repetition rep = {0, 0};

    if (rep_str[0] == '+') {
        rep.min_count = 1;
        rep.max_count = -1;
    } else if (rep_str[0] == '*') {
        rep.min_count = 0;
        rep.max_count = -1;
    } else if (rep_str[0] == '{') {
        int x, y;
        char* endptr;
        x = (int)strtol(rep_str+1, &endptr, 10);
        rep.min_count = x;

        if (*endptr == '}') {
            rep.max_count = x;
        } else if (*endptr == ',') {
            endptr++;
            if (*endptr == '}') {
                rep.max_count = -1;
            } else {
                y = (int)strtol(endptr, &endptr, 10);
                rep.max_count = y;
            }
        }
    }
    return rep;
}

int find_fa_pattern(const char *str, size_t len, int min_len, int max_len, int pattern_type) {
    uint8_t forb1 = 0, forb2 = 0;
    if (pattern_type == 0) {
        forb1 = '\r';
    } else if (pattern_type == 1) {
        forb1 = '\n';
    } else if (pattern_type == 2) {
        forb1 = '\r';
        forb2 = '\n';
    }

    size_t pos = 0;
    int run = 0;
    size_t run_start = 0;

    uint8_t mask_buf[(MAX_VL + 7) / 8];

    while (pos < len) {
        size_t remaining = len - pos;
        size_t vl = __riscv_vsetvl_e8m1(remaining);

        vuint8m1_t vec = __riscv_vle8_v_u8m1((const unsigned char*)(str + pos), vl);

        vbool8_t mask = __riscv_vmsne_vx_u8m1_b8(vec, forb1, vl);
        if (pattern_type == 2) {
            vbool8_t mask2 = __riscv_vmsne_vx_u8m1_b8(vec, forb2, vl);
            mask = __riscv_vmand_mm_b8(mask, mask2, vl);
        }

        __riscv_vsm_v_b8(mask_buf, mask, vl);

        for (size_t j = 0; j < vl; j++) {
            bool allowed = ((mask_buf[j / 8] >> (j % 8)) & 1) != 0;

            if (allowed) {
                if (run == 0) {
                    run_start = pos + j;
                }
                run++;
            } else {
                if (run >= min_len && (max_len == -1 || run <= max_len)) {
                    return run_start;
                }
                run = 0;
            }
        }
        pos += vl;
    }

    if (run >= min_len && (max_len == -1 || run <= max_len)) {
        return run_start;
    }
    return -1;
}

int find_alternative_substrings(const char *main_str, size_t len,
                                const char **sub_strs, const int *sub_lens, size_t num_sub) {
    size_t pos = 0;
    while (pos < len) {
        size_t remaining = len - pos;
        size_t vl = __riscv_vsetvl_e8m1(remaining);

        vuint8m1_t vec = __riscv_vle8_v_u8m1((const unsigned char*)(main_str + pos), vl);

        uint8_t fc = (uint8_t)sub_strs[0][0];
        vbool8_t candidates_mask = __riscv_vmseq_vx_u8m1_b8(vec, fc, vl);
        for (size_t p = 1; p < num_sub; p++) {
            fc = (uint8_t)sub_strs[p][0];
            vbool8_t mask_p = __riscv_vmseq_vx_u8m1_b8(vec, fc, vl);
            candidates_mask = __riscv_vmor_mm_b8(candidates_mask, mask_p, vl);
        }

        uint8_t mask_buf[(MAX_VL + 7) / 8] = {0};
        __riscv_vsm_v_b8(mask_buf, candidates_mask, vl);

        for (size_t j = 0; j < vl; j++) {
            bool candidate = ((mask_buf[j / 8] >> (j % 8)) & 1) != 0;
            if (candidate) {
                size_t candidate_offset = pos + j;
                for (size_t p = 0; p < num_sub; p++) {
                    int pat_len = sub_lens[p];
                    if (candidate_offset + pat_len <= len) {
                        bool match = true;
                        for (int k = 0; k < pat_len; k++) {
                            if (main_str[candidate_offset + k] != sub_strs[p][k]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            return candidate_offset;
                        }
                    }
                }
            }
        }
        pos += vl;
    }
    return -1;
}

// dot_pattern:
// pattern_type:
//    1 -> ".*" (always match)
//    2 -> ".+" (match if len >= 1)
//    0 -> ".{x,y}" (match if x <= len <= y)
int dot_pattern(const char *str, size_t len, int min_len, int max_len, int pattern_type) {
    if (pattern_type == 1) {
        return 0;
    }
    if (pattern_type == 2) {
        if (len >= 1)
            return 0;
        else
            return -1;
    }
    if (pattern_type == 0) {
        if (len < (size_t)min_len)
            return -1;
        if (len > (size_t)max_len)
            return -1;
        return 0;
    }
    return -1;
}

// --- Unit Tests ---

void test_str_starts_with() {
    std::cout << "Running test_str_starts_with...\n";
    char s1[] = "Hello, RISC-V Vector Extension! This is a test string.";
    assert(str_starts_with_rvv(s1, "Hello, RISC-V Vector Extension!") == true);
    assert(str_starts_with_rvv(s1, "Hi") == false);
    std::cout << "test_str_starts_with passed.\n";
}

void test_str_ends_with() {
    std::cout << "Running test_str_ends_with...\n";
    char s1[] = "Hello, RISC-V Vector Extension! This is a test string.";
    assert(str_ends_with_rvv(s1, "string.") == true);
    assert(str_ends_with_rvv(s1, "extension!") == false);
    std::cout << "test_str_ends_with passed.\n";
}

void test_str_contains() {
    std::cout << "Running test_str_contains...\n";
    char s1[] = "Hello, RISC-V Vector Extension! This is a test string.";
    assert(str_contains_rvv(s1, "RISC-V") == true);
    assert(str_contains_rvv(s1, "Missing") == false);
    std::cout << "test_str_contains passed.\n";
}

void test_decomposeRegex() {
    std::cout << "Running test_decomposeRegex...\n";
    const char* pattern = "raphae[^\r\n]{6,}pattie[^\r\n]{5,}bewormed[^\r\n]{10,}reseize[^\r\n]{4,}underestimate";
    std::vector<std::string> tokens = decomposeRegex(pattern);
    assert(tokens[0] == "raphae");
    assert(tokens[1] == "[^\r\n]{6,}");
    assert(tokens[2] == "pattie");
    assert(tokens[3] == "[^\r\n]{5,}");
    assert(tokens[4] == "bewormed");
    assert(tokens[5] == "[^\r\n]{10,}");
    assert(tokens[6] == "reseize");
    assert(tokens[7] == "[^\r\n]{4,}");
    assert(tokens[8] == "underestimate");
    std::cout << "test_decomposeRegex passed.\n";
}

void test_parse_repetition() {
    std::cout << "Running test_parse_repetition...\n";
    Repetition rep0 = parse_repetition("+");
    Repetition rep1 = parse_repetition("*");
    Repetition rep2 = parse_repetition("{3,7}");
    assert(rep0.min_count == 1 && rep0.max_count == -1);
    assert(rep1.min_count == 0 && rep1.max_count == -1);
    assert(rep2.min_count == 3 && rep2.max_count == 7);
    std::cout << "test_parse_repetition passed.\n";
}

void test_find_fa_pattern() {
    std::cout << "Running test_find_fa_pattern...\n";
    const char* main_str = "\n\n\n\n\n\nhello,\nworld\n\n\n\n\n\n\n";
    size_t len = std::strlen(main_str);
    int offset = find_fa_pattern(main_str, len, 3, 7, 1);
    assert(offset == 6);
    std::cout << "test_find_fa_pattern passed.\n";
}

void test_find_alternative_substrings() {
    std::cout << "Running test_find_alternative_substrings...\n";
    const char* main_str2 = "abcxxxdefghijkl";
    size_t len = std::strlen(main_str2);
    const char* sub_strs[] = {"leo", "ijk", "cxx"};
    const int sub_lens[] = {3, 3, 3};
    size_t num_sub = sizeof(sub_strs) / sizeof(sub_strs[0]);
    int offset = find_alternative_substrings(main_str2, len, sub_strs, sub_lens, num_sub);
    assert(offset == 2);
    std::cout << "test_find_alternative_substrings passed.\n";
}

void test_dot_pattern() {
    std::cout << "Running test_dot_pattern...\n";
    const char* main_str3 = "abcd";
    size_t len = std::strlen(main_str3);
    int offset = dot_pattern(main_str3, len, 3, -1, 0);
    assert(offset == 0);
    offset = dot_pattern(main_str3, len, 1, -1, 1);
    assert(offset == 0);
    offset = dot_pattern(main_str3, len, 0, -1, 2);
    assert(offset == 0);
    const char* main_str4 = "ab";
    len = std::strlen(main_str4);
    offset = dot_pattern(main_str4, len, 3, -1, 0);
    assert(offset == -1);
    std::cout << "test_dot_pattern passed.\n";
}

int main() {
    const uint32_t seed = 0xdeadbeef;
    srand(seed);

    int N = rand() % 2000;

    char s0[N];
    gen_string(s0, N);

    size_t golden, actual;

    asm volatile (
        ".insn 4, 0x8200007b\n" // Custom instruction before vector operation
    );

    golden = strlen(s0);

    // asm volatile (
    //     ".insn 4, 0x8000007b\n" // Custom instruction after vector operation
    // );

    // asm volatile (
    //     ".insn 4, 0x8000007b\n" // Custom instruction before vector operation
    // );
    actual = strlen_vec(s0);
    // asm volatile (
    //     ".insn 4, 0x8200007b\n" // Custom instruction after vector operation
    // );

    puts(golden == actual ? "PASS" : "FAIL");

    test_str_starts_with();
    test_str_ends_with();
    test_str_contains();
    test_decomposeRegex();
    test_parse_repetition();
    test_find_fa_pattern();
    test_find_alternative_substrings();
    test_dot_pattern();

    return 0;
}