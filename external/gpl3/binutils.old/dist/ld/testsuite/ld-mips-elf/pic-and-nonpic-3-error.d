#name: PIC and non-PIC test 3 (error)
#source: pic-and-nonpic-3b.s
#as: -EB -32 -mips2
#ld: tmpdir/pic-and-nonpic-3a.so -melf32btsmip -znocopyreloc
#error: .*: non-dynamic relocations refer to dynamic symbol foo
