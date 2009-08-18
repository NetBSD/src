#name: MIPS multi-got-hidden-1
#as: -EB -32 -KPIC
#source: multi-got-1-1.s
#source: multi-got-1-2.s
#source: multi-got-hidden-1.s
#ld: -melf32btsmip -e 0
#objdump: -dr
#pass
