| $NetBSD: boot.S,v 1.13 2020/08/16 06:43:43 isaki Exp $

|
| (1) IPL (or previous stage loader) loads first 1KB of this primary
|     bootloader to (*).  (*) is 0x2000 (from FD) or 0x2400 (from SASI/SCSI).
|
| (2) The first 1KB loads full primary bootloader (including first 1KB) from
|     the boot partition to 0x3000.  And jump to there.
|
| (3) The full primary bootloader loads the secondary bootloader known as
|     /boot from its filesystem to 0x6000.  And jump to there.
|
|       (1)         ->        (2)         ->        (3)
|  +------------+        +------------+        +------------+    0x000000
|  :            :        :            :        :            :
|  +------------+        +------------+        +------------+    (*)
|  | first 1KB  |        | first 1KB  |        | first 1KB  |
|  +------------+        +------------+        +------------+    (*)+0x400
|  :            :        :            :        :            :
|  :            :        +------------+        +------------+    0x003000
|  :            :        |full primary|        |full primary|
|  :            :        |boot loader |        |boot loader |
|  :            :        |(text+data) |        |(text+data) |
|  :            :        +------------+        +------------+    0x005000
|  :            :        |(startregs) |        |(startregs) |
|  :            :        |(bss)       |        |(bss)       |
|  :            :        +------------+        +------------+    0x006000
|  :            :        :            :        | /boot      |
|  :            :        :            :        +------------+
|  :            :        :            :        :            :
|  ~            ~        ~            ~        ~            ~
|  :            :        :            :<-SP    :            :<-SP
|  + - - - - - -+        + - - - - - -+        + - - - - - -+    0x100000
|  :            :        :(heap)      :        :(heap)      :
|  :            :        :            :        :            :
|
| The program code before first_kbyte
| - must not access any text/data labels after first_kbyte
|   (because it may not be loaded yet).
| - must access any labels before first_kbyte by PC relative addressing
|   (because this loader is assembled as starting from 0x3000 but is loaded
|   at 0x2000 or 0x2400).
| - must use RELOC() macro to access bss section (and first_kbyte as a jump
|   destination address).
|
| The program code after first_kbyte can access any labels in all sections
| directly.

#include <machine/asm.h>
#include "iocscall.h"

#define RELOC(var)	%a5@(((var)-top):W)

#define BOOT_ERROR(s)	jbsr boot_error; .asciz s; .even

#define minN	(0)
#define minC	(1)
#define minH	(2)
#define minR	(3)
#define maxN	(4)
#define maxC	(5)
#define maxH	(6)
#define maxR	(7)

		.globl	_C_LABEL(bootmain)
		.globl	_C_LABEL(startregs)
		.text

ASENTRY_NOPROFILE(start)
ASENTRY_NOPROFILE(top)
		bras	entry
		.ascii	"SHARP/"
		.ascii	"X680x0"
		.word	0x8199,0x94e6,0x82ea,0x82bd
		.word	0x8e9e,0x82c9,0x82cd,0x8cbb
		.word	0x8ec0,0x93a6,0x94f0,0x8149
msg_progname:
		| This will be printed on boot_error.  And it also roles
		| a signature in binary dump.
		| Max length of PROG(without \0) is 14 ("fdboot_ustarfs").
		.ascii	"\r\n\n"		| 3
		.ascii	PROG			| 14
		.asciz	": "			| 2+1
		.org	msg_progname + 20
entry:
		jbra	disklabel_end

		| Disklabel must be placed at 0x40 and the size is 404 bytes.
		| (See LABELOFFSET in <machine/disklabel.h>)
		.org	0x40
disklabel:
		.space	404
disklabel_end:
		| At first save all initial registers for observing traces
		| of the IPL (or the previous bootloader).  At this point
		| we cannot use RELOC() yet so that use absolute addressing.
		| To prevent startregs from being cleared by subsequent bss
		| initialization, we place it out of bss area.
		moveml	%d0-%d7/%a0-%a7,startregs:W

		| Initialize the screen.  Some IPL (060turbo ROM or genuine
		| boot selector) don't initialize the screen.  It should be
		| done as early as possible.
		moveql	#0x10,%d1
		IOCS(__CRTMOD)

		| Set system stack
		swap	%d1			| %d1 = 0x0010_0000
		moveal	%d1,%sp

		| Set base pointer.  Now we can use RELOC() macro.
		leal	TEXTADDR:W,%a5

		| Initialize bss.
		| This code limits bss less than 64KB but it's no matter.
		| The bss cannot grow more than 4KB.  See xxboot.ldscript.
		leal	RELOC(__bss_start),%a1
		movew	#_end - 1,%d0		| bss end

		subw	%a1,%d0			| don't change this op!!
clrbss:						|  see chkmpu below
		clrb	%a1@+
		dbra	%d0,clrbss

		| If it boots from SCSI, %d4 has SCSI ID.
		movel	%d4,RELOC(SCSI_ID)

chkmpu:
		| Check MPU beforehand since we want to use 68020 instructions.
		| Here the above "subw %a1,%d0" = 0x9049 and %d0.w = -1 at this
		| point, so that subsequent moveb loads
		|   0x49 if MPU <= 010 (clrbss + %d0.w)
		|   0x90 if MPU >= 020 (clrbss + %d0.w*2).
		| This is a MOVE op, not a TST op because TST with pc-relative
		| is not available on 000/010.
		moveb	%pc@(clrbss-chkmpu-2:B,%d0:W:2),%d0
		jmi	mpuok
		BOOT_ERROR("MPU 68000?");
mpuok:
		|
		| Check where did I boot from.
		|
		IOCS(__BOOTINF)
		movel	%d0,RELOC(BOOT_INFO)	| save whole result

		| %d0 = 0xHHWWWWWW
		|
		| HH:     how did I boot (powersw or alarm etc)
		| WWWWWW: where did I boot from
		|  0x80...0x8f		SASI
		|  0x90...0x93		Floppy
		|  0xed0000...0xed3ffe	SRAM
		|  others		ROM (maybe SCSI)

		bfextu	%d0{#8:#8},%d1
		jne	boot_rom_ram		| ROM or SRAM
		| FALLTHROUGH			| SASI or Floppy

boot_sasi_floppy:
		| Floppy or SASI
		cmpiw	#0x90,%d0
		jlt	boot_dev_not_supp	| SASI

		|
		| Boot from floppy
		|
boot_floppy:
		| Make PDA+MODE
		lslw	#8,%d0			| %d0=$00009X00 (X is unit#)
		moveql	#0x70,%d1
		orw	%d0,%d1			| %d1=$00009X70 = (PDA<<8)+MODE
		movel	%d1,RELOC(FDMODE)
check_fd_format:
		| Check fd format.
		| Obtain min & max sector # of track(cylinder) 0.
		| On x68k, we can consider only double-sided floppy.
		moveql	#0,%d2
init_loop:
		| On 1st time, clear %d3-%d5 with zero.
		| On 2nd time, initialize %d3-%d5 with first %d2.
		movel	%d2,%d3			| %d3: initial NCHR
		movel	%d2,%d4			| %d4: minimum NCHR
		movel	%d2,%d5			| %d5: maximum NCHR
loop:
		| B_READID with MSB of %d2 set obtains detected CHRN to %d2.
		moveql	#1,%d2			| %d2 = 0x00000001
		rorl	#1,%d2			| %d2 = 0x80000000
		IOCS(__B_READID)
						| %d2 = 0xCCHHRRNN
		rorl	#8,%d2			| %d2 = 0xNNCCHHRR

		| On 1st time, goto init_loop with %d2 (%d2 is not zero).
		| On 2nd time, fall through because %d3 is not zero.
		tstl	%d3
		jeq	init_loop

		cmpl	%d4,%d2			| if (%d2 < %d4)
		jge	1f			|
		movel	%d2,%d4			|  min = %d2
1:
		cmpl	%d5,%d2			| if (%d2 > %d5)
		jle	1f			|
		movel	%d2,%d5			|  max = %d2
1:
		cmpl	%d3,%d2			| if (%d2 == %d3) break
		jne	loop

		| Assume 2HD
		oriw	#0x0100,%d5		| FDSEC.maxsec.H = 1
		moveml	%d4-%d5,RELOC(FDSEC)	| Store
		| end of check_fd_format

		| read 8KB myself from floppy
						| %d1: (PDA<<8)+MODE already
		movel	%d4,%d2			| %d2: read pos = first sector
		moveql	#0x20,%d3		| %d3: read bytes = (0x20 << 8)
		lsll	#8,%d3			|  = 0x2000 = 8192
		leal	%a5@,%a1		| %a1: dest buffer
		IOCS(__B_READ)

		| Jump to full parimary loader
		jmp	RELOC(first_kbyte)

boot_rom_ram:
		| ROM(SCSI) or SRAM
		cmpib	#0xed,%d1
		jeq	boot_dev_not_supp	| SRAM

		|
		| Boot from SCSI
		|
boot_scsi:
		| get block length of the SCSI disk
		leal	RELOC(SCSI_CAP),%a1
		SCSIIOCS(__S_READCAP)
		tstl	%d0
		jeq	boot_scsi1
		BOOT_ERROR("READCAP failed")
boot_scsi1:
		movel	RELOC(SCSI_CAP+4),%d0	| %d0 = blocksize in bytes
		lsrl	#2,%d0			| %d0 = blocksize in longword
		moveql	#25,%d5
		bfffo	%d0{#0:#32},%d1		| 25:256 24:512 23:1024 22:2048
		subl	%d1,%d5			|  0:256  1:512  2:1024  3:2048
		movel	%d5,RELOC(SCSI_BLKLEN)	| %d5 = sector length index

		| Find out the start position of the boot partition.
		| There seems to be no interface or consensus about this and
		| so that we would have to do it heuristicly.
		|
		| ROM firmware:
		|	pass read pos (in block #, aka sector #) in %d2.
		|	Human68k-style partition table does not exist.
		|	%d2 is 4 at the maximum.
		| SCSI IPLs (genuine and SxSI):
		|	pass read pos (in kilobytes) in %d2.
		|	%d2 is bigger than 0x20.
		|	partition table on the memory is destroyed.
		| BOOT MENU Ver.2.22:
		|	passes partition table entry address in %a0.
		|	%d2 is cleared to zero
		| No other IPLs are supported.

		tstl	%d2
		jne	1f
		| If no information in %d2, probably from BOOT MENU.
		| %a0 points the on-memory partition table entry.
		movel	%a0@(0x0008),%d2	| %d2 = pos in kbyte
1:
		moveql	#0x20,%d3
		cmpl	%d3,%d2
		jcs	1f			| jump if %d2 > 0x20
		| SCSI IPL or BOOT MENU.
		| At this point, %d2 is pos in kbyte in all cases.
		lsll	#8,%d2			| %d2 = pos in longword
		divul	%d0,%d2			| %d2 = pos in sector
1:
		| At this point, %d2 is pos in sector in all cases.
		| TDSIZE = 8192, TDSIZE / 4 = 0x800 = (0x20 << 6).
		lsll	#6,%d3			| %d3 = TDSIZE in longword
		divul	%d0,%d3			| %d0 = TDSIZE in sector
		| Read full primary bootloader
		moveal	%a5,%a1			| %a1 = dest buffer
		jbsr	scsiread

		| Selected start sector should not <= 4.  There should be
		| partition table.  If so, repoints to zero(?).
		moveql	#5,%d0
		cmpl	%d0,%d2
		bcc	1f
		moveql	#0,%d2
1:
		movel	%d2,RELOC(SCSI_PARTTOP)

		| Jump to full parimary loader
		jmp	RELOC(first_kbyte)

|
| scsiread
|	Read SCSI disk using __S_READ as possible.  If __S_READ cannot be
|	used (due to read length or offset), use __S_READEXT instead.
| input:
|	%d2.l: pos in sector
|	%d3.l: len in sector (must be < 65536)
|	%d4.l: target SCSI ID
|	%d5.l: sector length index (0:256, 1:512, 2:1024, 3:2048, ...)
|	%a1.l: buffer address
| destroy:
|	%d0,%d1
scsiread:
		| if (len >= 256 || pos + len >= 0x200000)
		|   use READEXT
		| else
		|   use READ

		moveql	#__S_READEXT,%d1

		cmpiw	#256,%d3
		jge	scsiread_core		| if (d3 >= 256) use READEXT

		movel	%d2,%d0
		addl	%d3,%d0			| %d0 = pos + len
		jcs	scsiread_core		| if overflow, use READEXT
		bftst	%d0{#0:#11}		| if (pos + len >= 0x200000)
		jne	scsiread_core		|  use REAEXT

		moveql	#__S_READ,%d1		| else use READ
scsiread_core:
		IOCS(__SCSIDRV)
		rts

boot_dev_not_supp:
		BOOT_ERROR("not supported device");

|
| void __dead BOOT_ERROR(const char *msg);
|	Print an error message, wait for key press, and reboot.
|	Called from C.
ENTRY_NOPROFILE(BOOT_ERROR)
		addql	#4,%sp			| throw away return address
		| FALLTHROUGH
|
| BOOT_ERROR(msg)
|	Print an error message, wait for key press, and reboot.
|	Called from asm.
boot_error:
		leal	%pc@(msg_progname),%a1
		IOCS(__B_PRINT)
		moveal	%sp@+,%a1
		IOCS(__B_PRINT)
ENTRY_NOPROFILE(exit)
ENTRY_NOPROFILE(_rtt)
		leal	%pc@(msg_reboot),%a1
		IOCS(__B_PRINT)

		| wait for a key press (or release of a modifier)
		IOCS(__B_KEYINP)

		| issue software reset
		trap	#10
		| NOTREACHED
msg_reboot:
		.asciz	"\r\n[Hit key to reboot]"
		.even

		.globl	first_kbyte
first_kbyte:
|--------------------------------------------------------------------------
|
#if defined(SELFTEST)
		jbsr	selftest_ashldi3
		jbsr	selftest_ashrdi3
		jbsr	selftest_memcmp
		jbsr	selftest_memmove
		jbsr	selftest_memset
#endif

		jmp	_C_LABEL(bootmain)
		| NOTREACHED

|
| uint32_t badbadd(void *addr)
|	returns 1 if reading addr occurs bus error.  Otherwise it returns 0.
ENTRY_NOPROFILE(badbaddr)
		leal	0x0008:W,%a1		| bus error vector
		moveql	#1,%d0
		leal	%pc@(badbaddr1),%a0
		movew	%sr,%sp@-
		oriw	#0x0700,%sr		| keep out interrupts
		movel	%a1@,%sp@-
		movel	%a0,%a1@		| set bus error vector
		movel	%sp,%d1			| save sp
		moveal	%sp@(10),%a0
		tstb	%a0@			| try read...
		moveql	#0,%d0			| this is skipped on bus error
badbaddr1:
		moveal	%d1,%sp			| restore sp
		movel	%sp@+,%a1@
		movew	%sp@+,%sr
		rts

|
| int raw_read(uint32_t blkpos, uint32_t bytelen, void *buf)
|	blkpos:  read start position in 512 byte block unit (always?).
|	bytelen: read length in bytes.
|	         caller already avoids bytelen == 0 so that no checks here.
|	         must be a multiple of sector size on scsi.
|	buf:     destination buffer address
|
ENTRY_NOPROFILE(raw_read)
		moveal	%sp,%a1
		moveml	%d2-%d7/%a2-%a6,%sp@-
		moveml	%a1@,%d0/%d2-%d3/%a1	| %d0 (return address)
						| %d2 blkpos
						| %d3 bytelen
						| %a1 buf
		| At this point boot device is either floppy or SCSI.
		tstb	%pc@(BOOT_INFO+1)
		jeq	raw_read_floppy
		| FALLTHROUGH

raw_read_scsi:
						| %d2 = pos from device top
						|  in 512 bytes/block
		lsll	#1,%d2			| %d2 = in 256 bytes/block
		movel	%pc@(SCSI_BLKLEN),%d5	| %d5 = sector length index
		lsrl	%d5,%d2			| %d2 = pos from device top
						|  in media sector size

		divull	%pc@(SCSI_CAP+4),%d0:%d3| %d3 = bytelen / sectsize
						| %d0 = bytelen % sectsize
		tstl	%d0
		jeq	.Lraw1
		BOOT_ERROR("Err1")		| ASSERT(bytelen%sectsize==0)
.Lraw1:
		movel	%pc@(SCSI_ID),%d4	| %d4 = SCSI ID
		jbsr	scsiread

raw_read_exit:
		moveml	%sp@+,%d2-%d7/%a2-%a6
		rts

raw_read_floppy:
		| nhead = FDSEC.maxsec.H - FDSEC.minsec.H + 1
		|       = 2;
		| nsect = FDSEC.maxsec.R - FDSEC.minsec.R + 1;
		|
		| sect = (blkpos % nsect) + FDSEC.minsec.R;
		| head = ((blkpos / nsect) % nhead) + FDSEC.minsec.H;
		| cyl  = ((blkpos / nsect) / nhead) + FDSEC.minsec.C;
		|
		| NCHR = (FDSEC.minsec.N << 24) |
		|      (cyl << 16) |
		|      (head << 8) |
		|      sect;

		| calc nsect.
		moveql	#1,%d0			| %d0 = 1
		addb	%pc@(FDSEC+maxR),%d0	| %d0 = 1 + maxsec.R
		subb	%pc@(FDSEC+minR),%d0	| %d0 = 1 + maxsec.R - minsec.R
						|     = nsect

		| Convert blkpos to N/C/H/R.
		divuw	%d0,%d2			| %d2.hw = blkpos % nsect
						| %d2.lw = blkpos / nsect
		| Here, %d2.hw becomes sector number and .lw becomes cyl+head.
		| %d2.lw = %0000_0000_CCCC_CCCH in binary form.  LSB of
		| (blkpos / nsect) is head number because we support only
		| double-sided floppy here.
						| %d2.w = %0000_0000_CCCC_CCCH
		lslw	#7,%d2			| %d2.w = %0CCC_CCCC_H000_0000
		lsrb	#7,%d2			| %d2.w = %0CCC_CCCC_0000_000H
						| i.e,
						| %d2 = $00rrCCHH
		swap	%d2			| %d2 = $CCHH00rr
		lslw	#8,%d2			| %d2 = $CCHHrr00
		| two bytes from odd FDSEC+minR is (minR << 8 | maxN) and
		| minN == maxN always.
		addw	%pc@(FDSEC+minR),%d2	| %d2 = $CCHHRRNN
		rorl	#8,%d2			| %d2 = $NNCCHHRR

		movel	%pc@(FDMODE),%d1	| %d1 = PDA+MODE
		IOCS(__B_READ)
		andil	#0xf8ffff00,%d0		| Check status (must be zero)
		jeq	raw_read_exit
		BOOT_ERROR("B_READ failed");

|
| BSS
|
		BSS(BOOT_INFO, 4)	| whole result of IOCS BOOTINF

		BSS(FDMODE, 4)
		BSS(FDSEC, 8)		| +0: (minN) sector length
					| +1: (minC) track number
					| +2: (minH) head
					| +3: (minR) sector number
					| +4: (maxN) sector length
					| +5: (maxC) track number
					| +6: (maxH) head
					| +7: (maxR) sector number

		BSS(SCSI_ID, 4)		| SCSI ID, if booted from SCSI
		BSS(SCSI_CAP, 8)	| result of SCSI READCAP
					|  +0.L: total number of logical blocks
					|  +4.L: block length in bytes
		BSS(SCSI_PARTTOP, 4)	| top sector # of this partition
		BSS(SCSI_BLKLEN ,4)	| sector length index
					|  0:256, 1:512, 2:1024, 3:2048, ..
