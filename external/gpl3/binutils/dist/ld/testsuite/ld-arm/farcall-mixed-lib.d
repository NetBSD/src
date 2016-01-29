tmpdir/farcall-mixed-lib.so:     file format elf32-(little|big)arm
architecture: arm.*, flags 0x00000150:
HAS_SYMS, DYNAMIC, D_PAGED
start address 0x.*

Disassembly of section .plt:

.* <app_func@plt-0x14>:
 .*:	e52de004 	push	{lr}		; \(str lr, \[sp, #-4\]!\)
 .*:	e59fe004 	ldr	lr, \[pc, #4\]	; .* <app_func@plt-0x4>
 .*:	e08fe00e 	add	lr, pc, lr
 .*:	e5bef008 	ldr	pc, \[lr, #8\]!
 .*:	.*
.* <app_func@plt>:
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!.*
.* <app_func_weak@plt>:
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!.*
.* <lib_func3@plt>:
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!.*
.* <lib_func4@plt>:
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!.*

Disassembly of section .text:

.* <lib_func1>:
 .*:	e1a0c00d 	mov	ip, sp
 .*:	e92dd800 	push	{fp, ip, lr, pc}
 .*:	ebffff.. 	bl	.* <app_func@plt>
 .*:	ebffff.. 	bl	.* <app_func_weak@plt>
 .*:	ebfffff. 	bl	.* <lib_func3@plt>
 .*:	ebfffff. 	bl	.* <lib_func4@plt>
 .*:	e89d6800 	ldm	sp, {fp, sp, lr}
 .*:	e12fff1e 	bx	lr
	...

.* <lib_func2>:
 .*:	f000 e8.. 	blx	.* <__app_func_from_thumb>
 .*:	f000 e8.. 	blx	.* <__app_func_weak_from_thumb>
 .*:	f000 e8.. 	blx	.* <__lib_func3_from_thumb>
 .*:	f000 e8.. 	blx	.* <__lib_func4_from_thumb>
 .*:	4770      	bx	lr
 .*:	46c0      	nop.*
#...

.* <__lib_func3_from_thumb>:
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__lib_func3_from_thumb\+0x8>
 .*:	e08ff00c 	add	pc, pc, ip
 .*:	feffff.. 	.word	0xfeffff..

.* <__app_func_weak_from_thumb>:
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__app_func_weak_from_thumb\+0x8>
 .*:	e08ff00c 	add	pc, pc, ip
 .*:	feffff.. 	.word	0xfeffff..

.* <__lib_func4_from_thumb>:
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__lib_func4_from_thumb\+0x8>
 .*:	e08ff00c 	add	pc, pc, ip
 .*:	feffff.. 	.word	0xfeffff..

.* <__app_func_from_thumb>:
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__app_func_from_thumb\+0x8>
 .*:	e08ff00c 	add	pc, pc, ip
 .*:	feffff.. 	.word	0xfeffff..
	...

.* <lib_func3>:
 .*:	f000 e80c 	blx	200037c <__app_func_from_thumb>
 .*:	f000 e804 	blx	2000370 <__app_func_weak_from_thumb>
 .*:	4770      	bx	lr
#...

.* <__app_func_weak_from_thumb>:
 .*:	e59fc000 	ldr	ip, \[pc\]	; 2000378 <__app_func_weak_from_thumb\+0x8>
 .*:	e08ff00c 	add	pc, pc, ip
 .*:	fdffff34 	.word	0xfdffff34

.* <__app_func_from_thumb>:
 .*:	e59fc000 	ldr	ip, \[pc\]	; 2000384 <__app_func_from_thumb\+0x8>
 .*:	e08ff00c 	add	pc, pc, ip
 .*:	fdffff1c 	.word	0xfdffff1c
	...
