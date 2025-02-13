.section .text
.balign 4
.global str_starts_with_rvv
str_starts_with_rvv:
    li   a3, 1               # Assume true (return value)

    li   t0, -1              # Set maximum VL
loop_vector:
    # 1) Set vector length
    vsetvli x0, t0, e8, m8, ta, ma

    # 2) Load chunks of prefix and str
    vle8.v v8, (a1)          # Load prefix chunk into v8
    vle8.v v9, (a0)          # Load str chunk into v9
    csrr t1, vl              # Actual VL for this chunk

    # 3) Compare prefix and str
    vmsne.vv v0, v8, v9      # v0: mask of mismatches
    vfirst.m a4, v0          # Find first mismatch index
    bge a4, zero, return_false  # Mismatch found

    # 4) Check for NULL in prefix
    vmseq.vi v0, v8, 0       # Check for NULLs in prefix
    vfirst.m a4, v0
    bge a4, zero, return_true   # NULL found in prefix

    # 5) Advance pointers
    add a1, a1, t1           # Move prefix pointer
    add a0, a0, t1           # Move str pointer

    # 6) Check for end of str (NULL in str)
    vmseq.vi v0, v9, 0       # Check for NULLs in str
    vfirst.m a4, v0
    bge a4, zero, return_false  # NULL in str before prefix ends

    j loop_vector            # Continue loop

# Return false
return_false:
    li a3, 0                 # Set return value to false
    mv a0, a3
    ret

# Return true
return_true:
    li a3, 1                 # Set return value to true
    mv a0, a3
    ret
