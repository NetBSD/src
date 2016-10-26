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
 .*:	.* 	.word	.*
.* <app_func@plt>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!	; .*
.* <app_func_weak@plt>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!	; 0x.*
.* <lib_func3@plt>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!	; 0x.*
.* <lib_func4@plt>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!	; 0x.*

Disassembly of section .text:

.* <lib_func1>:
 .*:	e1a0c00d 	mov	ip, sp
 .*:	e92dd800 	push	{fp, ip, lr, pc}
 .*:	ebffff.. 	bl	.* <app_func@plt\+0x.*>
 .*:	ebffff.. 	bl	.* <app_func_weak@plt\+0x.*>
 .*:	ebffff.. 	bl	.* <lib_func3@plt\+0x.*>
 .*:	ebffff.. 	bl	.* <lib_func4@plt\+0x.*>
 .*:	e89d6800 	ldm	sp, {fp, sp, lr}
 .*:	e12fff1e 	bx	lr
	...

.* <__real_lib_func2>:
 .*:	f000 f8.. 	bl	.* <__app_func_from_thumb>
 .*:	f000 f8.. 	bl	.* <__app_func_weak_from_thumb>
 .*:	f000 f8.. 	bl	.* <__lib_func3_from_thumb>
 .*:	f000 f8.. 	bl	.* <__lib_func4_from_thumb>
 .*:	4770      	bx	lr
#...

.* <__app_func_from_thumb>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__app_func_from_thumb\+0xc>
 .*:	e08cf00f 	add	pc, ip, pc
 .*:	feffff.. 	.word	0xfeffff..

.* <__lib_func4_from_thumb>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__lib_func4_from_thumb\+0xc>
 .*:	e08cf00f 	add	pc, ip, pc
 .*:	feffff.. 	.word	0xfeffff..

.* <__app_func_weak_from_thumb>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__app_func_weak_from_thumb\+0xc>
 .*:	e08cf00f 	add	pc, ip, pc
 .*:	feffff.. 	.word	0xfeffff..

.* <__lib_func3_from_thumb>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e59fc000 	ldr	ip, \[pc\]	; .* <__lib_func3_from_thumb\+0xc>
 .*:	e08cf00f 	add	pc, ip, pc
 .*:	feffff.. 	.word	0xfeffff..
	...

.* <__real_lib_func3>:
 .*:	f000 f80e 	bl	2000390 <__app_func_from_thumb>
 .*:	f000 f804 	bl	2000380 <__app_func_weak_from_thumb>
 .*:	4770      	bx	lr
#...

.* <__app_func_weak_from_thumb>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e59fc000 	ldr	ip, \[pc\]	; 200038c <__app_func_weak_from_thumb\+0xc>
 .*:	e08cf00f 	add	pc, ip, pc
 .*:	fdffff28 	.word	0xfdffff28

.* <__app_func_from_thumb>:
 .*:	4778      	bx	pc
 .*:	46c0      	nop			; \(mov r8, r8\)
 .*:	e59fc000 	ldr	ip, \[pc\]	; 200039c <__app_func_from_thumb\+0xc>
 .*:	e08cf00f 	add	pc, ip, pc
 .*:	fdffff08 	.word	0xfdffff08

.* <lib_func3>:
 .*:	e59fc004 	ldr	ip, \[pc, #4\]	; 20003ac <lib_func3\+0xc>
 .*:	e08cc00f 	add	ip, ip, pc
 .*:	e12fff1c 	bx	ip
 .*:	ffffffc5 	.word	0xffffffc5

.* <lib_func2>:
 .*:	e59fc004 	ldr	ip, \[pc, #4\]	; 20003bc <lib_func2\+0xc>
 .*:	e08cc00f 	add	ip, ip, pc
 .*:	e12fff1c 	bx	ip
 .*:	feffff55 	.word	0xfeffff55
