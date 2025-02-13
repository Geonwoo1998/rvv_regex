#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <riscv_vector.h>
#include <string.h>
#include "common.h"

#define MAX_VL 256

extern "C" char* strcpy(char* dst, const char* src);
extern "C" bool str_starts_with(char* str, const char* prefix);
// extern "C" bool str_starts_with_rvv(char* str, const char* prefix);
extern "C" void test_illegal_instructions();

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

bool str_contains_rvv(const char* str, const char* sub)
{
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
bool fa_pattern_rvv(const char* fa)
{
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

// input : raphae[^\r\n]{6,}pattie[^\r\n]{5,}bewormed[^\r\n]{10,}reseize[^\r\n]{4,}underestimate
// ss patterns : {"raphae", "pattie", "bewormed", "reseize", "underestimate"}
// fa patterns : {"[^\r\n]{6,}", "[^\r\n]{5,}", "[^\r\n]{10,}", "[^\r\n]{4,}"}
std::vector<std::string> decomposeRegex(const std::string &pattern)
{
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

typedef struct {
    int min_count;
    int max_count;
} Repetition;

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

// int find_fa_pattern(const char *str, size_t len, int min_len, int max_len, int pattern_type) {
//     uint8_t forb1 = 0, forb2 = 0;
//     if (pattern_type == 0) {
//         forb1 = '\r';
//     } else if (pattern_type == 1) {
//         forb1 = '\n';
//     } else if (pattern_type == 2) {
//         forb1 = '\r';
//         forb2 = '\n';
//     }

//     size_t pos = 0;
//     int run = 0;
//     size_t run_start = 0;
//     bool mask_array[MAX_VL];

//     while (pos < len) {
//         size_t remaining = len - pos;
//         size_t vl = __riscv_vsetvl_e8m8(remaining);

//         vuint8m1_t vec = __riscv_vle8_v_u8m1(str + pos, vl);
//         vbool1_t mask = __riscv_vmseq_vx_u8m1_b1(vec, forb1, vl);
//         if (pattern_type == 2) {
//             vbool1_t mask2 = __riscv_vmseq_vx_u8m1_b1(vec, forb2, vl);
//             mask = __riscv_vmand_mm_b1(mask, mask2);
//         }

//         __riscv_vse8_v_b8((uint8_t*)mask_array, mask, vl);

//         for (size_t j = 0; j < vl; j++) {
//             if (mask_array[j]) {
//                 if (run == 0) {
//                     run_start = pos + j;
//                 }
//                 run++;
//             } else {
//                 if (run >= min_len) {
//                     if (max_len == -1 || run <= max_len) {
//                         return run_start;
//                     }
//                 }
//                 run = 0;
//             }
//         }
//         pos += vl;
//     }

//     return -1;
// }

int find_fa_pattern(const char *str, size_t len, int min_len, int max_len, int pattern_type) {
    // 금지 문자 설정
    uint8_t forb1 = 0, forb2 = 0;
    if (pattern_type == 0) {
        forb1 = '\r';
    } else if (pattern_type == 1) {
        forb1 = '\n';
    } else if (pattern_type == 2) {
        forb1 = '\r';
        forb2 = '\n';
    }

    size_t pos = 0;      // 전체 문자열 내 현재 처리 offset
    int run = 0;         // 현재까지 연속된 allowed 문자의 수
    size_t run_start = 0;// 현재 run의 시작 offset

    // 벡터 마스크를 저장할 버퍼:
    // RISC-V 벡터 boolean은 bit-packed 되어 저장되므로, vl개의 lane을 저장하려면 (vl+7)/8 바이트가 필요함.
    uint8_t mask_buf[(MAX_VL + 7) / 8];

    while (pos < len) {
        size_t remaining = len - pos;
        // 남은 요소에 맞춰 벡터 길이 결정 (8비트 단위)
        size_t vl = __riscv_vsetvl_e8m1(remaining);

        // 메모리에서 8비트 데이터 로드 (캐스팅 필요)
        vuint8m1_t vec = __riscv_vle8_v_u8m1((const unsigned char*)(str + pos), vl);

        // "allowed" 여부: 해당 문자가 금지 문자(forb1)와 **다르다면** true
        vbool8_t mask = __riscv_vmsne_vx_u8m1_b8(vec, forb1, vl);
        if (pattern_type == 2) {
            // 두 번째 금지 문자에 대해서도 allowed 여부 검사
            vbool8_t mask2 = __riscv_vmsne_vx_u8m1_b8(vec, forb2, vl);
            mask = __riscv_vmand_mm_b8(mask, mask2, vl);
        }

        // 벡터 마스크를 bit-packed 형태로 저장
        __riscv_vsm_v_b8(mask_buf, mask, vl);

        // 각 lane에 대해 언팩하여 allowed 여부를 확인
        for (size_t j = 0; j < vl; j++) {
            // mask_buf는 비트팩된 상태이므로,
            // j번째 lane의 결과는 mask_buf[j/8]의 (j % 8)번째 비트에 해당함.
            bool allowed = ((mask_buf[j / 8] >> (j % 8)) & 1) != 0;

            if (allowed) {
                // run이 시작되는 지점이면 run_start 기록
                if (run == 0) {
                    run_start = pos + j;
                }
                run++;
            } else {
                // 금지 문자가 나온 경우, 지금까지의 run 길이가 조건에 부합하면 매칭된 것으로 반환
                if (run >= min_len && (max_len == -1 || run <= max_len)) {
                    return run_start;
                }
                run = 0;
            }
        }
        pos += vl;
    }

    // while 종료 후, 남은 run에 대해 최종 체크
    if (run >= min_len && (max_len == -1 || run <= max_len)) {
        return run_start;
    }
    return -1;
}

int find_alternative_substrings(const char *main_str, size_t len,
                                const char **sub_strs, const int *sub_lens, size_t num_sub)
{
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


    char s1[] = "Hello, RISC-V Vector Extension! This is a test string.";

    bool sw = str_starts_with_rvv(s1, "Hello, RISC-V Vector Extension! This is a test");

    std::cout << "Starts with 'Hello, RISC-V Vector Extension! This is a test': " << sw << "\n";

    bool ew = str_ends_with_rvv(s1, "string!");

    std::cout << "Ends with 'string!': " << ew << "\n";

    bool ct = str_contains_rvv(s1, "RISC-V");

    std::cout << "Contains 'RISC-V': " << ct << "\n";

    // const char* pattern = "raphae[^\r\n]{6,}pattie[^\r\n]{5,}bewormed[^\r\n]{10,}reseize[^\r\n]{4,}underestimate";
    const char* pattern = "raphae[^\r\n]{6,}pattie[^\r\n]{5,}bewormed[^\r\n]{10,}reseize[^\r\n]{4,}underestimate";

    std::vector<std::string> tokens = decomposeRegex(pattern);

    int i = 0;
    for (const auto &token : tokens) {
        std::cout << "token # " << i++ << " : " << token << "\n";
    }

    Repetition rep0 = parse_repetition("+");
    Repetition rep1 = parse_repetition("*");
    Repetition rep2 = parse_repetition("{3,7}");

    std::cout << "Repetition + : min = " << rep0.min_count << ", max = " << rep0.max_count << "\n";
    std::cout << "Repetition * : min = " << rep1.min_count << ", max = " << rep1.max_count << "\n";
    std::cout << "Repetition {3,7} : min = " << rep2.min_count << ", max = " << rep2.max_count << "\n";

    const char *main_str = "\n\n\n\n\n\nhello,\nworld\n\n\n\n\n\n\n";
    size_t len = std::strlen(main_str);

    int pattern_type = 1;
    int min_len = 3;
    int max_len = 7;

    int offset = find_fa_pattern(main_str, len, min_len, max_len, pattern_type);

    std::cout << "Found pattern at offset " << offset << "\n";

    const char *main_str2 = "abcxxxdefghijkl";
    len = std::strlen(main_str2);

    const char *sub_strs[] = {"leo", "ijk", "cxx"};
    const int sub_lens[] = {3, 3, 3};
    size_t num_sub = sizeof(sub_strs) / sizeof(sub_strs[0]);

    offset = find_alternative_substrings(main_str2, len, sub_strs, sub_lens, num_sub);

    std::cout << "Found substring at offset " << offset << "\n";

    return 0;
}