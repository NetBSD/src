
tmpdir/farcall-mixed-app-v5:     file format elf32-(little|big)arm
architecture: armv4t, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x.*

Disassembly of section .plt:

.* <lib_func2@plt-0x14>:
 .*:	e52de004 	push	{lr}		; \(str lr, \[sp, #-4\]!\)
 .*:	e59fe004 	ldr	lr, \[pc, #4\]	; .* <lib_func2@plt-0x4>
 .*:	e08fe00e 	add	lr, pc, lr
 .*:	e5bef008 	ldr	pc, \[lr, #8\]!
 .*:	.*
.* <lib_func2@plt>:
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!.*
.* <lib_func1@plt>:
 .*:	e28fc6.* 	add	ip, pc, #.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!.*

Disassembly of section .text:

.* <_start>:
 .*:	e1a0c00d 	mov	ip, sp
 .*:	e92dd800 	push	{fp, ip, lr, pc}
 .*:	eb000008 	bl	.* <__app_func_veneer>
 .*:	ebfffff5 	bl	.* <lib_func1@plt>
 .*:	ebfffff1 	bl	.* <lib_func2@plt>
 .*:	e89d6800 	ldm	sp, {fp, sp, lr}
 .*:	e12fff1e 	bx	lr
 .*:	e1a00000 	nop			; \(mov r0, r0\)

.* <app_tfunc_close>:
 .*:	b500      	push	{lr}
 .*:	f7ff efdc 	blx	.* <lib_func2@plt>
 .*:	bd00      	pop	{pc}
 .*:	4770      	bx	lr
 .*:	46c0      	nop			; \(mov r8, r8\)
#...

.* <__app_func_veneer>:
 .*:	e51ff004 	ldr	pc, \[pc, #-4\]	; .* <__app_func_veneer\+0x4>
 .*:	02100000 	.word	0x02100000

Disassembly of section .far_arm:

.* <app_func>:
 .*:	e1a0c00d 	mov	ip, sp
 .*:	e92dd800 	push	{fp, ip, lr, pc}
 .*:	eb0000.. 	bl	.* <__lib_func1_veneer>
 .*:	eb0000.. 	bl	.* <__lib_func2_veneer>
 .*:	e89d6800 	ldm	sp, {fp, sp, lr}
 .*:	e12fff1e 	bx	lr
 .*:	e1a00000 	nop			; \(mov r0, r0\)
 .*:	e1a00000 	nop			; \(mov r0, r0\)

.* <app_func2>:
 .*:	e12fff1e 	bx	lr
#...

.* <__lib_func1_veneer>:
 .*:	e51ff004 	ldr	pc, \[pc, #-4\]	; .* <__lib_func1_veneer\+0x4>
 .*:	000081e8 	.word	0x000081e8
.* <__lib_func2_veneer>:
 .*:	e51ff004 	ldr	pc, \[pc, #-4\]	; .* <__lib_func2_veneer\+0x4>
 .*:	000081dc 	.word	0x000081dc

Disassembly of section .far_thumb:

.* <app_tfunc>:
 .*:	b500      	push	{lr}
 .*:	f000 e806 	blx	.* <__lib_func2_from_thumb>
 .*:	bd00      	pop	{pc}
 .*:	4770      	bx	lr
 .*:	46c0      	nop			; \(mov r8, r8\)
#...

.* <__lib_func2_from_thumb>:
 .*:	e51ff004 	ldr	pc, \[pc, #-4\]	; 2200014 <__lib_func2_from_thumb\+0x4>
 .*:	000081dc 	.word	0x000081dc
