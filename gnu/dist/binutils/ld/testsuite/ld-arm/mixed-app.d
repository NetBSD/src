
tmpdir/mixed-app:     file format elf32-(little|big)arm
architecture: arm, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x.*

Disassembly of section .plt:

.* <.plt>:
 .*:	e52de004 	str	lr, \[sp, #-4\]!
 .*:	e59fe004 	ldr	lr, \[pc, #4\]	; .* <.plt\+0x10>
 .*:	e08fe00e 	add	lr, pc, lr
 .*:	e5bef008 	ldr	pc, \[lr, #8\]!
 .*:	.*
 .*:	(46c04778 	undefined|477846c0 	ldrmib	r4, \[r8, -r0, asr #13\]!)
 .*:	e28fc6.* 	add	ip, pc, #.*	; 0x.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!
 .*:	e28fc6.* 	add	ip, pc, #.*	; 0x.*
 .*:	e28cca.* 	add	ip, ip, #.*	; 0x.*
 .*:	e5bcf.* 	ldr	pc, \[ip, #.*\]!
Disassembly of section .text:

.* <_start>:
 .*:	e1a0c00d 	mov	ip, sp
 .*:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
 .*:	eb000004 	bl	.* <app_func>
 .*:	e89d6800 	ldmia	sp, {fp, sp, lr}
 .*:	e12fff1e 	bx	lr
 .*:	e1a00000 	nop			\(mov r0,r0\)
 .*:	e1a00000 	nop			\(mov r0,r0\)
 .*:	e1a00000 	nop			\(mov r0,r0\)

.* <app_func>:
 .*:	e1a0c00d 	mov	ip, sp
 .*:	e92dd800 	stmdb	sp!, {fp, ip, lr, pc}
 .*:	ebffffe. 	bl	.* <.text-0x..>
 .*:	e89d6800 	ldmia	sp, {fp, sp, lr}
 .*:	e12fff1e 	bx	lr
 .*:	e1a00000 	nop			\(mov r0,r0\)
 .*:	e1a00000 	nop			\(mov r0,r0\)
 .*:	e1a00000 	nop			\(mov r0,r0\)

.* <app_func2>:
 .*:	e12fff1e 	bx	lr
 .*:	e1a00000 	nop			\(mov r0,r0\)
 .*:	e1a00000 	nop			\(mov r0,r0\)
 .*:	e1a00000 	nop			\(mov r0,r0\)

.* <app_tfunc>:
 .*:	b500      	push	{lr}
 .*:	(ffc.f7ff|f7ffffc.) 	bl	.* <.text-0x..>
 .*:	bd00      	pop	{pc}
 .*:	4770      	bx	lr
 .*:	46c0      	nop			\(mov r8, r8\)
 .*:	46c0      	nop			\(mov r8, r8\)
 .*:	46c0      	nop			\(mov r8, r8\)
