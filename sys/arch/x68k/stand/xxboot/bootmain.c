/*	$NetBSD: bootmain.c,v 1.1.2.3 2013/01/16 05:33:08 yamt Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Takumi Nakamura.
 * Copyright (c) 1999, 2000 Itoh Yasufumi.
 * Copyright (c) 2001 Minoura Makoto.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Takumi Nakamura.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <machine/bootinfo.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include "libx68k.h"
#include "iocs.h"
#include "exec_image.h"

#define EXSCSI_BDID	((void *)0x00ea0001)
#define BINF_ISFD(pbinf)	(*((uint8_t *)(pbinf) + 1) == 0)

/* boot.S */
extern int badbaddr(volatile void *);
extern unsigned int ID;		/* target SCSI ID */
extern unsigned int BOOT_INFO;	/* result of IOCS(__BOOTINF) */
extern struct {
	struct fdfmt{
		uint8_t	N;	/* sector length 0: 128, ..., 3: 1K */
		uint8_t	C;	/* cylinder # */
		uint8_t	H;	/* head # */
		uint8_t	R;	/* sector # */
	} minsec, maxsec;
} FDSECMINMAX;			/* FD format type of the first track */

/* for debug */
unsigned int startregs[16];

static int get_scsi_host_adapter(char *);
void bootmain(void) __attribute__ ((__noreturn__));

/*
 * Check the type of SCSI interface
 */
static int
get_scsi_host_adapter(char *devstr)
{
	uint8_t *bootrom;
	int ha;

#ifdef XXBOOT_DEBUG
	*(uint32_t *)(devstr +  0) = '/' << 24 | 's' << 16 | 'p' << 8 | 'c';
#if defined(CDBOOT)
	*(uint32_t *)(devstr +  4) = '@' << 24 | '0' << 16 | '/' << 8 | 'c';
#else
	*(uint32_t *)(devstr +  4) = '@' << 24 | '0' << 16 | '/' << 8 | 's';
#endif
	*(uint32_t *)(devstr +  8) = 'd' << 24 | '@' << 16 | '0' << 8 | ',';
	*(uint32_t *)(devstr + 12) = '0' << 24 | ':' << 16 | 'a' << 8 | '\0';
#endif

	bootrom = (uint8_t *)(BOOT_INFO & 0x00ffffe0);
	/*
	 * bootrom+0x24	"SCSIIN" ... Internal SCSI (spc@0)
	 *		"SCSIEX" ... External SCSI (spc@1 or mha@0)
	 */
	if (*(uint16_t *)(bootrom + 0x24 + 4) == 0x494e) {	/* "IN" */
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 0;
	} else if (badbaddr(EXSCSI_BDID)) {
		ha = (X68K_BOOT_SCSIIF_MHA << 4) | 0;
#ifdef XXBOOT_DEBUG
		*(uint32_t *)devstr = '/' << 24 | 'm' << 16 | 'h' << 8 | 'a';
#endif
	} else {
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 1;
#ifdef XXBOOT_DEBUG
		devstr[5] = '1';
#endif
	}

	return ha;
}

extern const char bootprog_name[], bootprog_rev[];

void
bootmain(void)
{
	int bootdev, ha, fd;
	char bootdevstr[16];
	u_long marks[MARK_MAX];

	IOCS_B_PRINT(bootprog_name);
	IOCS_B_PRINT(" rev.");
	IOCS_B_PRINT(bootprog_rev);
	IOCS_B_PRINT("\r\n");

	ha = get_scsi_host_adapter(bootdevstr);
#ifdef XXBOOT_DEBUG
	bootdevstr[10] = '0' + (ID & 7);
	bootdevstr[14] = 'a';
#endif

#if defined(CDBOOT)
	bootdev = X68K_MAKESCSIBOOTDEV(X68K_MAJOR_CD, ha >> 4, ha & 15,
				       ID & 7, 0, 0);
#elif defined(FDBOOT) || defined(SDBOOT)
	if (BINF_ISFD(&BOOT_INFO)) {
		/* floppy */
#ifdef XXBOOT_DEBUG
		*(uint32_t *)bootdevstr =
		    ('f' << 24 | 'd' << 16 | '@' << 8 | '0' + (BOOT_INFO & 3));
		bootdevstr[4] = '\0';
#endif
		/* fdNa for 1024 bytes/sector, fdNc for 512 bytes/sector */
		bootdev = X68K_MAKEBOOTDEV(X68K_MAJOR_FD, BOOT_INFO & 3,
		    (FDSECMINMAX.minsec.N == 3) ? 0 : 2);
	} else {
		/* SCSI */
		bootdev = X68K_MAKESCSIBOOTDEV(X68K_MAJOR_SD, ha >> 4, ha & 15,
		    ID & 7, 0, 0 /* XXX: assume partition a */);
	}
#else
	bootdev = 0;
#endif

#ifdef XXBOOT_DEBUG
	IOCS_B_PRINT("boot device: ");
	IOCS_B_PRINT(bootdevstr);
#endif
	IOCS_B_PRINT("\r\n");

	marks[MARK_START] = BOOT_TEXTADDR;
	fd = loadfile("x68k/boot", marks, LOAD_TEXT|LOAD_DATA|LOAD_BSS);
	if (fd < 0)
		fd = loadfile("boot", marks, LOAD_TEXT|LOAD_DATA|LOAD_BSS);
	if (fd >= 0) {
		close(fd);
		exec_image(BOOT_TEXTADDR, /* image loaded at */
			   BOOT_TEXTADDR, /* image executed at */
			   BOOT_TEXTADDR, /* XXX: entry point */
			   0, 		  /* XXX: image size */
			   bootdev, 0);	  /* arguments */
	}
	IOCS_B_PRINT("can't load the secondary bootstrap.");
	exit(0);
}

int
devopen(struct open_file *f, const char *fname, char **file)
{

	*file = __UNCONST(fname);
	return xxopen(f);
}
