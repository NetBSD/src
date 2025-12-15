/* $NetBSD: devpath5.c,v 1.3 2025/12/15 17:06:42 joe Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: devpath5.c,v 1.3 2025/12/15 17:06:42 joe Exp $");
#endif /* not lint */

#include <assert.h>
#include <stdio.h>

#include "defs.h"
#include "devpath.h"
#include "devpath5.h"

#define easprintf	(size_t)easprintf

/************************************************************************
 * Type 5 - BIOS Boot Specification Device Path
 ************************************************************************/

/*
 * See Appendix A of BIOSBootSpecsV1.01.pdf for typename and statusflag.
 * Available from <http:/www.uefi.org/uefi>
 */
static inline const char *
devpath_bios_typename(uint devtype)
{

	switch (devtype) {
	case 0x01:	return "Floppy";
	case 0x02:	return "HardDisk";
	case 0x03:	return "CDRom";
	case 0x04:	return "PCMCIA";
	case 0x05:	return "USB";
	case 0x06:	return "Network";
	case 0x80:	return "BEV device";
	case 0xff:	return "Unknown";
	default:	return "Reserved";
	}
}

#define BIOS_STATUS_OLD_POSITION	__BITS(3,0)
#define BIOS_STATUS_ENABLED		__BIT(8)
#define BIOS_STATUS_FAILED		__BIT(9)
#define BIOS_STATUS_MEDIA		__BITS(11,10)
#define BIOS_STATUS_MEDIA_NONE		__SHIFTIN(0, BIOS_STATUS_MEDIA)
#define BIOS_STATUS_MEDIA_UNKNOWN	__SHIFTIN(1, BIOS_STATUS_MEDIA)
#define BIOS_STATUS_MEDIA_PRESENT	__SHIFTIN(2, BIOS_STATUS_MEDIA)
#define BIOS_STATUS_MEDIA_RESVD		__SHIFTIN(3, BIOS_STATUS_MEDIA)
#define BIOS_STATUS_RESERVED		(__BITS(15,12) | __BITS(7,4))
#define BIOS_STATUS_BITS \
	"\177\020"		\
	"f\x00\x04""POS\0"	\
	"b\x08""ENBL\0"		\
	"b\x09""FAIL\0"		\
	"f\x0a\x02""MEDIA\0"	\
	"=\0""NONE\0"		\
	"=\1""UNKNOWN\0"	\
	"=\2""PRESENT\0"	\
	"=\3""RSVD\0"

/* Bios Boot Specification Device */
static inline void
devpath_bios_BBS(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.6 */
	struct { /* Sub-Type 1 */
		devpath_t	hdr;	/* Length = 8 */
		uint16_t	DeviceType; /* see devpath_bios_typename() */
		uint16_t	StatusFlag;
		char		Description[]; /* asciiz */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 8);
	const char *typename;

	typename = devpath_bios_typename(p->DeviceType);

	path->sz = easprintf(&path->cp, "BBS(%s(%#x),0x%04x,%s)",
	    typename, p->DeviceType, p->StatusFlag, p->Description);

	if (dbg != NULL) {
		char statusflag[128];

		snprintb(statusflag, sizeof(statusflag), BIOS_STATUS_BITS,
		    p->StatusFlag);
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(DeviceType: %x (%s)\n)
		    DEVPATH_FMT(StatusFlag: %s\n)
		    DEVPATH_FMT(Description: '%s'\n),
		    DEVPATH_DAT_HDR(dp),
		    p->DeviceType,
		    typename,
		    statusflag,
		    p->Description);
	}
}

PUBLIC void
devpath_bios(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	assert(dp->Type == 5);

	switch (dp->SubType) {
	case 1:   devpath_bios_BBS(dp, path, dbg);	return;
	default:  devpath_unsupported(dp, path, dbg);	return;
	}
}
