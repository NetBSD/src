	.text

	.globl	_startit
_startit:
	move.l	sp,a3
	move.l	4,a6
	lea	(start_super,pc),a5
	jmp	(-0x1e,a6)		| supervisor-call

start_super:
	move.w	#0x2700,sr

	| the BSD kernel wants values into the following registers:
	| a0:  fastmem-start
	| d0:  fastmem-size
	| d1:  chipmem-size
	| d3:  Amiga specific flags
	| d4:  E clock frequency
	| d5:  AttnFlags (cpuid)
	| d7:  boothowto
	| a4:  esym location
	| a2:  Inhibit sync flags
	| All other registers zeroed for possible future requirements.

	lea	(_startit,pc),sp	| make sure we have a good stack ***

	move.l	(4,a3),a1		| loaded kernel
	move.l	(8,a3),d2		| length of loaded kernel
|	move.l	(12,a3),sp		| entry point in stack pointer
	move.l	(12,a3),a6		| push entry point		***
	move.l	(16,a3),a0		| fastmem-start
	move.l	(20,a3),d0		| fastmem-size
	move.l	(24,a3),d1		| chipmem-size
	move.l	(28,a3),d7		| boothowto
	move.l	(32,a3),a4		| esym
	move.l	(36,a3),d5		| cpuid
	move.l	(40,a3),d4		| E clock frequency
	move.l	(44,a3),d3		| Amiga flags
	move.l	(48,a3),a2		| Inhibit sync flags
	move.l	(52,a3),d6		| Load to fastmem flag
	sub.l	a5,a5			| target, load to 0

	cmp.b	#0x7D,(36,a3)		| is it DraCo?
	beq	nott			| yes, switch off MMU later

					| no, it is an Amiga:

|	move.w	#0xf00,0xdff180		|red
|	move.b	#0,0x200003c8
|	move.b	#63,0x200003c9
|	move.b	#0,0x200003c9
|	move.b	#0,0x200003c9

	move.w	#(1<<9),0xdff096	| disable DMA on Amigas.

| ------ mmu off start -----

	btst	#3,d5			| AFB_68040,SysBase->AttnFlags
	beq	not040

| Turn off 68040/060 MMU

	sub.l	a3,a3
	.word 0x4e7b,0xb003		| movec a3,tc
	.word 0x4e7b,0xb806		| movec a3,urp
	.word 0x4e7b,0xb807		| movec a3,srp
	.word 0x4e7b,0xb004		| movec a3,itt0
	.word 0x4e7b,0xb005		| movec a3,itt1
	.word 0x4e7b,0xb006		| movec a3,dtt0
	.word 0x4e7b,0xb007		| movec a3,dtt1
	bra	nott

not040:
	lea	(zero,pc),a3
	pmove	(a3),tc			| Turn off MMU
	lea	(nullrp,pc),a3
	pmove	(a3),crp		| Turn off MMU some more
	pmove	(a3),srp		| Really, really, turn off MMU

| Turn off 68030 TT registers

	btst	#2,d5			| AFB_68030,SysBase->AttnFlags
	beq	nott			| Skip TT registers if not 68030
	lea	(zero,pc),a3
	.word 0xf013,0x0800		| pmove a3@,tt0 (gas only knows about 68851 ops..)
	.word 0xf013,0x0c00		| pmove a3@,tt1 (gas only knows about 68851 ops..)

nott:
| ---- mmu off end ----
|	move.w	#0xf60,0xdff180		| orange
|	move.b	#0,0x200003c8
|	move.b	#63,0x200003c9
|	move.b	#24,0x200003c9
|	move.b	#0,0x200003c9

| ---- copy kernel start ----

	tst.l	d6			| Can we load to fastmem?
	beq	L0			| No, leave destination at 0
	move.l	a0,a5			| Move to start of fastmem chunk
	add.l	a0,a6			| relocate kernel entry point
L0:
	move.l	(a1)+,(a5)+
	sub.l	#4,d2
	bcc	L0

	lea	(ckend,pc),a1
	move.l	a5,-(sp)
	move.l	#_startit_end-ckend,d2
L2:
	move.l	(a1)+,(a5)+
	sub.l	#4,d2
	bcc	L2

	btst	#3,d5
	jeq	L1
	.word	0xf4f8
L1:
	moveq.l	#0,d2			| switch off cache to ensure we use
	movec	d2,cacr			| valid kernel data

|	move.w	#0xFF0,0xdff180		| yellow
|	move.b	#0,0x200003c8
|	move.b	#63,0x200003c9
|	move.b	#0,0x200003c9
|	move.b	#0,0x200003c9
	rts

| ---- copy kernel end ----

ckend:
|	move.w	#0x0ff,0xdff180		| petrol
|	move.b	#0,0x200003c8
|	move.b	#0,0x200003c9
|	move.b	#63,0x200003c9
|	move.b	#63,0x200003c9

	move.l	d5,d2
	rol.l	#8,d2
	cmp.b	#0x7D,d2
	jne	noDraCo

| DraCo: switch off MMU now:

	sub.l	a3,a3
	.word 0x4e7b,0xb003		| movec a3,tc
	.word 0x4e7b,0xb806		| movec a3,urp
	.word 0x4e7b,0xb807		| movec a3,srp
	.word 0x4e7b,0xb004		| movec a3,itt0
	.word 0x4e7b,0xb005		| movec a3,itt1
	.word 0x4e7b,0xb006		| movec a3,dtt0
	.word 0x4e7b,0xb007		| movec a3,dtt1

noDraCo:
	moveq	#0,d2			| zero out unused registers
	moveq	#0,d6			| (might make future compatibility
	move.l	d6,a1			|  would have known contents)
	move.l	d6,a3
	move.l	d6,a5
	move.l	a6,sp			| entry point into stack pointer
	move.l	d6,a6

|	move.w	#0x0F0,0xdff180		| green
|	move.b	#0,0x200003c8
|	move.b	#0,0x200003c9
|	move.b	#63,0x200003c9
|	move.b	#0,0x200003c9

	jmp	(sp)			| jump to kernel entry point

| A do-nothing MMU root pointer (includes the following long as well)

nullrp:	.long	0x7fff0001
zero:	.long	0

_startit_end:

	.data
	.globl	_startit_sz
_startit_sz: .long _startit_end-_startit
