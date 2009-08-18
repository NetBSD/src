#name: PIC and non-PIC test 4 (error)
#source: pic-and-nonpic-4b.s
#as: -EB -32 -mips2
#ld: tmpdir/pic-and-nonpic-4a.so -melf32btsmip -znocopyreloc
#error: .*: non-dynamic relocations refer to dynamic symbol obj1
