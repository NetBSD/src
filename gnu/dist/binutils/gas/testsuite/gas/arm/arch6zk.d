#name: ARM V6 instructions
#as: -march=armv6zk
#objdump: -dr --prefix-addresses --show-raw-insn

.*: +file format .*arm.*

Disassembly of section .text:
0+000 <[^>]*> f57ff01f ?	clrex
0+004 <[^>]*> e1dc3f9f ?	ldrexb	r3, \[ip\]
0+008 <[^>]*> 11d3cf9f ?	ldrexbne	ip, \[r3\]
0+00c <[^>]*> e1bc3f9f ?	ldrexd	r3, \[ip\]
0+010 <[^>]*> 11b3cf9f ?	ldrexdne	ip, \[r3\]
0+014 <[^>]*> e1fc3f9f ?	ldrexh	r3, \[ip\]
0+018 <[^>]*> 11f3cf9f ?	ldrexhne	ip, \[r3\]
0+01c <[^>]*> e320f080 ?	nop	\{128\}
0+020 <[^>]*> 1320f07f ?	nopne	\{127\}
0+024 <[^>]*> e320f004 ?	sev
0+028 <[^>]*> e1c73f9c ?	strexb	r3, ip, \[r7\]
0+02c <[^>]*> 11c8cf93 ?	strexbne	ip, r3, \[r8\]
0+030 <[^>]*> e1a73f9c ?	strexd	r3, ip, \[r7\]
0+034 <[^>]*> 11a8cf93 ?	strexdne	ip, r3, \[r8\]
0+038 <[^>]*> e1e73f9c ?	strexh	r3, ip, \[r7\]
0+03c <[^>]*> 11e8cf93 ?	strexhne	ip, r3, \[r8\]
0+040 <[^>]*> e320f002 ?	wfe
0+044 <[^>]*> e320f003 ?	wfi
0+048 <[^>]*> e320f001 ?	yield
0+04c <[^>]*> e16ec371 ?	smi	60465
0+050 <[^>]*> 11613c7e ?	smine	5070
0+054 <[^>]*> e1a00000 ?	nop[ 	]+\(mov r0,r0\)
0+058 <[^>]*> e1a00000 ?	nop[ 	]+\(mov r0,r0\)
0+05c <[^>]*> e1a00000 ?	nop[ 	]+\(mov r0,r0\)

