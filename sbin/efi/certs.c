/* $NetBSD: certs.c,v 1.4 2025/03/02 00:03:41 riastradh Exp $ */

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
__RCSID("$NetBSD: certs.c,v 1.4 2025/03/02 00:03:41 riastradh Exp $");
#endif /* not lint */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/efiio.h>

#include "efiio.h"
#include "defs.h"
#include "certs.h"

/*
 * See UEFI spec section 32.4
 */

#define EFI_CERT_SHA256_GUID \
	{0xc1c41626,0x504c,0x4092,0xac,0xa9,\
 	  {0x41,0xf9,0x36,0x93,0x43,0x28}}

#define EFI_CERT_RSA2048_GUID \
	{0x3c5766e8,0x269c,0x4e34,0xaa,0x14,\
	  {0xed,0x77,0x6e,0x85,0xb3,0xb6}}

#define EFI_CERT_RSA2048_SHA256_GUID \
	{0xe2b36190,0x879b,0x4a3d,0xad,0x8d,\
	  {0xf2,0xe7,0xbb,0xa3,0x27,0x84}}

#define EFI_CERT_SHA1_GUID \
	{0x826ca512,0xcf10,0x4ac9,0xb1,0x87,\
	  {0xbe,0x01,0x49,0x66,0x31,0xbd}}

#define EFI_CERT_RSA2048_SHA1_GUID \
	{0x67f8444f,0x8743,0x48f1,0xa3,0x28,\
	  {0x1e,0xaa,0xb8,0x73,0x60,0x80}}

#define EFI_CERT_X509_GUID \
	{0xa5c059a1,0x94e4,0x4aa7,0x87,0xb5,\
	  {0xab,0x15,0x5c,0x2b,0xf0,0x72}}

#define EFI_CERT_SHA224_GUID \
	{0x0b6e5233,0xa65c,0x44c9,0x94,0x07,\
	  {0xd9,0xab,0x83,0xbf,0xc8,0xbd}}

#define EFI_CERT_SHA384_GUID \
	{0xff3e5307,0x9fd0,0x48c9,0x85,0xf1,\
	  {0x8a,0xd5,0x6c,0x70,0x1e,0x01}}

#define EFI_CERT_SHA512_GUID \
	{0x093e0fae,0xa6c4,0x4f50,0x9f,0x1b,\
	  {0xd4,0x1e,0x2b,0x89,0xc1,0x9a}}

#define EFI_CERT_X509_SHA256_GUID \
	{0x3bd2a492,0x96c0,0x4079,0xb4,0x20,\
	  {0xfc,0xf9,0x8e,0xf1,0x03,0xed}}

#define EFI_CERT_X509_SHA384_GUID \
	{0x7076876e,0x80c2,0x4ee6,0xaa,0xd2,\
	  {0x28,0xb3,0x49,0xa6,0x86,0x5b}}

#define EFI_CERT_X509_SHA512_GUID \
	{0x446dbf63,0x2502,0x4cda,0xbc,0xfa,\
	  {0x24,0x65,0xd2,0xb0,0xfe,0x9d}}

#define EFI_CERT_EXTERNAL_MANAGEMENT_GUID \
	{0x452e8ced,0xdfff,0x4b8c,0xae,0x01,\
	  {0x51,0x18,0x86,0x2e,0x68,0x2c}}

#define EFI_CERT_GUIDS \
  _X(SHA256,		sigfn0, 	16 + 32)  \
  _X(RSA2048,		sigfn0, 	16 + 256) \
  _X(RSA2048_SHA256,	sigfn0, 	16 + 256) \
  _X(SHA1,		sigfn0, 	16 + 20)  \
  _X(RSA2048_SHA1,	sigfn0, 	16 + 256) \
  _X(X509,		sigfn0, 	0)	  \
  _X(SHA224,		sigfn0, 	16 + 28)  \
  _X(SHA384,		sigfn0, 	16 + 48)  \
  _X(SHA512,		sigfn0, 	16 + 64)  \
  _X(X509_SHA256,	sigfn256,	16 + 48)  \
  _X(X509_SHA384,	sigfn384,	16 + 64)  \
  _X(X509_SHA512,	sigfn512,	16 + 80)  \
  _X(EXTERNAL_MANAGEMENT, sigfn1,	16 + 1)

#define EFI_CERT_GUID_UNKNOWN	"unknown"

/************************************************************************/

typedef uint8_t EFI_SHA256_HASH[32];
typedef uint8_t EFI_SHA384_HASH[48];
typedef uint8_t EFI_SHA512_HASH[64];

typedef struct EFI_SIGNATURE_DAT {
	uuid_t			SignatureOwner;
	uint8_t			SignatureData[];
} __packed EFI_SIGNATURE_DATA_t;

typedef struct EFI_SIGNATURE_LIST {
	uuid_t			SignatureType;
	uint32_t		SignatureListSize;
	uint32_t		SignatureHeaderSize;
	uint32_t		SignatureSize;
	uint8_t			SignatureListBody[];
//	uint8_t			SignatureHeader[SignatureHeaderSize];
//	EFI_SIGNATURE_DATA	Signatures[][SignatureSize];
} __packed EFI_SIGNATURE_LIST_t;

typedef struct {
	uint16_t	Year;		// 1900 - 9999
	uint8_t		Month;		// 1 - 12
	uint8_t		Day;		// 1 - 31
	uint8_t		Hour;		// 0 - 23
	uint8_t		Minute;		// 0 - 59
	uint8_t		Second;		// 0 - 59
	uint8_t		Pad1;
	uint32_t	Nanosecond;	// 0 - 999,999,999
	int16_t		TimeZone;	// -1440 to 1440 or 2047 (0x7ff)
#define EFI_UNSPECIFIED_TIMEZONE	0x07FF
	uint8_t		Daylight;
#define EFI_TIME_ADJUST_DAYLIGHT	0x01
#define EFI_TIME_IN_DAYLIGHT		0x02
	uint8_t		Pad2;
} __packed EFI_TIME;

/************************************************************************/

static char *
show_time(const EFI_TIME *et, int indent)
{
	/*
	 * XXX: Deal with the Daylight flags!
	 */
	printf("%*s%u.%u.%u %u:%u:%u.%u",
	    indent, "",
	    et->Year,
	    et->Month,
	    et->Day,
	    et->Hour,
	    et->Minute,
	    et->Second,
	    et->Nanosecond);
	if (et->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
		printf(" (%d)", et->TimeZone);
	}
	printf("\n");

	return NULL;
}

/************************************************************************/

static int
sigfn0(const void *vp, size_t sz, int indent)
{
	const struct {
		uuid_t	uuid;
		uint8_t data[];
	} __packed *s = vp;

	printf("%*sOwner: ", indent, "");
	uuid_printf(&s->uuid);
	printf("\n");
	show_data(s->data, sz, "  ");
	return 0;
}

static int
sigfn1(const void *vp, size_t sz, int indent)
{
	const struct {
		uuid_t uuid;
		uint8_t zero;
	} __packed *s = vp;

	assert(sizeof(*s) == sizeof(s->uuid) + sz);
	printf("%*sOwner: ", indent, "");
	uuid_printf(&s->uuid);
	printf("\n");
	printf("%*szero: 0x%02x\n", indent, "", s->zero);
	return 0;
}

static int
sigfn256(const void *vp, size_t sz, int indent)
{
	const struct {
		uuid_t uuid;
		EFI_SHA256_HASH		ToBeSignedHash;
		EFI_TIME		TimeOfRevocation;
	} __packed *s = vp;

	assert(sizeof(*s) == sizeof(s->uuid) + sz);
	printf("%*sOwner: ", indent, "");
	uuid_printf(&s->uuid);
	printf("\n");
	show_data((const void *)&s->ToBeSignedHash, sizeof(s->ToBeSignedHash),
	    "  ");
	printf("%*sTimeOfRevocation: ", indent, "");
	show_time(&s->TimeOfRevocation, indent);
	return 0;
}

static int
sigfn384(const void *vp, size_t sz, int indent)
{
	const struct {
		uuid_t uuid;
		EFI_SHA384_HASH		ToBeSignedHash;
		EFI_TIME		TimeOfRevocation;
	} __packed *s = vp;

	assert(sizeof(*s) == sizeof(s->uuid) + sz);
	printf("%*sOwner: ", indent, "");
	uuid_printf(&s->uuid);
	printf("\n");
	show_data((const void *)&s->ToBeSignedHash, sizeof(s->ToBeSignedHash),
	    "  ");
	printf("%*sTimeOfRevocation: ", indent, "");
	show_time(&s->TimeOfRevocation, indent);
	return 0;
}

static int
sigfn512(const void *vp, size_t sz, int indent)
{
	const struct {
		uuid_t uuid;
		EFI_SHA512_HASH		ToBeSignedHash;
		EFI_TIME		TimeOfRevocation;
	} __packed *s = vp;

	assert(sizeof(*s) == sizeof(s->uuid) + sz);
	printf("%*sOwner: ", indent, "");
	uuid_printf(&s->uuid);
	printf("\n");
	show_data((const void *)&s->ToBeSignedHash, sizeof(s->ToBeSignedHash),
	    "  ");
	printf("%*sTimeOfRevocation: ", indent, "");
	show_time(&s->TimeOfRevocation, indent);
	return 0;
}

/************************************************************************/

struct cert_tbl {
	uuid_t guid;
	const char *name;
	int (*sigfn)(const void *, size_t, int);
	size_t sigsz;
};

static int
sortcmpfn(const void *a, const void *b)
{
	const struct cert_tbl *p = a;
	const struct cert_tbl *q = b;

	return memcmp(&p->guid, &q->guid, sizeof(p->guid));
}

static int
srchcmpfn(const void *a, const void *b)
{
	const struct cert_tbl *q = b;

	return memcmp(a, &q->guid, sizeof(q->guid));
}

static struct cert_tbl *
get_cert_info(uuid_t *uuid)
{
	static bool init_done = false;
	static struct cert_tbl tbl[] = {
#define _X(c,f,s)	{ .guid = EFI_CERT_ ## c ## _GUID, .name = #c, \
 			  .sigfn = f, .sigsz = s, },
		EFI_CERT_GUIDS
#undef _X
	};
	struct cert_tbl *tp;

	if (!init_done) {
		qsort(tbl, __arraycount(tbl), sizeof(*tbl), sortcmpfn);
		init_done = true;
	}

	tp = bsearch(uuid, tbl, __arraycount(tbl), sizeof(*tbl), srchcmpfn);
	if (tp == NULL) {
		printf("unknown owner GUID: ");
		uuid_printf(uuid);
		printf("\n");
	}
	return tp;
}

/************************************************************************/

static inline const char *
cert_info_name(struct cert_tbl *tp)
{

	return tp ? tp->name : EFI_CERT_GUID_UNKNOWN;
}

PUBLIC const char *
get_cert_name(uuid_t *uuid)
{

	return cert_info_name(get_cert_info(uuid));
}

static struct cert_tbl *
show_signature_list_header(EFI_SIGNATURE_LIST_t *lp, int indent)
{
	struct cert_tbl *tp;
	const char *name;

	tp = get_cert_info(&lp->SignatureType);
	name = cert_info_name(tp);
	printf("%*sSigType: %s\n", indent, "", name);
	printf("%*sListSize: %d (0x%x)\n", indent, "",
	    lp->SignatureListSize, lp->SignatureListSize);
	printf("%*sHdrSize: %d (0x%x)\n", indent, "",
	    lp->SignatureHeaderSize, lp->SignatureHeaderSize);
	printf("%*sSigSize: %d (0x%x)\n", indent, "",
	    lp->SignatureSize, lp->SignatureSize);

	return tp;
}

static int
parse_signature_list(const uint8_t *bp, size_t sz, int indent)
{
	union {
		const uint8_t *bp;
		EFI_SIGNATURE_LIST_t *lp;
	} u;
	struct cert_tbl *tp;
	const uint8_t *endall, *endlist;

	u.bp = bp;
	endall = bp + sz;
	while (u.bp < endall) {
		tp = show_signature_list_header(u.lp, indent);

		/*
		 * XXX: all the documented cases seem to have no
		 * signature header.
		 */
		if (u.lp->SignatureHeaderSize != 0) {
			printf("expected zero SignatureHeaderSize: got %d\n",
			    u.lp->SignatureHeaderSize);
		}
		assert(u.lp->SignatureHeaderSize == 0);

		/*
		 * Sanity check.
		 */
		if (tp && tp->sigsz && tp->sigsz != u.lp->SignatureSize) {
			printf("expected signature size: %zu, got: %u\n",
			    tp->sigsz, u.lp->SignatureSize);
		}

		endlist = u.bp + u.lp->SignatureListSize;
		for (uint8_t *sp = u.lp->SignatureListBody
			 + u.lp->SignatureHeaderSize;
		     sp < endlist;
		     sp += u.lp->SignatureSize) {
			if (tp)
				tp->sigfn(sp, u.lp->SignatureSize, 2);
			else
				sigfn0(sp, u.lp->SignatureSize, 2);
		}

		u.bp = endlist;
	}
	return 0;
}

PUBLIC int
show_cert_data(efi_var_t *v, bool dbg)
{
	union {
		const uint8_t *bp;
		EFI_SIGNATURE_LIST_t *lp;
	} u;
	const uint8_t *end;
	const char *name;

	printf("%s: ", v->name);

	u.bp = v->ev.data;
	end = u.bp + v->ev.datasize;
	for (;;) {
		name = get_cert_name(&u.lp->SignatureType);

		u.bp += u.lp->SignatureListSize;
		if (u.bp < end)
			printf("%s ", name);
		else {
			printf("%s\n", name);
			break;
		}
	}

	if (dbg)
		parse_signature_list(v->ev.data, v->ev.datasize, 2);

	return 0;
}
