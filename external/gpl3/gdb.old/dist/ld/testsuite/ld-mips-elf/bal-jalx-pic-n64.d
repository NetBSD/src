#name: MIPS BAL/JALX in PIC mode (n64)
#source: ../../../gas/testsuite/gas/mips/branch-addend.s
#ld: -Ttext 0x1c000000 -e 0x1c000000 -shared
#target: [check_shared_lib_support]
#error: \A[^\n]*: in function `bar':\n
#error:   \(\.text\+0x1014\): unsupported branch between ISA modes\Z
