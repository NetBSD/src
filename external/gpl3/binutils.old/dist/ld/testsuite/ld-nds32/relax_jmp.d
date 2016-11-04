#as: -Os
#ld: -static --relax -T	$srcdir/$subdir/relax_jmp.ld
#objdump: -d --prefix-addresses -j .text

.*:     file format .*nds32.*


Disassembly of section .text:
0+0000 <[^>]*> j8	00000006 <main>
0+0002 <[^>]*> jal	00000006 <main>
0+0006 <[^>]*> srli45	\$r0, 0

