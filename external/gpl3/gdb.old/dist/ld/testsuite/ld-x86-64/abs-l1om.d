#name: Absolute non-overflowing relocs
#source: ../ld-i386/abs.s
#source: ../ld-i386/zero.s
#as: --64 -march=l1om
#ld: -m elf_l1om -z noseparate-code
#objdump: -rs -j .text
#target: x86_64-*-linux*

.*:     file format .*

Contents of section \.text:
[ 	][0-9a-f]+ c800fff0 c8000110 c9c3.*
