#source: compress1.s
#as: --compress-debug-sections=none
#ld: -r --compress-debug-sections=zlib-gnu
#readelf: -SW
#xfail: [uses_genelf]
#xfail: riscv*-*-*
# Not all ELF targets use the elf.em emulation...
# RISC-V has linker relaxations that delete code, so text label subtractions
# do not get resolved at assembly time, which results in a compressed section.

#failif
#...
  \[[ 0-9]+\] \.zdebug_aranges[ 	]+(PROGBITS|MIPS_DWARF)[ 	0-9a-z]+ .*
#...
