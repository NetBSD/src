/*	$NetBSD: bootmain.c,v 1.1.2.2 2012/04/17 00:07:03 yamt Exp $	*/

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

#define EXSCSI_BDID	((void*) 0x00ea0001)

/* boot_cd9660.S */
extern int badbaddr __P((volatile void *adr));
extern unsigned int ID;		/* target SCSI ID */
extern unsigned int BOOT_INFO;	/* result of IOCS(__BOOTINF) */

/* for debug */
unsigned int startregs[16];

static int get_scsi_host_adapter (char *);
void bootmain (void) __attribute__ ((__noreturn__));

/*
 * Check the type of SCSI interface
 */
static int
get_scsi_host_adapter(devstr)
	char *devstr;
{
	char *bootrom;
	int ha;

	*(int *)devstr = '/' << 24 | 's' << 16 | 'p' << 8 | 'c';
	*(int *)(devstr + 4) = '@' << 24 | '0' << 16 | '/' << 8 | 'c';
	*(int *)(devstr + 8) = 'd' << 24 | '@' << 16 | '0' << 8 | ',';
	*(int *)(devstr + 12) = '0' << 24 | ':' << 16 | 'a' << 8 | '\0';

	bootrom = (char *) (BOOT_INFO & 0x00ffffe0);
	/*
	 * bootrom+0x24	"SCSIIN" ... Internal SCSI (spc@0)
	 *		"SCSIEX" ... External SCSI (spc@1 or mha@0)
	 */
	if (*(u_short *)(bootrom + 0x24 + 4) == 0x494e) {	/* "IN" */
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 0;
	} else if (badbaddr(EXSCSI_BDID)) {
		ha = (X68K_BOOT_SCSIIF_MHA << 4) | 0;
		*(int *)devstr = '/' << 24 | 'm' << 16 | 'h' << 8 | 'a';
	} else {
		ha = (X68K_BOOT_SCSIIF_SPC << 4) | 1;
		devstr[5] = '1';
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

#ifdef DEBUG
	printf("%s rev.%s\n", bootprog_name, bootprog_rev);
#endif

	ha = get_scsi_host_adapter(bootdevstr);
	bootdevstr[10] = '0' + (ID & 7);
	bootdevstr[14] = 'a';
	bootdev = X68K_MAKESCSIBOOTDEV(X68K_MAJOR_CD, ha >> 4, ha & 15,
				       ID & 7, 0, 0);
#ifdef DEBUG
	printf(" boot device: %s\n", bootdevstr);
#endif

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

extern int xxboot(struct open_file *);
int
devopen(struct open_file *f, const char *fname, char **file)
{
	return xxopen(f);
}
