# name: ELF MIPS16 ASE markings
# source: elf_ase_mips16.s
# objdump: -p
# as: -mips16

.*:.*file format.*mips.*
private flags = [0-9a-f]*[4-7c-f]......: .*[[,]mips16[],].*

