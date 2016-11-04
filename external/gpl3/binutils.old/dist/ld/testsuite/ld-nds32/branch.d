#as: -Os
#ld: -static --relax -T	$srcdir/$subdir/branch.ld
#objdump: -d --prefix-addresses -j .text

.*:     file format .*nds32.*


Disassembly of section .text:
0+0000 <[^>]*> beq	\$r0, \$r1, 0000002c <main>
0+0004 <[^>]*> bne	\$r0, \$r1, 0000002c <main>
0+0008 <[^>]*> bnez38	\$r0, 0000002c <main>
0+000a <[^>]*> beqz38	\$r0, 0000002c <main>
0+000c <[^>]*> bgez	\$r0, 0000002c <main>
.*
0+0012 <[^>]*> bgezal	\$r0, 0000002c <main>
0+0016 <[^>]*> bgtz	\$r0, 0000002c <main>
.*
0+001c <[^>]*> blez	\$r0, 0000002c <main>
.*
0+0022 <[^>]*> bltz	\$r0, 0000002c <main>
0+0026 <[^>]*> srli45	\$r0, 0
0+0028 <[^>]*> bltzal	\$r0, 0000002c <main>
0+002c <main>.*

