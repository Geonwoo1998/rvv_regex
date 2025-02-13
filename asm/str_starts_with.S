.text
.balign 4
.global str_starts_with
str_starts_with:
    li a3, 1               # Initialize return value to true (1)
loop:
    lbu t0, (a0)           # Load byte from str
    lbu t1, (a1)           # Load byte from prefix
    beqz t1, end           # If prefix is null-terminator, return true
    bne t0, t1, false      # If str and prefix differ, return false
    addi a0, a0, 1         # Increment str pointer
    addi a1, a1, 1         # Increment prefix pointer
    j loop                 # Continue loop

false:
    li a3, 0               # Set return value to false (0)

end:
    mv a0, a3              # Move return value to a0
    ret                    # Return