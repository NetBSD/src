| file: boot.S
| author: chapuni(webmaster@chapuni.com)
|         ITOH Yasufumi
|
| $NetBSD: boot.S,v 1.9.4.1 2013/01/16 05:33:08 yamt Exp $

#include <machine/asm.h>
#include "iocscall.h"

#define BASEOFF		0x8000
#define BASEPTR_A	(TEXTADDR+BASEOFF)
#define BASEPTR_R	%pc@(top+BASEOFF:W)

#define SRAM		0x00ED0000	/* SRAM stat addr */
#define SRAM_MEMSZ	(SRAM + 8)	/* (L) size of main memory */
#define MINMEM		0x00400000	/* at least 4MB required */

#define BOOT_ERROR(s)	jbsr boot_error; .asciz s; .even

	.globl	_C_LABEL(bootufs)
	.text
ASENTRY_NOPROFILE(start)
ASENTRY_NOPROFILE(top)
		bras	_ASM_LABEL(entry0)
		.ascii	"SHARP/"
		.ascii	"X680x0"
		.word	0x8199,0x94e6,0x82ea,0x82bd
		.word	0x8e9e,0x82c9,0x82cd,0x8cbb
		.word	0x8ec0,0x93a6,0x94f0,0x8149
		.word	0
| 0x2000 (FD), 0x2400 (SASI/SCSI) (もしくは 0x0f0000)
| d4 にはすでに SCSI ID が入っている
| ここから jmp まではリロケータブルに書かねばならない。
ASENTRY_NOPROFILE(entry0)
		moveml	%d0-%d7/%a0-%a7,_C_LABEL(startregs)
		lea	BASEPTR_A:l,%a5		| set base ptr
#define _RELOC(adr)	%a5@(((adr)-(BASEPTR_A&0xffff)):W)
#define ASRELOC(var)	_RELOC(_ASM_LABEL(var))
#define RELOC(var)	_RELOC(_C_LABEL(var))

		lea	RELOC(edata),%a1
		bra	_ASM_LABEL(entry)

|	Disklabel= 404bytes
|	Since LABELLOFFSET in <machine/disklabel.h> is 0x40,
|	entry must be after 0x000001d4 (0x000f01d4)
		nop
disklabel:
		.space	404

ASENTRY_NOPROFILE(entry)
		movew	#_end-1,%d0	| bss end (low word only)

		| clear out bss  (must be <= 64KB)
		subw	%a1,%d0
clrbss:		clrb	%a1@+
		dbra	%d0,clrbss

		movel	%d4,RELOC(ID)	| SCSI ID (if booted from SCSI)

		| set system stack
		lea	ASRELOC(top),%a1 | set stack pointer to 0x000F0000
		lea	%a1@,%sp	| a1 will be used later for IOCS calls

		| we use 68020 instructions, and check MPU beforehand
		|
		| here d0.w = -1, and the above "subw a1,d0" = 0x9049, and
		|	if MPU <= 010	loads 0x49,
		|	if MPU >= 020	loads 0x90.
		| This is a move, not a tst instruction
		| because pc-relative tsts are not availble on 000/010.
chkmpu:		moveb	%pc@(clrbss-chkmpu-2:B,%d0:W:2),%d0	| 103B 02xx
		jmi	mpuok		| MC68020 or later
		BOOT_ERROR("MPU 68000?")
mpuok:		| XXX check for MMU?

		IOCS(__BOOTINF)
		lsll	#8,%d0		| clear MSByte
		lsrl	#8,%d0		|
		movel	%d0,RELOC(BOOT_INFO)

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
		moveml	%d0-%d1,RELOC(FDSECMINMAX) | min and max sec #
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
		BOOT_ERROR("unsupported boot device")

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
		SCSIIOCS(__S_READCAP)	| using buffer at a1
		tstl	%d0
		jeq	1f
		BOOT_ERROR("READCAP failed")
1:		moveq	#0,%d5
		moveb	%a1@(6),%d5	| 1: 256, 2: 512, 4: 1024
		lsrb	#1,%d5		| 0: 256, 1: 512, 2: 1024
		movel	%d5,RELOC(SCSI_BLKLEN)

		| find out the start position of the boot partition
		| XXX VERY AD HOC
		|
		| ROM firmware:
		|	pass read pos (in block #) in d2
		|	Human68k-style partition table does not exist
		|	d2 is 4 at the maximum
		| SCSI IPLs (genuine and SxSI):
		|	pass read pos (in kilobytes) in d2
		|	d2 is bigger than 0x20
		|	partition table on the memory is destroyed
		| BOOT MENU Ver.2.22:
		|	passes partition table entry address in a0
		|	d2 is cleared to zero
		| No other IPL is supported.  XXX FIXME
		tstl	%d2
		jne	sc1
		| no information in d2 -- probably from BOOT MENU
		| a0 points the partiion table entry
		movel	%a0@(0x0008),%d2 | in KByte
sc1:		cmpl	#0x20,%d2
		jcs	sc2
		lsll	#8,%d2		| clear MSByte
		lsrl	#6,%d2		|
		lsrl	%d5,%d2		| in sector
sc2:
		| read entire boot
		moveq	#8192/256,%d3	| size is 8KB
		lsrl	%d5,%d3		| in sector
		jbsr	scsiread	| read at  %a1

		cmpil	#5,%d2
		bcc	sc3
		movql	#0,%d2
sc3:		movel	%d2,RELOC(SCSI_PARTTOP)
#else
		moveq	#1,%d5		| 512bytes/sec
		movel	%d5,%sp@-
		moveq	#8192/512,%d3	| 読み込める最大サイズ
		moveq	#0x40,%d2	| いわゆる決め打ち(sd*a のみ)
		SCSIIOCS(__S_READ)
#endif

boot_read_done:
		jmp	first_kbyte

read_error:	BOOT_ERROR("read error")

#undef RELOC	/* base register  a5  is no longer available */
#undef ASRELOC
#undef _RELOC

|
|	read SCSI
|
|	input:	d2.l: pos in sector
|		d3.l: len in sector
|		d4: target SCSI ID
|		d5: sector length (0: 256, 1: 512, 2: 1024)
|		a1: buffer address
|	destroy:
|		d0, d1, a1
|
scsiread:
		moveml	%d2-%d3/%d6-%d7/%a2,%sp@-
		| if (pos >= 0x200000 || (len > 255 && pos + len >= 0x200000))
		|	use READEXT
		| else
		|	use READ
		moveq	#0x20,%d0
		swap	%d0		| d0.l = 0x00200000
		moveq	#0,%d6
		subqb	#1,%d6		| d6.l = 255
		moveq	#8,%d7
		addb	%d5,%d7		| d7.b = (sector length: 0-2) + 8
		cmpl	%d0,%d2
		jcc	scsiread_ext
		moveq	#__S_READ,%d1
		cmpl	%d3,%d6
		jcc	scsiread_noext
		subl	%d2,%d0		| d0.0 = 0x200000 - pos
		cmpl	%d0,%d3		|	<= len
		jcs	scsiread_noext	| no

scsiread_ext:	| use READEXT
		extw	%d6		| d6.l = 65535
		moveq	#__S_READEXT,%d1

scsiread_noext:	| use READ
loop_scsiread:
		| d1: SCSI IOCS call #
		| d6: max sector count at a time
		movel	%d3,%a2		| save original len in a2
		cmpl	%d3,%d6
		jcc	1f
		movel	%d6,%d3
1:		IOCS(__SCSIDRV)		| SCSIIOCS(d1)
		tstl	%d0
		jne	read_error
		movel	%d3,%d0		| addr += read count << (8 + sec len)
		asll	%d7,%d0
		addl	%d0,%a1
		exg	%d3,%a2		| restore original len to d3
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
		jmp	_C_LABEL(bootufs)	| 0x0Fxxxx に飛んでゆく

		.word	0

| int badbaddr __P((void *adr));
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
		movel	%sp,%d1			| save sp
		moveal	%sp@(10),%a0
		tstb	%a0@			| try read...
		moveq	#0,%d0			| this is skipped on bus error
badr1:		moveal	%d1,%sp			| restore sp
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
		| d2.l:		pos in 512byte-blocks
		| d3.l:		length in bytes
		| a1 (=d1):	buffer address

		lea	BASEPTR_R,%a5	| set base ptr
#define _RELOC(adr)	%a5@(((adr)-(BASEPTR_A&0xffff)):W)
#define ASRELOC(var)	_RELOC(_ASM_LABEL(var))
#define RELOC(var)	_RELOC(_C_LABEL(var))

		tstb	_RELOC(_C_LABEL(BOOT_INFO)+1) | simple check.  may be incorrect!
		beqs	raw_read_floppy

raw_read_scsi:
		movel	RELOC(ID),%d4	| SCSI ID
#ifdef SCSI_ADHOC_BOOTPART
		movel	RELOC(SCSI_BLKLEN),%d5 | sector size: 0-2
		| XXX length must be sector aligned
		lsrl	#8,%d3		| size in 256byte-blocks
		lsrl	%d5,%d3		| size in sector
		bcss	read_half	| minimal error check
		lsll	#1,%d2		| X flag and d2: pos in 256byte-blocks
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
|		BOOT_ERROR("out of seek")	| pos exceeds 32bit
|1:
		jbsr	scsiread
		bras	raw_read_end

raw_read_floppy:
		|
		| Floppy read routine
		|

		| convert to seek position

		asll	#2,%d2		| size in 128byte-blocks

		| sec = raw_read_pos	(d2)
		| sec >>= 7 + (sector length: 0-3)

		lea	RELOC(FDSECMINMAX),%a0
		moveq	#0,%d1
		moveb	%a0@,%d1	| d1: sector length (0-3)
		lsrl	%d1,%d2		| d2: pos in sector
		bcss	read_half	| error check

		| trk = sec / (# sectors)
		| sec = sec % (# sectors)

		moveb	%a0@(7),%d1	| d1: max sector #
		subb	%a0@(3),%d1	|   - min sector #
		addqb	#1,%d1		| d1: # sectors
		divu	%d1,%d2		| d2: (sec << 16) | track

		| position = (sec length << 24) | (track/2 << 16)
		|		| (track%2 << 8) | (min sec # + sec)

		movel	%a0@,%d0	| d0: (sec len << 24) | min sec #
		lsrw	#1,%d2		| d2: (sec << 16) | (track / 2)
		jcc	1f
		bset	#8,%d0		| |= (track % 2) << 8
1:		swap	%d2		| d2: ((track / 2) << 16) | sec
		addl	%d0,%d2		| d2: position

		| read
		movel	RELOC(FDMODE),%d1	| PDA*256 + MODE

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
		| return:
		|  d0:	bit 31-24	ST0
		|	bit 23-16	ST1
		|	bit 15- 8	ST2
		|	bit  7- 0	C
		|	-1 on parameter error
		| destroy: d0, d2, d3, a1
		IOCS(__B_READ)
		andil	#0xf8ffff00,%d0		| check status (must be zero)
		jne	read_error

raw_read_end:
		moveml	%sp@+,%a2-%a6/%d2-%d7
		rts
#undef _RELOC	/* base register  a5  is no longer available */
#undef ASRELOC
#undef RELOC

read_half:	BOOT_ERROR("read half of block")


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
|	global variables
|
	BSS(ID, 4)			| SCSI ID
	BSS(BOOT_INFO, 4)		| result of IOCS(__BOOTINF)
	BSS(FDMODE, 4)			| Floppy access mode: PDA x 256 + MODE
	BSS(FDSECMINMAX, 8)		| +0: (min sector) sector length
					| +1: (min sector) track #
					| +2: (min sector) side
					| +3: (min sector) sector #
					| +4: (max sector) sector length
					| +5: (max sector) track #
					| +6: (max sector) side
					| +7: (max sector) sector #
#ifdef SCSI_ADHOC_BOOTPART
	BSS(SCSI_PARTTOP, 4)		| start sector of boot partition
	BSS(SCSI_BLKLEN ,4)		| sector len 0: 256, 1: 512, 2: 1024
#endif
