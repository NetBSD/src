| file: boot.s
| author: chapuni(GBA02750@niftyserve.or.jp)
|         Yasha
|
| $NetBSD: xxboot.S,v 1.2 2001/06/12 16:57:28 minoura Exp $

#include <machine/asm.h>
#include "iocscall.h"

#define BASEOFF		0x8000
#define BASEPTR_A	(TEXTADDR+BASEOFF)
#define BASEPTR_R	%pc@(top+BASEOFF:W)

#define SRAM		0x00ED0000	/* SRAM stat addr */
#define SRAM_MEMSZ	(SRAM + 8)	/* (L) size of main memory */
#define MINMEM		0x00400000	/* at least 4MB required */

#define BOOT_ERROR1(s)	jbsr boot_error; .asciz s; .even

	.globl	_C_LABEL(header)
	.globl	_C_LABEL(image)

_C_LABEL(header)=0x100000		| まずは、このアドレスに
_C_LABEL(image)=_C_LABEL(header)+(8*4)		| コアをロードします。

#ifdef BOOT_DEBUG
	.globl	_C_LABEL(startregs)
_C_LABEL(startregs)=0xb000		| chosen arbitrarily...
#endif

	.globl	_C_LABEL(bootufs)
	.text
ASENTRY_NOPROFILE(top)
		bras	entry0
		.ascii	"SHARP/"
		.ascii	"X680x0"
		.word	0x8199,0x94e6,0x82ea,0x82bd
		.word	0x8e9e,0x82c9,0x82cd,0x8cbb
		.word	0x8ec0,0x93a6,0x94f0,0x8149
| 0x2000 (FD), 0x2400 (SASI/SCSI) (もしくは 0x3F0000)
| %d4 にはすでに SCSI ID が入っている
| ここから jmp まではリロケータブルに書かねばならない。
entry0:
#ifdef BOOT_DEBUG
		moveml	%d0-%d7/%a0-%a7,_C_LABEL(startregs)
#endif
		lea	BASEPTR_A:l,%a5		| set base ptr
#define _RELOC(adr)	%a5@(((adr)-(BASEPTR_A&0xffff)):W)
#define ASRELOC(var)	_RELOC(_ASM_LABEL(var))
#define RELOC(var)	_RELOC(_C_LABEL(var))

		lea	RELOC(edata),%a1	| bss start
		movew	#_end-1,%d0		| bss end (low word only)

#ifndef BOOT_DEBUG		/* not enough space -- moved below */
		| check memory size (high word)
		cmpiw	#MINMEM/0x10000,SRAM_MEMSZ
#endif
		bra	entry

| 0x40 (= LABELOFFSET in <machine/disklabel.h>)
|	Here disklabel exists for floppy disk (276 bytes)
disklabel:
		.space	300

#ifdef TITLE_IMAGE
		.word	img_title-_ASM_LABEL(top)
#else
		.word	0
#endif
entry:
#ifdef BOOT_DEBUG
		| check memory size (high word)
		cmpiw	#MINMEM/0x10000,SRAM_MEMSZ
#endif
		jcc	memok		| (continued from above) RAM >= 4MB
		BOOT_ERROR1("4MB RAM required")
memok:
		| clear out bss  (must be <= 64KB)
		subw	%a1,%d0
clrbss:		clrb	%a1@+
		dbra	%d0,clrbss

		| set system stack
		lea	ASRELOC(top),%a1 | set stack pointer to 0x003F0000
		lea	%a1@,%sp	| %a1 will be used later for IOCS calls

		| we use 68020 instructions, and check MPU beforehand
		|
		| here %d1.w = -1, and the above "subw %a1,%d0" = 0x9049, and
		|	if MPU <= 010	loads 0x49,
		|	if MPU >= 020	loads 0x90.
		| This is a move, not a tst instruction
		| because pc-relative tsts are not availble on 000/010.
chkmpu:		moveb	%pc@(clrbss-chkmpu-2:B,%d0:W:2),%d0	| 103B 02xx
		jmi	mpuok		| MC68020 or later
		BOOT_ERROR1("MPU 68000?")
mpuok:		| XXX check for MMU?

		movel	%d4,RELOC(ID)	| SCSI ID (if booted from SCSI)
		IOCS(__BOOTINF)
		movel	%d0,RELOC(BOOT_INFO)
		lsll	#8,%d0		| clear MSByte
		lsrl	#8,%d0		|

		|
		| 0x80...0x8F		SASI
		| 0x90...0x93		Floppy
		| 0xED0000...0xED3FFE	SRAM
		| others		ROM (SCSI?)
		|
		movel	%d0,%d1
		clrb	%d1
		tstl	%d1
		jne	boot_ram_rom
		|
		| SASI or Floppy
		|
		movel	%d0,%d2
		andib	#0xFC,%d0
		cmpib	#0x90,%d0
		jne	boot_dev_unsupported	| boot from SASI?
		|
		| Floppy
		|
		moveb	%d2,%d0
		andib	#0x03,%d0		| drive # (head=0)
		jbsr	check_fd_format
		moveml	%d0-%d1,RELOC(FDSECMINMAX)	| min and max sec #
		lslw	#8,%d2
		moveq	#0x70,%d1
		orw	%d2,%d1		| PDA*256 + MODE
		movel	%d1,RELOC(FDMODE)
		movel	%d0,%d2		| read position (first sector)
		movel	#8192,%d3	| read bytes
		IOCS(__B_READ)
		jra	boot_read_done

#include "chkfmt.s"

boot_ram_rom:
		movel	%d0,%d1
		swap	%d1
		cmpiw	#0x00ED,%d1
		jne	boot_SCSI
		| boot from SRAM?

boot_dev_unsupported:
		BOOT_ERROR1("unsupported boot device")

|
| volatile void BOOT_ERROR(const char *msg);
|	print error message, wait for key press and reboot
|
booterr_msg:	.ascii	"\r\n\n"
		.ascii	BOOT
		.asciz	": "
reboot_msg:	.asciz	"\r\n[Hit key to reboot]"
		.even

ENTRY_NOPROFILE(BOOT_ERROR)
		addql	#4,%sp

boot_error:	lea	%pc@(booterr_msg),%a1
		IOCS(__B_PRINT)
		moveal	%sp@+,%a1
		IOCS(__B_PRINT)
		lea	%pc@(reboot_msg),%a1
		IOCS(__B_PRINT)

		| wait for a key press (or release of a modifier)
		IOCS(__B_KEYINP)

		| issue software reset
		trap	#10
		| NOTREACHED


		|
		| ROM boot ... probably from SCSI
		|
boot_SCSI:
#ifdef SCSI_ADHOC_BOOTPART
		|
		| Find out boot partition in an ad hoc manner.
		|

		| get block length of the SCSI disk
		SCSIIOCS(__S_READCAP)	| using buffer at %a1
		tstl	%d0
		jeq	1f
		BOOT_ERROR1("READCAP failed")
1:		moveq	#0,%d5
		moveb	%a1@(6),%d5	| 1: 256, 2: 512, 4: 1024
		lsrb	#1,%d5		| 0: 256, 1: 512, 2: 1024
		movel	%d5,RELOC(SCSI_BLKLEN)

		| find out the start position of the boot partition
		| XXX VERY AD HOC
		|
		| SCSI IPLs (genuine and SxSI):
		|	pass read pos (in kilobytes) in %d2
		|	partition table on the memory is destroyed
		| BOOT MENU Ver.2.22:
		|	passes partition table entry address in %a0
		|	%d2 is cleared to zero
		| No other IPL is supported.  XXX FIXME
		tstl	%d2
		jne	1f
		| no information in %d2 -- probably from BOOT MENU
		| %a0 points the partiion table entry
		movel	%a0@(0x0008),%d2 | in KByte
1:		lsll	#8,%d2		| clear MSByte
		lsrl	#6,%d2		|
		lsrl	%d5,%d2		| in sector
		movel	%d2,RELOC(SCSI_PARTTOP)

		| read entire boot
		moveq	#8192/256,%d3	| size is 8KB
		lsrl	%d5,%d3		| in sector
		jbsr	scsiread	| read at  %a1
#else
		moveq	#1,%d5		| 512bytes/sec
		moveq	#8192/512,%d3	| 読み込める最大サイズ
		moveq	#0x40,%d2	| いわゆる決め打ち(sd*a のみ)
		SCSIIOCS(__S_READ)
#endif

boot_read_done:

| テキストパレットの初期化
		moveq	#15,%d1
initpalet:
		moveq	#0,%d2
		subqw	#1,%d2		| movel	#0xFFFF,%d2
		IOCS(__TPALET2)
		subqw	#1,%d1
		moveq	#0,%d2
		IOCS(__TPALET2)
		dbra	%d1,initpalet

		jsr	RELOC(printtitle)
		jmp	RELOC(bootufs)		| 0x3Fxxxx に飛んでゆく

read_error:	BOOT_ERROR1("read error")

#undef _RELOC	/* base register  a5  is no longer available */
#undef ASRELOC
#undef RELOC

|
|	read SCSI
|
|	input:	%d2.l: pos in sector
|		%d3.l: len in sector
|		%d4: target SCSI ID
|		%d5: sector length (0: 256, 1: 512, 2: 1024)
|		%a1: buffer address
|	destroy:
|		%d0, %d1, %a1
|
scsiread:
		moveml	%d2-%d3/%d6-%d7/%a2,%sp@-
		| if (pos >= 0x200000 || (len > 255 && pos + len >= 0x200000))
		|	use READEXT
		| else
		|	use READ
		moveq	#0x20,%d0
		swap	%d0		| %d0.l = 0x00200000
		moveq	#0,%d6
		subqb	#1,%d6		| %d6.l = 255
		moveq	#8,%d7
		addb	%d5,%d7		| %d7.b = (sector length: 0-2) + 8
		cmpl	%d0,%d2
		jcc	scsiread_ext
		moveq	#__S_READ,%d1
		cmpl	%d3,%d6
		jcc	scsiread_noext
		subl	%d2,%d0		| %d0.0 = 0x200000 - pos
		cmpl	%d0,%d3		|	<= len
		jcs	scsiread_noext	| no

scsiread_ext:	| use READEXT
		extw	%d6		| %d6.l = 65535
		moveq	#__S_READEXT,%d1

scsiread_noext:	| use READ
loop_scsiread:
		| %d1: SCSI IOCS call #
		| %d6: max sector count at a time
		movel	%d3,%a2		| save original len in %a2
		cmpl	%d3,%d6
		jcc	1f
		movel	%d6,%d3
1:		IOCS(__SCSIDRV)		| SCSIIOCS(%d1)
		tstl	%d0
		jne	read_error
		movel	%d3,%d0		| addr += read count << (8 + sec len)
		asll	%d7,%d0
		addl	%d0,%a1
		exg	%d3,%a2		| restore original len to %d3
		addl	%a2,%d2		| pos += read count
		subl	%a2,%d3		| len -= read count
		jne	loop_scsiread
		moveml	%sp@+,%d2-%d3/%d6-%d7/%a2
		rts

|
|	The former part must reside in the first 1KB.
|
		.globl	first_kbyte
first_kbyte:
|--------------------------------------------------------------------------
|
|	The latter text+data part is not accessible at the first boot time.
|	PC-relative can be used from here.
|

| int badbaddr __P((caddr_t adr));
|	check if the given address is valid for byte read
|	return: 0: valid, 1: not valid

ENTRY_NOPROFILE(badbaddr)
		lea	0x0008:W,%a1		| MPU Bus Error vector
		moveq	#1,%d0
		lea	%pc@(badr1),%a0
		movew	%sr,%sp@-
		oriw	#0x0700,%sr		| keep out interrupts
		movel	%a1@,%sp@-
		movel	%a0,%a1@		| set bus error vector
		movel	%sp,%d1			| save %sp
		moveal	%sp@(10),%a0
		tstb	%a0@			| try read...
		moveq	#0,%d0			| this is skipped on bus error
badr1:		moveal	%d1,%sp			| restore %sp
		movel	%sp@+,%a1@
		movew	%sp@+,%sr
		rts

| void RAW_READ __P((void *buf, u_int32_t blkpos, size_t bytelen));
| inputs:
|	buf:	 input buffer address
|	blkpos:	 read start position in the partition in 512byte-blocks
|	bytelen: read length in bytes

Lraw_read_buf=4+(4*11)
Lraw_read_pos_=Lraw_read_buf+4
Lraw_read_len=Lraw_read_buf+8

#ifdef SCSI_ADHOC_BOOTPART
|	RAW_READ of physical disk
ENTRY_NOPROFILE(RAW_READ0)
		moveq	#0,%d0
		jra	raw_read1
#endif
ENTRY_NOPROFILE(RAW_READ)
#ifdef SCSI_ADHOC_BOOTPART
		movel	_C_LABEL(SCSI_PARTTOP),%d0
raw_read1:
#endif
		moveml	%d2-%d7/%a2-%a6,%sp@-
		moveml	%sp@(Lraw_read_buf),%d1-%d3
		movel	%d1,%a1
		| %d2.l:	pos in 512byte-blocks
		| %d3.l:	length in bytes
		| %a1 (=%d1):	buffer address

		lea	BASEPTR_R,%a5	| set base ptr
#define _RELOC(adr)	%a5@(((adr)-(BASEPTR_A&0xffff)):W)
#define ASRELOC(var)	_RELOC(_ASM_LABEL(var))
#define RELOC(var)	_RELOC(_C_LABEL(var))

		tstb	_RELOC(_C_LABEL(BOOT_INFO)+1) | simple check.  may be incorrect!
		beqs	raw_read_floppy

raw_read_scsi:
		movel	RELOC(ID),%d4	| SCSI ID
#ifdef SCSI_ADHOC_BOOTPART
		movel	RELOC(SCSI_BLKLEN),%d5	| sector size: 0-2
		| XXX length must be sector aligned
		lsrl	#8,%d3		| size in 256byte-blocks
		lsrl	%d5,%d3		| size in sector
		bcss	read_half	| minimal error check
		lsll	#1,%d2		| X flag and %d2: pos in 256byte-blocks
		roxrl	%d5,%d2		| pos in sector
		addl	%d0,%d2		| physical pos in sector
#else
		moveq	#1,%d5		| 512bytes/sec
		moveq	#9,%d0		| shift count
		addl	#511,%d3
		lsrl	%d0,%d3
		bcss	read_half	| minimal error check

		addl	#0x40,%d2	| 'a' partition starts here
#endif
|		jcc	1f
|		BOOT_ERROR1("out of seek")	| pos exceeds 32bit
|1:
		jbsr	scsiread
		bras	raw_read_end

raw_read_floppy:
		|
		| Floppy read routine
		|

		| convert to seek position

		asll	#2,%d2		| size in 128byte-blocks

		| sec = raw_read_pos	(%d2)
		| sec >>= 7 + (sector length: 0-3)

		lea	RELOC(FDSECMINMAX),%a0
		moveq	#0,%d1
		moveb	%a0@,%d1	| %d1: sector length (0-3)
		lsrl	%d1,%d2		| %d2: pos in sector
		bcss	read_half	| error check

		| trk = sec / (# sectors)
		| sec = sec % (# sectors)

		moveb	%a0@(7),%d1	| %d1: max sector #
		subb	%a0@(3),%d1	|   - min sector #
		addqb	#1,%d1		| %d1: # sectors
		divu	%d1,%d2		| %d2: (sec << 16) | track

		| position = (sec length << 24) | (track/2 << 16)
		|		| (track%2 << 8) | (min sec # + sec)

		movel	%a0@,%d0	| %d0: (sec len << 24) | min sec #
		lsrw	#1,%d2		| %d2: (sec << 16) | (track / 2)
		jcc	1f
		bset	#8,%d0		| |= (track % 2) << 8
1:		swap	%d2		| %d2: ((track / 2) << 16) | sec
		addl	%d0,%d2		| %d2: position

		| read
		movel	RELOC(FDMODE),%d1	| PDA*256 + MODE

		| B_READ (for floppy)
		|  %d1.w: PDA x 256 + MODE
		|	PDA: 0x90 (drive 0) ... 0x93 (drive 3)
		|	MODE:	bit6: MFM
		|		bit5: retry
		|		bit4: seek
		|  %d2.l: position
		|	bit31-24: sector length (0: 128, 1: 256, 2: 512, 3: 1K)
		|	bit23-16: track # (0-79)
		|	bit15-08: side (0 or 1)
		|	bit07-00: sector # (1-)
		|  %d3.l: read bytes
		|  %a1:   read address
		| return:
		|  %d0:	bit 31-24	ST0
		|	bit 23-16	ST1
		|	bit 15- 8	ST2
		|	bit  7- 0	C
		|	-1 on parameter error
		| destroy: %d0, %d2, %d3, %a1
		IOCS(__B_READ)
		andil	#0xf8ffff00,%d0		| check status (must be zero)
		jne	read_error

raw_read_end:
		moveml	%sp@+,%a2-%a6/%d2-%d7
		rts
#undef _RELOC	/* base register  %a5  is no longer available */
#undef ASRELOC
#undef RELOC

read_half:	BOOT_ERROR1("read half of block")

#if 0
ENTRY_NOPROFILE(JISSFT)
		movel	%sp@(4),%d1
		IOCS(__JISSFT)
		rts
#endif

ENTRY_NOPROFILE(B_SFTSNS)
		IOCS(__B_SFTSNS)
		rts

ENTRY_NOPROFILE(B_KEYINP)
		IOCS(__B_KEYINP)
		rts

ENTRY_NOPROFILE(B_PUTC)
		movel	%sp@(4),%d1
		IOCS(__B_PUTC)
		rts

ENTRY_NOPROFILE(B_PRINT)
		movel	%sp@(4),%a1
		IOCS(__B_PRINT)
		rts

ENTRY_NOPROFILE(B_COLOR)
		movel	%sp@(4),%d1
		IOCS(__B_COLOR)
		rts

ENTRY_NOPROFILE(B_LOCATE)
		movel	%d2,%sp@-
		movel	%sp@(8),%d1
		movel	%sp@(12),%d2
		IOCS(__B_LOCATE)
		movel	%sp@+,%d2
		rts

ENTRY_NOPROFILE(B_CLR_ST)
		movel	%sp@(4),%d1
		IOCS(__B_CLR_ST)
		rts

ENTRY_NOPROFILE(printtitle)
		lea	%pc@(msg_title),%a1
		IOCS(__B_PRINT)
#ifndef TITLE_IMAGE
		rts
#endif

#ifdef TITLE_IMAGE
# define IMGXPOS	17
# define IMGYPOS	1
# define IMGWIDTH	56
# define IMGHEIGHT	52
|
|	display title image
|
put_image:	link	%a6,#-(IMGWIDTH*IMGHEIGHT/8*2+8) | make local buffer
		moveal	%sp,%a1				| buffer (plane #0) top
		moveml	%d2-%d7/%a2-%a4,%sp@-

		lea	%a1@(IMGWIDTH*IMGHEIGHT/8+4),%a4 | buffer (plane #1) top
		movel	#IMGWIDTH*0x10000+IMGHEIGHT,%a1@ | image size
		movel	%a1@+,%a4@+

		lea	%pc@(img_title),%a2
		moveaw	%a2@+,%a3		| only the lower word is valid

1:		tstb	%a2@+			| skip human readable comment
		jne	1b
		lea	%a2@(4),%a0		| image data here

		moveq	#0,%d2			| (b) now have 0bit
		moveq	#0,%d3			| (w) input bit buffer

		moveq	#16,%d4			| (b) output bit count
		moveq	#IMGWIDTH*IMGHEIGHT/16+0xffffff00,%d7

		| %d2.b: # bits in input buffer
		| %d3.w: input buffer
		| %d4.b: # bits in output buffer
		| %d5.w: output buffer for plane 0
		| %d6.w: output buffer for plane 1
		| %d7.b: size count
		| %a0: input data pointer

Lrunlen:	| a run entry:
		|	value	 (2bit)
		|	length-1 (3bit)
		subqb	#5,%d2		| have enough bits?
		jcc	Lhave5b		| yes
		| fill byte
		cmpaw	%a0,%a3		| the register order is significant
					| for word comparison
		jne	1f
		lea	%pc@(disklabel),%a0 | continue to disklabel area?
		cmpiw	#0x696d,%a0@+	| check magic number
		jne	Lrxabort	| no, abort (no image)
1:		moveb	%a0@+,%d0
		addqb	#8,%d2		| %d2: 3: have 0bit, ..., 7: have 4bit
		lslw	#8,%d0		| clear unused bits
		lsrw	%d2,%d0
		orw	%d0,%d3

Lhave5b:	| field#  19 20 21 22 23 24 25 26 27 28 29 30 31
		| %d3: ... v  v  l  l  l  ?  ?  ?  ?  ?  ?  ?  0
		bfexts	%d3{#19,#2},%d0	| value
		bfextu	%d3{#21,#3},%d1	| length - 1
		lslw	#5,%d3

Lrxloop:	lslw	#2,%d6
		bfins	%d0,%d6{#30,#2}
		roxrw	#1,%d6
		roxlw	#1,%d5
		subqb	#1,%d4
		jne	1f
		moveq	#16,%d4
		movew	%d5,%a1@+
		movew	%d6,%a4@+
		subqb	#1,%d7		| end?
1:		dbeq	%d1,Lrxloop	| fall down if end
		jne	Lrunlen		| fall down if end

		moveq	#1,%d1
		movew	%a2@+,%d2
		IOCS(__TPALET2)
		moveq	#2,%d1
		movew	%a2@,%d2
		IOCS(__TPALET2)

|		moveq	#0x02,%d1	| plane 1 (already have it)
		IOCS(__TCOLOR)
		moveq	#IMGXPOS,%d1
		moveq	#IMGYPOS,%d2
		IOCS(__TEXTPUT)
		moveq	#0x01,%d1	| plane 0
		IOCS(__TCOLOR)
		moveq	#IMGXPOS,%d1
		lea	%a6@(-(IMGWIDTH*IMGHEIGHT/8*2+8)),%a1
		IOCS(__TEXTPUT)

Lrxabort:	moveml	%sp@+,%d2-%d7/%a2-%a4
		unlk	%a6
		rts
#endif

#if 0
|IOCS SYS_STAT   $AC
|
|入力 %d1.l=0 のとき %d0.l に以下の値
|        bit0〜7         0:68000, 1:68010, 2:68020, 3:68030
|        bit14           0:MMUなし, 1:MMUあり
|        bit15           0:FPCPなし, 1:FPCPあり
|        bit16〜31       クロックスピード
|
| X68000 の IOCS には $AC はないのでエラーになりますが、Xellent30 の場合は
|起動時に SRAM で組み込んでくれます。
	.globl	_SYS_STAT
_SYS_STAT:
		movel	%sp@(4),%d1
		IOCS(__SYS_STAT)
		rts

		.globl	_getcpu
_getcpu:
	movl	#0x200,%d0		| data freeze bit
	movc	%d0,%cacr		|   only exists on 68030
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lnot68030		| yes, we have 68020/68040
	movq	#3,%d0			| 68030
	rts
Lnot68030:
	bset	#31,%d0			| data cache enable bit
	movc	%d0,%cacr		|   only exists on 68040
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lis68020		| yes, we have 68020
	moveq	#0,%d0			| now turn it back off
	movec	%d0,%cacr		|   before we access any data
	movq	#4,%d0			| 68040
	rts
Lis68020:
	movq	#2,%d0			| 68020
	rts
#endif

|
| void memcpy(void *dst, const void *src, size_t count);
| void memmove(void *dst, const void *src, size_t count);
|
|	small and slow memcpy...
|	THIS FUNCTION DOES NOT CONFORM THE ANSI STANDARD
|
ENTRY_NOPROFILE(memcpy)
ENTRY_NOPROFILE(memmove)
	lea	%sp@(12),%a1
	movel	%a1@,%d1	| count
	jeq	Lmcpret
	moveal	%a1@-,%a0	| src
	moveal	%a1@-,%a1	| dest
	cmpl	%a1,%a0
	jcc	Lmcpfw
	| copy backward
	addal	%d1,%a0
	addal	%d1,%a1
1:	moveb	%a0@-,%a1@-
	subql	#1,%d1
	jne	1b
	jra	Lmcpret
Lmcpfw:	| copy forward
1:	moveb	%a0@+,%a1@+
	subql	#1,%d1
	jne	1b
Lmcpret:
|	movel	%sp@(8),%d0	| uncomment this to conform ANSI
	rts

|
|	Copy and exec kernel
|
#include "../common/execkern.S"

|
|	Title message
|
msg_title:
	.byte	0x1a			| clear screen and cursor home
	.ascii	"\033[3;4H"		| move cursor to 4, 3
	.ascii	"\033[37m"		| bold
	.ascii	"NetBSD boot"
	.ascii	"\033[m"		| normal
	.ascii	" ("
	.ascii	BOOT
	.ascii	" "
	.ascii	BOOT_VERS
#ifdef BOOT_DEBUG
	.ascii	"/DEBUG"
#endif
#ifndef TITLE_IMAGE
	.ascii	"/no title image"
#endif
	.ascii	")"
	.asciz	"\r\n\n"
	.even

#ifdef TITLE_IMAGE
|
|	Title Image
|
img_title:
	.word	imgend1
#ifdef BOOT_DEBUG
	.space	200		| no space for BSD Daemon...
#else
	| compressed image data

	| BSD Daemon Image, used for NetBSD boot by permission.
	| (Thanks for permission!)

	| BSD Daemon Copyright 1988 by Marshall Kirk McKusick.
	| All Rights Reserved.
	|
	| Permission to use the daemon may be obtained from:
	|     Marshall Kirk McKusick
	|     1614 Oxford St
	|     Berkeley, CA 94709-1608
	|     USA
	| or via email at mckusick@mckusick.com

	| Human readable copyright notice to be found with strings(1).
	| Terminate with a nul character.

	.asciz	"\nBSD Daemon Copyright 1988 by Marshall Kirk McKusick."

	.word	0x5295,0xAD6A		| palet1, palet2
	.word	0x39c3,0x0c1c,0xd039,0xce71,0xa327,0x3830,0x739c,0xe146
	.word	0x6073,0x8227,0x39ce,0x0865,0x0738,0x2184,0x1ce7,0x3233
	.word	0x049c,0xe88a,0x0e73,0x9618,0x8271,0x0486,0x6646,0x2093
	.word	0x9ce4,0xcc12,0x1461,0x1622,0x3104,0x9ce7,0x2653,0x10e2
	.word	0x4158,0x91a8,0x24e7,0x38d1,0x9807,0x00c8,0x7216,0x6a29
	.word	0x39ce,0x4440,0x1906,0x4190,0xe031,0xe452,0x739c,0x9006
	.word	0x8180,0xec18,0x0221,0xc8a0,0xe739,0xb60c,0x0380,0xdc35
	.word	0x149c,0xe724,0x01b0,0x601d,0x062c,0x939c,0xe6c8,0x700c
	.word	0x0b60,0xcc58,0x739c,0xf81e,0x09a0,0x6a49,0x39ce,0x6803
	.word	0x00c0,0x701c,0x0740,0xd4d0,0x739c,0xd806,0x0100,0xe038
	.word	0x0e81,0xb924,0xe726,0x0d0c,0x20e2,0xc839,0x249c,0xe4c4
	.word	0x0ba4,0x0b42,0xc811,0xc920,0xe73c,0x070e,0x0383,0x44b5
	.word	0x241c,0xe3c8,0x6228,0x701b,0x023f,0x8924,0xe726,0x0501
	.word	0xc301,0xc8d3,0xfc49,0x0739,0xa01c,0x1219,0x0fc9,0x8924
	.word	0xe716,0x2028,0x64a8,0xc441,0xc147,0x3249,0xce4c,0x0e38
	.word	0x2a24,0x58ea,0x4939,0xcd00,0xe208,0x860a,0x8bea,0x0c39
	.word	0xce09,0x0608,0x8e10,0x3449,0x1aa2,0x4e73,0x9d3a,0x4111
	.word	0xfc45,0x2739,0xcb0c,0xa308,0x0205,0x0fea,0x2939,0xce4d
	.word	0x2030,0x4823,0xfdc1,0x4739,0xc68c,0x4100,0x4490,0x0c8f
	.word	0x70cc,0x5473,0x9ce0,0x8a22,0x1866,0x078a,0x4e72,0x6c7b
	.word	0x0260,0x9e03,0xc148,0x780f,0xc090,0xc970,0x04c5,0x5c06
	.word	0x45a1,0x641a,0x0e47,0xa1ec,0x7903,0xd03c,0x8310,0x0643
	.word	0x91e0,0x3926,0x47b1,0xe408,0x3e05,0x9044,0x190e,0x4780
	.word	0xe49a,0x0c38,0x2c1c,0x390e,0x8100,0x683a,0x2689,0xa0c3
	.word	0x00d0,0x722d,0x0780,0xc8b4,0x2d8f,0x25d0,0x721c,0x8720
	.word	0xc872,0x1c8b,0xa5d8,0xb25d,0x0721,0xd032,0x0c87,0x21c8
	.word	0xb22c,0x8141,0xd872,0x5c86,0x1916,0xc190,0xe43a,0x0e81
	.word	0x0403,0x2188,0x740d,0x1341,0xc8f6,0x0d8f,0x22d0,0x220c
	.word	0x8300,0x9032,0x0c97,0x300e,0x49a0,0x6478,0x0645,0x91e8
	.word	0x1816,0x0190,0x647b,0x1649,0x90e8,0x380e,0x4180,0x645a
	.word	0x0e83,0x90e4,0x3b1e,0xc981,0x6c79,0x07c5,0xd0f8,0x2939
	.word	0xc523,0x8946,0x820d,0x0c45,0x070f,0x6192,0xde08,0x3832
	.word	0x0572,0x083c,0x3920,0x721a,0x9ac8,0x8868,0xf889,0xc160
	.word	0x4377,0x8a84,0x88ee,0xd891,0xc2b0,0x2200,0xb2a2,0xa1c3
	.word	0x10dc,0x1564,0x1ce3,0x79c6,0x9022,0x2258,0x1073,0x9ce7
	.word	0x2a20,0x701c
	.even
	.space	IMAGE_EXTRA	| allow larger image
	.even
#endif
imgend1:
#endif	/* TITLE_IMAGE */

|
|	global variables
|
	BSS(ID,4)		| SCSI ID
	BSS(BOOT_INFO,4)	| result of IOCS(__BOOTINF)
	BSS(FDMODE,4)		| Floppy access mode: PDA x 256 + MODE
	BSS(FDSECMINMAX,8)	| +0: (min sector) sector length
				| +1: (min sector) track #
				| +2: (min sector) side
				| +3: (min sector) sector #
				| +4: (max sector) sector length
				| +5: (max sector) track #
				| +6: (max sector) side
				| +7: (max sector) sector #
#ifdef SCSI_ADHOC_BOOTPART
	BSS(SCSI_PARTTOP,4)	| top position of boot partition in sector
	BSS(SCSI_BLKLEN,4)	| sector len 0: 256, 1: 512, 2: 1024
#endif
