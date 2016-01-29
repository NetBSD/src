#name: PC-Rel relocation against defined
#source: pcrel.s
#objdump: -r
#ld: -shared -e0 -defsym global_a=0x1000
#...
