#name: microMIPS JALX to unaligned symbol 1
#source: unaligned-jalx-1.s -mmicromips
#source: unaligned-insn.s
#as: -EB -32
#ld: -EB -Ttext 0x1c000000 -e 0x1c000000
#error: \A[^\n]*: in function `foo':\n
#error:   \(\.text\+0x[0-9a-f]+\): cannot convert a jump to JALX for a non-word-aligned address\Z
