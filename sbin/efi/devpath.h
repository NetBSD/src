/* $NetBSD: devpath.h,v 1.3 2025/03/02 00:03:41 riastradh Exp $ */

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

#ifndef _DEVPATH_H_
#define _DEVPATH_H_

#ifndef lint
__RCSID("$NetBSD: devpath.h,v 1.3 2025/03/02 00:03:41 riastradh Exp $");
#endif /* not lint */

#include <stdlib.h>
#include <util.h>

typedef struct EFI_DEVICE_PATH_PROTOCOL {
	uint8_t Type;
	uint8_t SubType;
//	uint8_t Length[2];
	uint16_t Length;
//	uint8_t	Data[];
} EFI_DEVICE_PATH_PROTOCOL;

typedef EFI_DEVICE_PATH_PROTOCOL devpath_t;

typedef struct devpath_elm {
	size_t sz;
	char *cp;
} devpath_elm_t;

enum {
	DEVPATH_TYPE_HW    = 1,
	DEVPATH_TYPE_ACPI  = 2,
	DEVPATH_TYPE_MSG   = 3,
	DEVPATH_TYPE_MEDIA = 4,
	DEVPATH_TYPE_BIOS  = 5,
	DEVPATH_TYPE_END   = 0x7f,
};

#define DEVPATH_END_ALL \
  ((EFI_DEVICE_PATH_PROTOCOL){.Type = 0x7f, .SubType = 0xff, .Length = 0x04})
#define DEVPATH_END_SEG \
  ((EFI_DEVICE_PATH_PROTOCOL){.Type = 0x7f, .SubType = 0x01, .Length = 0x04})

/*
 * For debugging
 */
#define DEVPATH_FMT_HDR	"  DevPath_Hdr: Type %u, SubType: %u, Length: %u\n"
#define DEVPATH_FMT_PFX	"    "
#define DEVPATH_FMT(s)	DEVPATH_FMT_PFX #s
#define DEVPATH_DAT_HDR(dp)	(dp)->Type, (dp)->SubType, (dp)->Length

static inline char *
aconcat(char *bp1, const char *sep, char *bp2)
{
	char *bp;

	easprintf(&bp, "%s%s%s", bp1, sep, bp2);
	free(bp1);
	free(bp2);
	return bp;
}

static inline const char *
devpath_type_name(size_t type)
{
	static const char *type_tbl[] = {
		[DEVPATH_TYPE_HW]    = "HW",
		[DEVPATH_TYPE_ACPI]  = "ACPI",
		[DEVPATH_TYPE_MSG]   = "MSG",
		[DEVPATH_TYPE_MEDIA] = "MEDIA",
		[DEVPATH_TYPE_BIOS]  = "BIOS",
	};

	if (type == DEVPATH_TYPE_END)
		return "END";

	if (type > __arraycount(type_tbl) || type == 0)
		return "UNKNOWN";

	return type_tbl[type];

}

static inline void
devpath_hdr(devpath_t *dp, devpath_elm_t *elm)
{

	elm->sz = (size_t)easprintf(&elm->cp,
	    DEVPATH_FMT_HDR, DEVPATH_DAT_HDR(dp));
}

static inline void
devpath_unsupported(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	path->sz = (size_t)easprintf(&path->cp,
	    "[unsupported devpath type: %s(%u), subtype: %u]",
	    devpath_type_name(dp->Type), dp->Type, dp->SubType);

	if (dbg != NULL)
		devpath_hdr(dp, dbg);
}

char *devpath_parse(devpath_t *, size_t, char **);

#endif /* _DEVPATH_H_ */
