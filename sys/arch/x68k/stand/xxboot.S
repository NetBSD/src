| file: boot.s
| author: chapuni(GBA02750@niftyserve.or.jp)
|         Yasha
|
| $NetBSD: xxboot.S,v 1.2 1998/01/05 20:52:24 perry Exp $

#include "iocscall.h"

|.offset	$100000
|_header::	ds.l	8
|_image::

	.globl	_header
	.globl	_image

_header=0x100000		| まずは、このアドレスに
_image=_header+(8*4)		| コアをロードします。

	.globl	_bootufs
	.text
top:
		bras	entry0
		.ascii	"SHARP/"
		.ascii	"X680x0"
		.word	0x8199,0x94e6,0x82ea,0x82bd
		.word	0x8e9e,0x82c9,0x82cd,0x8cbb
		.word	0x8ec0,0x93a6,0x94f0,0x8149
| 2400(もしくは$3F0000)
| d4 にはすでに SCSI ID が入っている
| ここから jmp まではリロケータブルに書かねばならない。
entry0:
		lea	_edata:l,a1		| clear out BSS
		movl	#_end-1,d0		| (must be <= 256 kB)
		subl	a1,d0
		lsrl	#2,d0
1:		clrl	a1@+
		dbra	d0,1b
		jra	entry

| 0x40 (= LABELOFFSET in <machine/disklabel.h>)
|	Here disklabel exists for floppy disk (276 bytes)
disklabel:
		.space	0x200-0x40

entry:
		lea	top:l,a1	| 0x3F0000 にスタックポインタを
		lea	a1@,sp		| 設定する。だから :l が付いてないとダメ

		movel	d4,_ID:l	| SCSI ID (if booted from SCSI)
		IOCS(__BOOTINF)
		lsll	#8,d0		| clear MSByte
		lsrl	#8,d0		|
		movel	d0,_BOOT_INFO:l
		|
		| 0x80...0x8F		SASI
		| 0x90...0x93		Floppy
		| 0xED0000...0xED3FFE	SRAM
		| others		ROM (SCSI?)
		|
		movel	d0,d1
		clrb	d1
		tstl	d1
		jne	boot_ram_rom
		|
		| SASI or Floppy
		|
		movel	d0,d2
		andib	#0xFC,d0
		cmpib	#0x90,d0
		jne	boot_dev_unsupported	| boot from SASI?
		|
		| Floppy
		|
		moveb	d2,d0
		andib	#0x03,d0		| drive # (head=0)
		jbsr	check_fd_format
		moveml	d0-d1,_FDSECMINMAX:l	| min and max sec #
		lslw	#8,d2
		moveq	#0x70,d1
		orw	d2,d1		| PDA*256 + MODE
		movel	d1,_FDMODE:l
		movel	d0,d2		| read position (first sector)
		movel	#8192,d3	| read bytes
		IOCS(__B_READ)
		jra	boot_read_done

#include "chkfmt.s"

boot_ram_rom:
		movel	d0,d1
		swap	d1
		cmpiw	#0x00ED,d1
		jne	boot_SCSI
		| boot from SRAM?

boot_dev_unsupported:
		lea	pc@(unsupported_msg),a1
boot_error:
		IOCS(__B_PRINT)

	.globl	_IPLERR
_IPLERR:
		IOCS(__IPLERR)

unsupported_msg:
		.asciz	"\r\nunsupported boot device"
		.even

		|
		| ROM boot ... probably from SCSI
		|
boot_SCSI:
		moveq	#1,d5		| 512bytes/sec
		moveq	#8192/512,d3	| 読み込める最大サイズ
		moveq	#0x40,d2	| いわゆる決め打ち(sd*a のみ)
		SCSIIOCS(__S_READ)

boot_read_done:

| テキストパレットの初期化
		moveq	#15,d1
initpalet:
		moveq	#0,d2
		subqw	#1,d2		| movel	#0xFFFF,d2
		IOCS(__TPALET2)
		subqw	#1,d1
		moveq	#0,d2
		IOCS(__TPALET2)
		dbra	d1,initpalet

		jmp	_bootufs:l		| 0x3Fxxxx に飛んでゆく

| int RAW_READ(void *buf, int pos, size_t length);
|	.offset	16
|raw_read_buf:	ds.l	1
|raw_read_pos:	ds.l	1
|raw_read_len:	ds.l	1
|	.text
raw_read_buf=4+(4*11)
raw_read_pos=raw_read_buf+4
raw_read_len=raw_read_buf+8

	.globl	_RAW_READ
_RAW_READ:
		moveml	a2-a6/d2-d7,sp@-
		movel	sp@(raw_read_pos),d2	| pos in byte
		movel	sp@(raw_read_len),d3
		movel	sp@(raw_read_buf),a1

		tstw	_BOOT_INFO	| simple check.  may be incorrect!
		beqs	raw_read_floppy
raw_read_scsi:
		movel	_ID:l,d4
		moveq	#1,d5		| 512bytes/sec
		addl	#511,d3
		moveq	#9,d1
		lsrl	d1,d2
		lsrl	d1,d3
		addl	#0x40,d2
		SCSIIOCS(__S_READ)
		bras	raw_read_end

raw_read_floppy:
		|
		| Floppy read routine
		|

		| convert to seek position

		| sec = raw_read_pos	(d2)
		| sec >>= 7

		lsrl	#7,d2

		| sec >>= (sector length: 0-3)

		lea	_FDSECMINMAX,a0
		moveq	#0,d1
		moveb	a0@,d1		| d1: sector length (0-3)
		lsrl	d1,d2		| d2: pos in sector
		bcss	read_half	| error check

		| trk = sec / (# sectors)
		| sec = sec % (# sectors)

		moveb	a0@(7),d1	| d1: max sector #
		subb	a0@(3),d1	|   - min sector #
		addqb	#1,d1		| d1: # sectors
		divu	d1,d2		| d2: (sec << 16) | track

		| position = (sec length << 24) | (track/2 << 16)
		|		| (track%2 << 8) | (min sec # + sec)

		movel	a0@,d0		| d0: (sec len << 24) | min sec #
		lsrw	#1,d2		| d2: (sec << 16) | (track / 2)
		jcc	1f
		bset	#8,d0		| |= (track % 2) << 8
1:		swap	d2		| d2: ((track / 2) << 16) | sec
		addl	d0,d2		| d2: position

		| read
		movel	_FDMODE:l,d1	| PDA*256 + MODE

		| B_READ (for floppy)
		|  d1.w: PDA x 256 + MODE
		|	PDA: 0x90 (drive 0) ... 0x93 (drive 3)
		|	MODE:	bit6: MFM
		|		bit5: retry
		|		bit4: seek
		|  d2.l: position
		|	bit31-24: sector length (0: 128, 1: 256, 2: 512, 3: 1K)
		|	bit23-16: track # (0-79)
		|	bit15-08: side (0 or 1)
		|	bit07-00: sector # (1-)
		|  d3.l: read bytes
		|  a1:   read address
		IOCS(__B_READ)

raw_read_end:
		moveml	sp@+,a2-a6/d2-d7
		moveq	#0,d0		| XXX
		rts

read_half:	lea	pc@(half_msg),a1
		jra	boot_error

half_msg:	.asciz	"\r\nread half of block"
		.even

	.globl	_JISSFT
_JISSFT:
		movel	sp@(4),d1
		IOCS(__JISSFT)
		rts

	.globl	_B_SFTSNS
_B_SFTSNS:
		IOCS(__B_SFTSNS)
		rts

	.globl	_B_KEYINP
_B_KEYINP:
		IOCS(__B_KEYINP)
		rts

	.globl	_B_PUTC
_B_PUTC:
		movel	sp@(4),d1
		IOCS(__B_PUTC)
		rts

	.globl	_B_PRINT
_B_PRINT:
		movel	sp@(4),a1
		IOCS(__B_PRINT)
		rts

	.globl	_B_COLOR
_B_COLOR:
		movel	sp@(4),d1
		IOCS(__B_COLOR)
		rts

	.globl	_B_LOCATE
_B_LOCATE:
		movel	d2,sp@-
		movel	sp@(8),d1
		movel	sp@(12),d2
		IOCS(__B_LOCATE)
		movel	sp@+,d2
		rts

	.globl	_B_CLR_ST
_B_CLR_ST:
		movel	sp@(4),d1
		IOCS(__B_CLR_ST)
		rts

|IOCS SYS_STAT   $AC
|
|入力 d1.l=0 のとき d0.l に以下の値
|        bit0〜7         0:68000, 1:68010, 2:68020, 3:68030
|        bit14           0:MMUなし, 1:MMUあり
|        bit15           0:FPCPなし, 1:FPCPあり
|        bit16〜31       クロックスピード
|
| X68000 の IOCS には $AC はないのでエラーになりますが、Xellent30 の場合は
|起動時に SRAM で組み込んでくれます。
	.globl	_SYS_STAT
_SYS_STAT:
		movel	sp@(4),d1
		IOCS(__SYS_STAT)
		rts

		.globl	_getcpu
_getcpu:
	movl	#0x200,d0		| data freeze bit
	movc	d0,cacr			|   only exists on 68030
	movc	cacr,d0			| read it back
	tstl	d0			| zero?
	jeq	Lnot68030		| yes, we have 68020/68040
	movq	#3,d0			| 68030
	rts
Lnot68030:
	bset	#31,d0			| data cache enable bit
	movc	d0,cacr			|   only exists on 68040
	movc	cacr,d0			| read it back
	tstl	d0			| zero?
	jeq	Lis68020		| yes, we have 68020
	moveq	#0,d0			| now turn it back off
	movec	d0,cacr			|   before we access any data
	movq	#4,d0			| 68040
	rts
Lis68020:
	movq	#2,d0			| 68020
	rts

|
| void memcpy(void *dst, const void *src, size_t count);
| void memmove(void *dst, const void *src, size_t count);
|
|	small and slow memcpy...
|	THIS FUNCTION DOES NOT CONFORM THE ANSI STANDARD
|
	.globl	_memcpy, _memmove
_memcpy:
_memmove:
	lea	sp@(12),a1
	movel	a1@,d1		| count
	jeq	Lmcpret
	moveal	a1@-,a0		| src
	moveal	a1@-,a1		| dest
	cmpl	a1,a0
	jcc	Lmcpfw
	| copy backward
	addal	d1,a0
	addal	d1,a1
1:	moveb	a0@-,a1@-
	subql	#1,d1
	jne	1b
	jra	Lmcpret
Lmcpfw:	| copy forward
1:	moveb	a0@+,a1@+
	subql	#1,d1
	jne	1b
Lmcpret:
|	movel	sp@(8),d0	| uncomment this to conform ANSI
	rts


	.comm	_ID,4		| SCSI ID
	.comm	_BOOT_INFO,4	| result of IOCS(__BOOTINF)
	.comm	_FDMODE,4	| Floppy access mode: PDA x 256 + MODE
	.comm	_FDSECMINMAX,8	| +0: (min sector) sector length
				| +1: (min sector) track #
				| +2: (min sector) side
				| +3: (min sector) sector #
				| +4: (max sector) sector length
				| +5: (max sector) track #
				| +6: (max sector) side
				| +7: (max sector) sector #

|	.end
