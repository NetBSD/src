#source: dummy.s
#as: --32
#ld: -m elf_i386 tmpdir/start32.o tmpdir/foox32.o
#error: .*i386:x64-32 architecture of input file `tmpdir/foox32.o' is incompatible with i386 output
