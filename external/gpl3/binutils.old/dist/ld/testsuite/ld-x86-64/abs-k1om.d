#name: Absolute non-overflowing relocs
#source: ../ld-i386/abs.s
#source: ../ld-i386/zero.s
#as: --64 -march=k1om
#ld: -m elf_k1om
#objdump: -rs -j .text

.*:     file format .*

Contents of section \.text:
[ 	][0-9a-f]+ c800fff0 c8000110 c9c3.*
