    .section .text
    .align  2
    .global str_starts_with_rvv
    .type   str_starts_with_rvv, @function

#------------------------------------------------------------------------------
# bool str_starts_with_rvv(char* str, const char* prefix)
#   - a0 = str
#   - a1 = prefix
#   - return (bool) in a0
#------------------------------------------------------------------------------
str_starts_with_rvv:
    # 함수 프로로그(스택에 ra, s0 등 저장)
    addi    sp, sp, -16
    sw      ra, 12(sp)
    sw      s0,  8(sp)

    #--------------------------------------------------------------------------
    # 1) prefix 길이 계산 (간단한 strlen 구현)
    #    - a1에 prefix 시작 주소 있음
    #    - s0에 prefix 시작 주소 백업
    #--------------------------------------------------------------------------
    mv      s0, a1      # s0 := prefix 시작 주소
    li      a2, 0       # a2 := prefix_len 계산용 (초기 0)

calc_prefix_len:
    lbu     a3, 0(a1)   # a3 := prefix[i]
    beqz    a3, prefix_len_done  # 만약 '\0'이면 길이 계산 끝
    addi    a2, a2, 1   # prefix_len++
    addi    a1, a1, 1   # prefix 포인터++
    j       calc_prefix_len

prefix_len_done:
    # a2 = prefix_len
    # prefix 포인터(a1) 원래대로 복원
    mv      a1, s0

    # t2 := prefix_len
    mv      t2, a2

    #--------------------------------------------------------------------------
    # 2) RVV를 사용해 chunk 단위로 prefix와 str 비교
    #--------------------------------------------------------------------------
    li      t0, -1      # t0 = -1 => vsetvli zero, t0, e8, ... 로 최대 VL 활용

loop_vector:
    # 남은 prefix_len(t2)이 0이면 이미 전부 매칭된 것
    beqz    t2, matched

    # vsetvli
    vsetvli zero, t0, e8, m8, ta, ma

    # prefix에서 chunk(=vl)만큼 로드
    vle8ff.v v8, (a1)
    # str에서도 동일 chunk만큼 로드
    vle8ff.v v9, (a0)

    # 이번에 실제 로드된 개수 (vl)
    csrr    t1, vl

    # 두 벡터 비교: 다르면(mask=1)
    vmsne.vv v0, v8, v9
    # mismatch가 처음 발생한 인덱스
    vfirst.m t3, v0

    # t3 < t1 => 현재 chunk 내 mismatch 존재
    blt     t3, t1, check_mismatch

    #--------------------------------------------------------------------------
    # 3) 이번 chunk에서는 mismatch가 없으므로 prefix_len에서 vl만큼 소모
    #--------------------------------------------------------------------------
    sub     t2, t2, t1       # prefix_len -= vl
    add     a1, a1, t1       # prefix_ptr += vl
    add     a0, a0, t1       # str_ptr    += vl
    j       loop_vector

#--------------------------------------------------------------------------
# 4) mismatch가 발생했을 때 실제로 prefix 범위 내인지 확인
#--------------------------------------------------------------------------
check_mismatch:
    # chunk 내 mismatch 위치 = t3
    # 글로벌 mismatch 오프셋 = (현재 prefix_ptr - prefix 시작주소) + t3
    mv      t4, a1
    sub     t4, t4, s0       # (a1 - s0) = 지금까지 매칭된 바이트 수
    add     t4, t4, t3       # t4 = 글로벌 mismatch 인덱스

    # prefix 범위 밖(>= prefix_len)이면 prefix는 일치로 간주
    bge     t4, t2, matched

mismatch:
    # prefix 범위 내 mismatch => false 반환
    li      a0, 0
    j       done

matched:
    # prefix_len을 소진하거나, mismatch가 prefix 범위를 벗어났으면 => true
    li      a0, 1

done:
    # 함수 에필로그(복원 및 리턴)
    lw      s0,  8(sp)
    lw      ra, 12(sp)
    addi    sp, sp, 16
    ret
