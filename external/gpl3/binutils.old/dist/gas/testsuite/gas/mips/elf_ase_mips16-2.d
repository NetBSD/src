# name: ELF MIPS16 ASE markings 2
# source: nop.s
# objdump: -p
# as: -32 -mips16

.*:.*file format.*mips.*
private flags = [0-9a-f]*[4-7c-f]......: .*[[,]mips16[],].*

