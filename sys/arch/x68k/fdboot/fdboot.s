|	$NetBSD: fdboot.s,v 1.3 1998/01/05 20:52:16 perry Exp $
|
| file: boot.s
| author: chapuni(GBA02750@niftyserve.or.jp)
|.offset	$100000
|_header::	ds.l	8
|_image::

	.global	_header
	.global _image

_header=0x100000		| まずは、このアドレスに
_image=_header+(8*4)		| コアをロードします。

	.global	_bootufs
	.text
top:
		jra	entry0:S
		.ascii	"SHARP/"
		.ascii	"X68030"
		.word	0x8199,0x94e6,0x82ea,0x82bd
		.word	0x8e9e,0x82c9,0x82cd,0x8cbb
		.word	0x8ec0,0x93a6,0x94f0,0x8149
entry0:		jra	entry
| 0x2a
		.space	0x200-0x2a

| 2400(もしくは$3F0000)
| d4 にはすでに SCSI ID が入っている
| ここから jmp まではリロケータブルに書かねばならない。
entry:
	lea	_edata:l,a0		| clear out BSS
	movl	#_end-4,d0		| (must be <= 256 kB)
	subl	#_edata,d0
	lsrl	#2,d0
1:	clrl	a0@+
	dbra	d0,1b

		lea	top:l,a1	| 0x3F0000 にスタックポインタを
		lea	a1@,sp		| 設定する。だから :l が付いてないとダメ
||		moveq	#1,d5		| 512bytes/sec
||		moveq	#8192/512,d3	| 読み込める最大サイズ
||		moveq	#0x40,d2	| いわゆる決め打ち(sd*a のみ)
||		moveq	#0x21,d1	| read
||		moveq	#0xFFFFFFF5,d0	| SCSI
||		trap	#15
||		movel	d4,_ID:l
		moveq	#0xFFFFFF8E,d0	| __BOOTINF
		trap	#15
		movel	d0,d2
		andib	#0x03,d0		| drive # (head=0)
		jbsr	check_fd_format
		moveml	d0-d1,_FDSECMINMAX:l	| min and max sec #
		lslw	#8,d2
		moveq	#0x70,d1
		orw	d2,d1		| PDA*256 + MODE
		movel	d1,_FDMODE:l
		movel	d0,d2		| read position (first sector)
		movel	#8192,d3	| read bytes
		moveq	#0x46,d0	| __B_READ
		trap	#15

| テキストパレットの初期化
		moveq	#7,d3
		moveq	#0,d1
initpalet:
		moveq	#0,d2
		moveq	#0x14,d0		|TPALET2
		trap	#15
		addqw	#1,d1
		movel	#0xFFFF,d2
		moveq	#0x14,d0		|TPALET2
		trap	#15
		addqw	#1,d1
		dbra	d3,initpalet

		jmp	_bootufs:l		| 0x3Fxxxx に飛んでゆく

#include "chkfmt.s"

| int RAW_READ(void *buf, int pos, size_t length);
|	.offset	16
|raw_read_buf:	ds.l	1
|raw_read_pos:	ds.l	1
|raw_read_len:	ds.l	1
|	.text
raw_read_buf= 4+(4*11)
raw_read_pos= raw_read_buf+4
raw_read_len= raw_read_buf+8

	.global _RAW_READ
_RAW_READ:
||		moveml	a2-a6/d2-d7,sp@-
||		movel	_ID:l,d4
||		moveq	#1,d5		| 512bytes/sec
||		movel	sp@(raw_read_buf),a1
||		movel	sp@(raw_read_pos),d2
||		movel	sp@(raw_read_len),d3
||		addl	#511,d3
||		moveq	#9,d1
||		lsrl	d1,d2
||		lsrl	d1,d3
||		addl	#0x40,d2
||		moveq	#0x21,d1
||		moveq	#0xF5-0x100,d0
||		trap	#15
||		moveml	sp@+,a2-a6/d2-d7
||		rts
		moveml	a2-a6/d2-d7,sp@-
		movel	sp@(raw_read_pos),d2
		movel	sp@(raw_read_len),d3
		movel	sp@(raw_read_buf),a1

		|
		| Floppy R/W routine
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
		moveq	#0x46,d0	| __B_READ
		trap	#15

		moveml	sp@+,a2-a6/d2-d7
		moveq	#0,d0		| XXX
		rts

read_half:
		lea	half_msg,a1
boot_error:
		moveq	#0x21,d0	| __B_PRINT
		trap	#15
		moveq	#0xFFFFFFFE,d0	| __IPLERR
		trap	#15

half_msg:
		.ascii	"read half of block\r\n\0"
		.even

	.global	_JISSFT
_JISSFT:
		movel	sp@(4),d1
		moveq	#0xFFFFFFA1,d0
		trap	#15
		rts

	.global	_B_SFTSNS
_B_SFTSNS:
		moveq	#0x02,d0
		trap	#15
		rts

	.global	_B_KEYINP
_B_KEYINP:
		moveq	#0x00,d0
		trap	#15
		rts

	.global _B_PUTC
_B_PUTC:
		movel	sp@(4),d1
		moveq	#0x20,d0
		trap	#15
		rts

	.global _B_PRINT
_B_PRINT:
		movel	sp@(4),a1
		moveq	#0x21,d0
		trap	#15
		rts

	.global _B_COLOR
_B_COLOR:
		movel	sp@(4),d1
		moveq	#0x22,d0
		trap	#15
		rts

	.global _B_LOCATE
_B_LOCATE:
		movel	d2,sp@-
		movel	sp@(8),d1
		movel	sp@(12),d2
		moveq	#0x23,d0
		trap	#15
		movel	sp@+,d2
		rts

	.global _B_CLR_ST
_B_CLR_ST:
		movel	sp@(4),d1
		moveq	#0x2A,d0
		trap	#15
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
	.global	_SYS_STAT
_SYS_STAT:
		movel	sp@(4),d1
		moveq	#0xAC-0x100,d0
		trap	#15
		rts

		.global	_getcpu
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
	beq	Lis68020		| yes, we have 68020
	moveq	#0,d0			| now turn it back off
	movec	d0,cacr			|   before we access any data
	movq	#4,d0			| 68040
	rts
Lis68020:
	movq	#2,d0			| 68020
	rts

||	.comm	_ID,4
	.comm	_FDMODE,4
	.comm	_FDSECMINMAX,8	| +0: (min sector) sector length
				| +1: (min sector) track #
				| +2: (min sector) side
				| +3: (min sector) sector #
				| +4: (max sector) sector length
				| +5: (max sector) track #
				| +6: (max sector) side
				| +7: (max sector) sector #

	.end
