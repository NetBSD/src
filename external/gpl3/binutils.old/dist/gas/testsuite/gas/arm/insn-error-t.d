# Check that changing thumb -> arm state immediately 
# after an invalid instruction does not cause an internal error.
#name: invalid instruction recovery test - Thumb version
#objdump: -d --prefix-addresses --show-raw-insn
#skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*
#error-output: insn-error-t.l
