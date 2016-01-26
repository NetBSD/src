# name: ELF MIPS16 ASE markings
# source: empty.s
# objdump: -p
# as: -32 -mips16

.*:.*file format.*mips.*
!private flags = .*mips16.*

