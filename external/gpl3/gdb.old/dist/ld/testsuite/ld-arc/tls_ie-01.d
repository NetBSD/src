#source: tls_ie-01.s
#as: -mcpu=arc700
#ld: -static
#objdump: -s -j .got
#xfail: arc*-*-elf*

[^:]+:     file format elf32-.*arc

Contents of section \.got:
 [0-9a-f]+ [08]+ [0c]+ +.+
