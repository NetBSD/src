#as: -EB
#objdump: -dr --prefix-addresses --show-raw-insn -M reg-names=numeric
#name: uld2 -EB
#source: uld2.s

# Further checks of uld macro.

.*: +file format .*mips.*

Disassembly of section .text:
0+0000 <[^>]*> 68a40000 	ldl	\$4,0\(\$5\)
0+0004 <[^>]*> 6ca40007 	ldr	\$4,7\(\$5\)
0+0008 <[^>]*> 68a40001 	ldl	\$4,1\(\$5\)
0+000c <[^>]*> 6ca40008 	ldr	\$4,8\(\$5\)
0+0010 <[^>]*> 68a10000 	ldl	\$1,0\(\$5\)
0+0014 <[^>]*> 6ca10007 	ldr	\$1,7\(\$5\)
0+0018 <[^>]*> 00202825 	move	\$5,\$1
0+001c <[^>]*> 68a10001 	ldl	\$1,1\(\$5\)
0+0020 <[^>]*> 6ca10008 	ldr	\$1,8\(\$5\)
0+0024 <[^>]*> 00202825 	move	\$5,\$1
	\.\.\.
