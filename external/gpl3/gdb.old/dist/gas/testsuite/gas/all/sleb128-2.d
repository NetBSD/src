#objdump : -s -j .data -j "\$DATA\$"
#name : .sleb128 tests (2)
# RISC-V doesn't support .sleb operands that are the difference of two symbols
# because symbol values are not known until after linker relaxation has been
# performed.
#notarget : riscv*-*-*

.*: .*

Contents of section (\.data|\$DATA\$):
 .* 7d2a.*
