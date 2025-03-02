/* $NetBSD: devpath2.c,v 1.6 2025/03/02 14:18:04 riastradh Exp $ */

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
__RCSID("$NetBSD: devpath2.c,v 1.6 2025/03/02 14:18:04 riastradh Exp $");
#endif /* not lint */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <util.h>

#include "defs.h"
#include "devpath.h"
#include "devpath2.h"
#include "utils.h"

#define easprintf	(size_t)easprintf

/************************************************************************
 * Type 2 - ACPI Device Path
 ************************************************************************/

/*
 * See 19.3.4 of ACPI Specification, Release 6.5 for compressed EISAID
 * algorithm.
 */

#define PNP0A03		0x0a0341d0
#define PNP0A08		0x0a0841d0

static const char *
eisaid_to_str(uint32_t eisaid)
{
	static const char hexdigits[] = "0123456789ABCDEF";
	static char text[8];
	union {
		uint32_t eisaid;
		uint8_t b[4];
	} u;

	u.eisaid = eisaid;

	text[0]  = (char)__SHIFTOUT(u.b[0], __BITS(4,0));
	text[1]  = (char)__SHIFTOUT(u.b[0], __BITS(7,5));
	text[1] |= (char)__SHIFTOUT(u.b[1], __BITS(1,0)) << 3;
	text[2]  = (char)__SHIFTOUT(u.b[1], __BITS(7,2));

	text[0] += 0x40;	/* '@' */
	text[1] += 0x40;
	text[2] += 0x40;

#define hi_nib(v)	(((v) >> 4) & 0x0f)
#define lo_nib(v)	(((v) >> 0) & 0x0f)
	text[3] = hexdigits[hi_nib(u.b[3])];
	text[4] = hexdigits[lo_nib(u.b[3])];
	text[5] = hexdigits[hi_nib(u.b[2])];
	text[6] = hexdigits[lo_nib(u.b[2])];
	text[7] = 0;
#undef lo_nib
#undef hi_nib

	return text;
}

static inline const char *
devpath_acpi_acpi_eisaid(uint32_t eisaid)
{

#define PNP(v)	(0x000041d0 | ((v) << 16))
	switch (eisaid) {
//	case PNP(0x0f03):	return "PS2Mouse";	/* legacy? */
//	case PNP(0x0303):	return "FloppyPNP";	/* legacy? */
	case PNP(0x0a03):	return "PciRoot";
	case PNP(0x0a08):	return "PcieRoot";
	case PNP(0x0604):	return "Floppy";
	case PNP(0x0301):	return "Keyboard";
	case PNP(0x0501):	return "Serial";
	case PNP(0x0401):	return "ParallelPort";
	default:		return NULL;
	}
#undef PNP
}

static inline void
devpath_acpi_acpi(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.3 */
	struct {			/* Sub-Type 1 */
		devpath_t	hdr;	/* Length = 12 */
		uint32_t	_HID;
		uint32_t	_UID;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 12);
	const char *str;

	str = devpath_acpi_acpi_eisaid(p->_HID);
	if (str == NULL) {
		path->sz = easprintf(&path->cp, "ACPI(%s,0x%x)",
		    eisaid_to_str(p->_HID), p->_UID);
	} else {
		path->sz = easprintf(&path->cp, "%s(0x%x)", str, p->_UID);
	}

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(_HID: 0x%08x(%s)\n)
		    DEVPATH_FMT(_UID: 0x%08x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->_HID,
		    eisaid_to_str(p->_HID),
		    p->_UID);
	}
}

static inline void
devpath_acpi_acpiex(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.3 */
	struct {			/* Sub-Type 2 */
		devpath_t	hdr;	/* Length = 19 + n */
		uint32_t	_HID;
		uint32_t	_UID;
		uint32_t	_CID;
		char		_HIDSTR[];	/* at least NUL byte */
//		char		_UIDSTR[];	/* at least NUL byte */
//		char		_CIDSTR[];	/* at least NUL byte */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 16);
	char *hidstr = p->_HIDSTR;
	char *uidstr = hidstr + strlen(hidstr) + 1;
	char *cidstr = uidstr + strlen(uidstr) + 1;

#if 0
	There are 4 different subsubtypes depending on the ?ID, ?IDSTR values.

	"AcpiEx"
	"AcpiExp"
	"PciRoot"
	"PcipRoot"
#endif
	if (p->_HID == PNP0A08 || p->_CID == PNP0A08) {
		// PcieRoot(UID|UIDSTR)
		path->sz = easprintf(&path->cp, "PcieRoot(%s)",
		    *uidstr != '\0' ? uidstr : eisaid_to_str(p->_UID));
	}
	else if (p->_HID == PNP0A03 || p->_CID == PNP0A03) {
		assert(p->_HID != PNP0A08);
		// PciRoot(UID|UIDSTR)
		path->sz = easprintf(&path->cp, "PciRoot(%s)",
		    *uidstr != '\0' ? uidstr : eisaid_to_str(p->_UID));
	}
	else if (hidstr[0] == '\0' && cidstr[0] == '\0' && uidstr[0] != '\0') {
		assert(p->_HID != 0);
		path->sz = easprintf(&path->cp, "AcpiExp(%s,%s,%s)",
		    eisaid_to_str(p->_HID),
		    eisaid_to_str(p->_CID),
		    uidstr);
	}
	else {
		path->sz = easprintf(&path->cp, "ACPIEX(%s,%s,0x%x,%s,%s,%s)",
		    eisaid_to_str(p->_HID), eisaid_to_str(p->_CID),
		    p->_UID, hidstr, cidstr, uidstr);
	}

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(_HID: 0x%08x(%s)\n)
		    DEVPATH_FMT(_UID: 0x%08x\n)
		    DEVPATH_FMT(_CID: 0x%08x(%s)\n)
		    DEVPATH_FMT(_HIDSTR: %s\n)
		    DEVPATH_FMT(_UIDSTR: %s\n)
		    DEVPATH_FMT(_CIDSTR: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    p->_HID,
		    eisaid_to_str(p->_HID),
		    p->_UID,
		    p->_CID,
		    eisaid_to_str(p->_CID),
		    hidstr,
		    uidstr,
		    cidstr);
	}
}

static inline void
devpath_acpi_adr(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.3.1 */
	struct {		/* Sub-Type 3 */
		devpath_t	hdr;	/* Length = 8; at least one _ADR */
		uint32_t	_ADR[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 4);
	char *tp;
	size_t cnt;

	cnt = (p->hdr.Length - sizeof(p->hdr)) / sizeof(p->_ADR[0]);

	tp = estrdup("AcpiAdr(");
	for (size_t i = 0; i < cnt; i++) {
		path->sz = easprintf(&path->cp, "%s0x%08x%s", tp, p->_ADR[i],
		    i + 1 < cnt ? "," : "");
		free(tp);
		tp = path->cp;
	}
	path->sz = easprintf(&path->cp, "%s)", tp);
	free(tp);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR,
		    DEVPATH_DAT_HDR(dp));
		tp = dbg->cp;
		for (size_t i = 0; i < cnt; i++) {
			dbg->sz = easprintf(&dbg->cp, "%s"
			    DEVPATH_FMT(_ADR[%zu]: 0x%08x\n),
			    tp, i, p->_ADR[i]);
			free(tp);
			tp = dbg->cp;
		}
	}
}

static inline void
devpath_acpi_nvdimm(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.3.2 */
	struct {		/* Sub-Type 4 */
		devpath_t	hdr;	/* Length = 8 */
		uint32_t	NFIT_DevHdl;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 8);

	path->sz = easprintf(&path->cp, "NvdimmAcpiAdr(0x%x)", p->NFIT_DevHdl);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(NFIT_DevHdl: 0x%08x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->NFIT_DevHdl);
	}
}

#ifdef notdef
static inline void
devpath_acpi_unknown(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	path->sz = easprintf(&path->cp, "Msg(%d,%s)", dp->SubType,
	    encode_data((uint8_t *)(dp + 1), dp->Length - sizeof(*dp)));

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR,
		    DEVPATH_DAT_HDR(dp));
	}
}
#endif

PUBLIC void
devpath_acpi(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	assert(dp->Type = 2);

	switch (dp->SubType) {
	case 1:   devpath_acpi_acpi(dp, path, dbg);	return;
	case 2:   devpath_acpi_acpiex(dp, path, dbg);	return;
	case 3:   devpath_acpi_adr(dp, path, dbg);	return;
	case 4:   devpath_acpi_nvdimm(dp, path, dbg);	return;
	default:  devpath_unsupported(dp, path, dbg);	return;
	}
}
