;	$NetBSD: start.S,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $

; Copyright (c) 2003 ITOH Yasufumi.
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary forms are unlimited.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS''
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
; THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
; BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
; THE POSSIBILITY OF SUCH DAMAGE.

	.level	1.0

	.code
	.origin	0
	;
	; LIF (Logical Interchange Format) header
	;
lifhdr:	.byte	0x80,0x00	; LIF magic
	.string	"NetBSD"	; volume label (6 chars, fill with space)
	.origin	0xf0
	; 0xf0
lif_ipl_addr:
	.word	top-lifhdr	; start at 4KB (must be 2KB aligned)
lif_ipl_size:
	.word	0x00001000	; size 4KB (must be 2KB aligned)
lif_ipl_entry:
	.word	$START$-top	; entry offset

	; ipl part 1 starts here
	.origin	4096
top:
	;
	; IPL startup
	;
	; arg0 = interact flag (1: interactive, 0: otherwise)
	; arg1 = address of first doubleword past the end of ipl part 1
	;
	.export $START$,entry
$START$:
	b,n	start			; 0: entry address

; version of interface of primary to secondary boot
BOOT_IF_VERSION:	.equ	0
; version 0: arg0 = interact flag, arg1 = version (0),
;	     arg2 = end addr, arg3 = selected boot partition
;	     r1, r3 - r22, r28, r29, r31 = cleared to zeros

cksum:		.word	0		; 4: checksum will be stored here
version:	.word	BOOT_IF_VERSION	; 8: version of interface
rsvd1:		.word	0		; 12: future use
rsvd2:		.word	0		; 16: future use
rsvd3:		.word	0		; 20: future use

start:
	; set data pointer for relocatable data access
	blr	%r0,%r27
	; get PSW at entry
	ssm	0,%r4
	.export	$global$,data
$global$:

	; save parameters for main
	copy	%arg0,%r3

tmpdiskbufsz:		.equ	0x1000	; 4KB
tmpdiskbuf_labelsec:	.equ	0x0200	; dbtob(LABELSECTOR)
tmpdiskbuf_labelsecsz:	.equ	512
tmpdiskbuf_part2:	.equ	0x0400
tmpdiskbuf_part2sz:	.equ	0x0400
tmpdiskbuf_part3:	.equ	0x0A00
tmpdiskbuf_part3sz:	.equ	0x0600

part1sz:		.equ	0x1000

	; get next free address
	.import	_end,data
	addil	L%_end-$global$,%r27;%r1
	ldo	R%_end-$global$(%r1),%r1

	; 32bit environment (and this code) requires stack is 64byte aligned.
	ldi	64-1,%r2
	add	%r1,%r2,%r1
	andcm	%r1,%r2,%r6			; r6 = tmp disk buffer
	ldo	tmpdiskbufsz+64(%r6),%sp	; tmp stack

	bl	print,%rp
	ldo	str_startup-$global$(%r27),%arg0

	;
	; read part 2 and 3 of ipl (see README.ipl)
	;

	; read disk blocks which contains ipl part 2 and part 3
	copy	%r6,%arg0
	ldi	0,%arg2			; offset = 0
	bl	boot_input,%rp
	ldi	tmpdiskbufsz,%arg1	; read size

	; part 2 address
	ldo	top-$global$+part1sz(%r27),%r19

	; copy part 2
	ldo	tmpdiskbuf_part2(%r6),%r20
	addi,tr	tmpdiskbuf_part2sz/4,%r0,%r2	; loop count, skip next
cpipl2:	stws,ma		%r1,4(0,%r19)		; write to dst
	addib,uv,n	-1,%r2,cpipl2		; check loop condition
	ldws,ma		4(0,%r20),%r1		; read from src

	; copy part 3
	; (r19 already has destination address of part 3)
	ldo	tmpdiskbuf_part3(%r6),%r20
	addi,tr	tmpdiskbuf_part3sz/4,%r0,%r2	; loop count, skip next
cpipl3:	stws,ma		%r1,4(0,%r19)		; write to dst
	addib,uv,n	-1,%r2,cpipl3		; check loop condition
	ldws,ma		4(0,%r20),%r1		; read from src

	; flush data cache / invalidate instruction cache
	ldo	top-$global$+part1sz(%r27),%r1	; part 2 address
	ldi	16,%r20		; 16: cache line size
flipl:	fdc	0(0,%r1)	; flush data cache line at r1
	comb,<	%r1,%r19,flipl
	fic,m	%r20(0,%r1)	; flush instruction cache line at r1, r1 += 16
	sync			; I/O operation is guaranteed to finish
				; in eight instructions after sync
	;
	; Now, whole the IPL is loaded
	;

	; clear BSS
	.import	_edata,data
	addil	L%_edata-$global$,%r27;%r1
	ldo	R%_edata-$global$(%r1),%r1
clrbss:	comb,<	%r1,%r6,clrbss
	stws,ma	%r0,4(0,%r1)

	; we have read disklabel -- save it for later use
	.import	labelsector,data
	addil	L%labelsector-$global$,%r27;%r1
	ldo	R%labelsector-$global$(%r1),%r20
	ldo	tmpdiskbuf_labelsec(%r6),%r21
	addi,tr	tmpdiskbuf_labelsecsz/4,%r0,%r2	; loop count, skip next
cplbl:	stws,ma		%r1,4(0,%r20)		; write to dst
	addib,uv,n	-1,%r2,cplbl		; check loop condition
	ldws,ma		4(0,%r21),%r1		; read from src

	; set stack
	; (r6 points at free space, 64byte aligned)
	; 32bit environment (and this code) requires stack is 64byte aligned.
	ldo	64(%r6),%sp	; 64 > 48: frame marker (32) + args(up to 4)

	; stack usage
	;	12bytes	arguments
	;	32	frame marker

	; parameters for main
	copy	%r3,%arg0
	copy	%r4,%arg2

	.import	ipl_main,entry
	bl	ipl_main,%rp
	copy	%sp,%arg1

	; main returned --- perform reset
	bl	print,%rp
	ldo	str_reset-$global$(%r27),%arg0
	; FALLTHROUGH

IOMOD_CMD:		.equ	0xFFFC0000 + (4*12)
IOMOD_CMD_STOP:		.equ	0
IOMOD_CMD_RESET:	.equ	5

; void reboot(void)
; void halt(void)
	.export reboot,entry
	.export halt,entry
reboot:
	addi,tr	IOMOD_CMD_RESET,%r0,%r1	; %r1 = IOMOD_CMD_RESET, skip next
halt:	ldi	IOMOD_CMD_STOP,%r1
iomcmd:	ldil	L%IOMOD_CMD,%r2
	ldo	R%IOMOD_CMD(%r2),%r2
	stwas	%r1,0(%r2)
	b,n	.

str_startup:
	.string		"\r\n\n"
	.stringz	"NetBSD/hppa FFS/LFS Primary Bootstrap\r\n\n"
str_reset:
	.stringz	"\r\nresetting..."
	.align	4

; void dispatch(unsigned interactive, unsigned top, unsigned end, int part,
;		unsigned entry)
	.export	dispatch,entry
dispatch:
	; flush data cache / invalidate instruction cache
	ldi	16,%r20		; 16: cache line size
flush:	fdc	0(0,%arg1)	; flush data cache line at arg1
	comb,<	%arg1,%arg2,flush
	fic,m	%r20(0,%arg1)	; flush instruction cache line at arg1, arg1+=16
	sync
	copy	%r0,%r1		; I/O operation is guaranteed to finish
	copy	%r0,%r3		; in eight instructions after sync
	copy	%r0,%r4
	copy	%r0,%r5		; while waiting, clear unused registers
	copy	%r0,%r6		; for future compatibility
	copy	%r0,%r7
	copy	%r0,%r8
	copy	%r0,%r9
	copy	%r0,%r10
	copy	%r0,%r11
	copy	%r0,%r12
	copy	%r0,%r13
	copy	%r0,%r14
	copy	%r0,%r15
	copy	%r0,%r16
	copy	%r0,%r17
	copy	%r0,%r18
	copy	%r0,%r19
	copy	%r0,%r20
	copy	%r0,%r21
	copy	%r0,%r22
	copy	%r0,%r28	; r23-r26: arg3-arg0, r27: dp
	copy	%r0,%r29	; r30: sp
	copy	%r0,%r31
	ldw	-52(%sp),%arg1			; arg4: exec address
	ldo	reboot-$global$(%r27),%rp	; reboot if returns
	bv	%r0(%arg1)			; execute
	ldi	BOOT_IF_VERSION,%arg1

;
;	IODC subroutines
;
PZ_MEM_CONSOLE:		.equ	0x3a0
PZ_MEM_BOOT:		.equ	0x3d0
PZ_MEM_KEYBOARD:	.equ	0x400

DEV_PATH:		.equ	0x00
DEV_LAYERS:		.equ	0x08
DEV_HPA:		.equ	0x20	; hard physical address space
DEV_SPA:		.equ	0x24	; soft physical address space
DEV_IODC_ENTRY:		.equ	0x28
DEV_CLASS:		.equ	0x2c
DEV_CL_DUPLEX:		.equ	0x7	; full-duplex console class

IODC_ENTRY_IO_BOOTIN:		.equ	0
IODC_ENTRY_IO_CONSOLEIN:	.equ	2
IODC_ENTRY_IO_CONSOLEOUT:	.equ	3

; call_iodc
; inputs:
;	%ret0	IODC base in page zero
;	%rp	return address
;	%r29	arg 8
;	%r19	arg 7
;	%r20	arg 6
;	%r21	arg 5
;	%r25	arg 1
; outputs
;	all scratch regs	undefined, unless defined by the IODC call
call_iodc:
	; set common arguments in registers
	addil	L%retbuf-$global$,%r27;%r1
	ldo	R%retbuf-$global$(%r1),%r22	; arg4: return buffer
	ldo	DEV_LAYERS(%ret0),%arg3		; arg3: layer
	ldw	DEV_SPA(%ret0),%arg2		; arg2: spa
	ldw	DEV_HPA(%ret0),%arg0		; arg0: hpa
	; check if narrow or wide mode
	ssm	0,%r1				; get PSW
	bb,<	%r1,4,call_iodc_64		; if W, call in 64bit mode
	ldw	DEV_IODC_ENTRY(%ret0),%r1	; ENTRY_IO address

	; narrow mode
	stw	%r29,-68(%sp)			; arg8: maxsize / lang
	stw	%r19,-64(%sp)			; arg7: size
	stw	%r20,-60(%sp)			; arg6: buf
	stw	%r21,-56(%sp)			; arg5: devaddr / unused
	bv	%r0(%r1)			; call ENTRY_IO
	stw	%r22,-52(%sp)			; arg4: return buffer

call_iodc_64:
	.allow	2.0
	; On PA64 convention, arg0 - arg7 are passed in registers.
	; Parameters are placed in INCREASING order.
	; The argument pointer points at the first stack parameter.
	; stack usage:
	;	64bytes	allocated for register arguments arg0-arg7
	;	 8	arg8 (argument pointer points here)
	;	16	frame marker
	std	%r29,-16-8(%sp)			; arg8: maxsize / lang
;	std	%sp,-8(%sp)			; psp in frame marker
	bv	%r0(%r1)			; call ENTRY_IO
	ldo	-16-8(%sp),%r29			; argument pointer
	.allow

;
; console output
;
; void putch(int)
; void print(const char *string)
	.align	4
	.export putch,entry
	.export print,entry
putch:
	stwm	%arg0,128(%sp)		; fake up a string on the stack
	stb	%r0,-124(%sp)		; (see stack usage below)
	addi,tr	-125,%sp,%arg0		; string address, skip next
print:
	.proc
	.callinfo	frame=128,save_rp,no_unwind
	.entry
	ldo	128(%sp),%sp
	stw	%rp,-128-20(%sp)

	; stack usage:
	;	36byte	IODC buffer (assume %sp was 64byte aligned)
	;	 4	saved reg
	;	88	arguments, frame marker
	;		32bit: 36 (arguments) + 32 (frame marker)
	;		64bit: 72 (arguments) + 16 (frame marker)
prbufsiz:	.equ	36

	; save callee-saves
	stw	%r3,-92(%sp)

	copy	%arg0,%r3

prloop:
	copy	%r0,%r19
	ldi	prbufsiz,%r20
	ldo	-128(%sp),%r1

strloop:
	ldb	0(%r3),%r2
	comb,=	%r2,%r0,endstr
	stbs,ma	%r2,1(0,%r1)
	ldo	1(%r19),%r19
	comb,<>	%r19,%r20,strloop
	ldo	1(%r3),%r3

endstr:
	comb,=,n	%r19,%r0,endpr

	; see IODC 3-51
	; arg0 hpa
	; arg1 option (ENTRY_IO_CONSOLEOUT (3))
	; arg2 spa
	; arg3 ID_addr (pointer to LAYER)
	; arg4 R_addr (pointer to return buffer (64word?))
	; arg5 unused (0)
	; arg6 memaddr (64byte-aligned) string buffer
	; arg7 reqsize
	; arg8 lang (0)
	ldi	PZ_MEM_CONSOLE,%ret0		; IODC base in page zero
	copy	%r0,%r29			; arg8: lang
;	copy	%r19,%r19			; arg7: size
	ldo	-128(%sp),%r20			; arg6: buf
;	copy	%r0,%r21			; arg5: unused
	bl	call_iodc,%rp
	ldi	IODC_ENTRY_IO_CONSOLEOUT,%arg1	; arg1: option
	b,n	prloop

endpr:
	; restore callee-saves
	ldw	-92(%sp),%r3

	; return subroutine
	ldw	-128-20(%sp),%rp
	bv	%r0(%rp)
	.exit
	ldo	-128(%sp),%sp
	.procend

;
; console input
;
; int getch(void)
	.align	4
	.export	getch,entry
getch:
	.proc
	.callinfo	frame=192,save_rp,no_unwind
	.entry
	stw	%rp,-20(%sp)
	ldo	192(%sp),%sp

	; stack usage:
	;	64byte	IODC buffer (assume %sp was 64byte aligned)
	;	40	unused
	;	88	arguments, frame marker
	;		32bit: 36 (arguments) + 32 (frame marker)
	;		64bit: 72 (arguments) + 16 (frame marker)

	; check if console is full or half duplex
	ldw	PZ_MEM_CONSOLE+DEV_CLASS(%r0),%r1	; device class
	extru	%r1,31,4,%r1				; right 4bits are valid
	ldi	PZ_MEM_CONSOLE,%ret0
	comib,=,n	DEV_CL_DUPLEX,%r1,getch_console	; use CONSOLE if full
	ldi	PZ_MEM_KEYBOARD,%ret0			; otherwise KEYBOARD
getch_console:

	; see IODC 3-50
	; arg0 hpa
	; arg1 option (ENTRY_IO_CONSOLEIN (2))
	; arg2 spa
	; arg3 ID_addr (pointer to LAYER)
	; arg4 R_addr (pointer to return buffer (64word?))
	; arg5 unused (0)
	; arg6 memaddr (64byte-aligned, must have 64byte) data buffer
	; arg7 reqsize
	; arg8 lang (0)
;	copy	%rp,%rp				; IODC base in page zero
	copy	%r0,%r29			; arg8: lang
	ldi	1,%r19				; arg7: size (1)
	ldo	-192(%sp),%r20			; arg6: buf
;	copy	%r0,%r21			; arg5: unused
	bl	call_iodc,%rp
	ldi	IODC_ENTRY_IO_CONSOLEIN,%arg1	; arg1: option

	; make return value
	comb,<>	%ret0,%r0,getch_ret		; return -1 on error
	ldi	-1,%ret0
	ldi	1,%r19

	; check if narrow or wide mode
	ssm	0,%r1				; get PSW
	bb,<	%r1,4,getch_64
	addil	L%retbuf-$global$,%r27;%r1
	ldw	R%retbuf-$global$(%r1),%r2	; ret[0]
	comclr,<>	%r19,%r2,%ret0		; return 0 if no char available
getch_retc:
	ldb	-192(%sp),%ret0			;  otherwise return the char

getch_ret:
	; return subroutine
	ldw	-192-20(%sp),%rp
	bv	%r0(%rp)
	.exit
	ldo	-192(%sp),%sp

getch_64:
	.allow	2.0
	ldd	R%retbuf-$global$(%r1),%r2	; ret[0] (64bit)
	b	getch_retc
	cmpclr,*<>	%r19,%r2,%ret0		; return 0 if no char available
	.allow
	.procend

;
; read boot device
;
; void boot_input(void *buf, unsigned len, unsigned pos)
	.align	4
	.export boot_input,entry
boot_input:
	.proc
	.callinfo	frame=128,save_rp,no_unwind
	.entry
	stw	%rp,-20(%sp)
	ldo	128(%sp),%sp

	; stack usage:
	;	40byte	unused (alignment)
	;	88	arguments, frame marker
	;		32bit: 36 (arguments) + 32 (frame marker)
	;		64bit: 72 (arguments) + 16 (frame marker)

	; see IODC 3-46
	; arg0 hpa
	; arg1 option (ENTRY_IO_BOOTIN (0))
	; arg2 spa
	; arg3 ID_addr (pointer to LAYER)
	; arg4 R_addr (pointer to return buffer (64word?))
	; arg5 devaddr
	; arg6 memaddr (64byte-aligned) string buffer
	; arg7 reqsize
	; arg8 maxsize
	ldi	PZ_MEM_BOOT,%ret0		; IODC base in page zero
	copy	%arg1,%r29			; arg8: maxsize
	copy	%arg1,%r19			; arg7: size
	copy	%arg0,%r20			; arg6: buf
	copy	%arg2,%r21			; arg5: devaddr
	bl	call_iodc,%rp
	ldi	IODC_ENTRY_IO_BOOTIN,%arg1	; arg1: option

	; return subroutine
	ldw	-128-20(%sp),%rp
	bv	%r0(%rp)
	.exit
	ldo	-128(%sp),%sp
	.procend

;
;	utilities
;	optimized for size
;

; int strcmp(const char *str1, const char *str2)
	.align	4
	.export		strcmp,entry
strcmp:
	.proc
	.callinfo	frame=0,no_calls
	.entry
	ldbs,ma		1(0,%arg0),%r1
strcmp_loop:
	comb,=		%r1,%r0,strcmp_eos
	ldbs,ma		1(0,%arg1),%r19
	comb,=,n	%r1,%r19,strcmp_loop
	ldbs,ma		1(0,%arg0),%r1
strcmp_eos:
	bv		%r0(%rp)
	.exit
	sub		%r1,%r19,%ret0
	.procend

; void memcpy(void *dst, const void *src, unsigned len)
	.align	4
	.export		memcpy,entry
	.export		memmove,entry
memcpy:
memmove:
	.proc
	.callinfo	no_unwind		; multiple exit points
	.entry
;	copy		%arg0,%ret0		; uncomment this to conform ANSI
	comb,<<,n	%arg0,%arg1,memcpy0	; copy forward or backward?
	add		%arg0,%arg2,%arg0	; dst end address
	add,tr		%arg1,%arg2,%arg1	; src end address, skip next
memcpy_bwd:
	stbs,mb		%r1,-1(0,%arg0)		; write to dst
	addib,uv,n	-1,%arg2,memcpy_bwd	; check loop condition
	ldbs,mb		-1(0,%arg1),%r1		; read from src
	bv,n		%r0(%rp)		; return subroutine
memcpy_fwd:
	stbs,ma		%r1,1(0,%arg0)		; write to dst
memcpy0:
	addib,uv,n	-1,%arg2,memcpy_fwd	; check loop condition
	ldbs,ma		1(0,%arg1),%r1		; read from src
	.exit
	bv,n		%r0(%rp)		; return subroutine
	.procend

;
;	string table
;	placed here to save space
;
	.export	str_seekseq, data
	.export	str_startup, data
	.export	str_bit_firmware, data
	.export	str_crlf, data
	.export	str_space, data
	.export	str_rubout, data
str_seekseq:
	.stringz	"repositioning media...\r\n"
str_bit_firmware:
	.stringz	"bit firmware\r\n"
str_rubout:
	.byte		0x08, 0x20, 0x08, 0x00	; "\b \b"

	.export	str_bootpart, data
str_bootpart:
	.string		"boot partition (a-p, ! to reboot) [a]:"
str_space:
	.stringz	" "
	.export	str_booting_part, data
str_booting_part:
	.string		"\r\nbooting from partition _"
str_crlf:
	.stringz	"\r\n"
	.export	str_warn_2GB, data
str_warn_2GB:
	.stringz	"boot partition exceeds 2GB boundary\r\n"
	.export	str_warn_unused, data
str_warn_unused:
	.stringz	"unused partition\r\n"
	.export	str_nolabel, data
str_nolabel:
	.stringz	"no disklabel\r\n"

	.export	str_filesystem, data
str_filesystem:
	.stringz	"filesystem: _FS\r\n"
	.export	str_nofs, data
str_nofs:
	.stringz	"no filesystem found\r\n"
	.export	str_lookup, data
	.export	str_loading, data
	.export	str_at, data
	.export	str_dddot, data
	.export	str_done, data
str_lookup:
	.stringz	"looking up "
str_loading:
	.stringz	"loading "
str_at:
	.stringz	" at 0x"
str_dddot:
	.stringz	"..."
str_done:
	.stringz	"done\r\n"

	.export	str_boot1, data
	.export	str_boot2, data
	.export	str_boot3, data
str_boot1:
	.stringz	"boot.hp700"
str_boot2:
	.stringz	"boot"
str_boot3:
	.stringz	"usr/mdec/boot"

	.export	str_noboot, data
str_noboot:
	.stringz	"no secondary boot found\r\n"

	.export	str_ukfmt, data
str_ukfmt:
	.stringz	": unknown format -- exec from top\r\n"

	.bss
	.align	64
retbuf:	.block	32*8		; *4 for narrow mode / *8 for wide mode

	.export diskbuf,data
	.align	64
diskbuf:
	.block	2048
