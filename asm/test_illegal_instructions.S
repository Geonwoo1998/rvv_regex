.section .text
.balign 4
.global test_illegal_instructions

test_illegal_instructions:
    li   t0, -1             # 테스트 1: 정수 로드 (정상적으로 동작해야 함)

    vsetvli x0, t0, e8, m8, ta, ma

    addi sp, sp, -16
    vle8ff.v v8, (sp)
    csrr t1, vl              

    vmsne.vv v0, v8, v8      

    vfirst.m a4, v0

    vmseq.vi v0, v8, 0       

    addi sp, sp, 16          

    ret                      
