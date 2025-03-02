/* $NetBSD: showvar.c,v 1.5 2025/03/02 01:07:11 riastradh Exp $ */

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
__RCSID("$NetBSD: showvar.c,v 1.5 2025/03/02 01:07:11 riastradh Exp $");
#endif /* not lint */

#include <sys/efiio.h>
#include <sys/ioctl.h>
#include <sys/uuid.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "efiio.h"
#include "defs.h"
#include "bootvar.h"
#include "certs.h"
#include "devpath.h"
#include "getvars.h"
#include "showvar.h"
#include "utils.h"

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI		   __BIT(0)
#define EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION 	   __BIT(1)
#define EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED __BIT(2)
#define EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED 	   __BIT(3)
#define EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED	   __BIT(4)
#define EFI_OS_INDICATIONS_START_OS_RECOVERY		   __BIT(5)
#define EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY	   __BIT(6)
#define EFI_OS_INDICATIONS_JSON_CONFIG_DATA_REFRESH	   __BIT(7)
#define OS_INDICATIONS_BITS		\
	"\177\020"			\
	"b\x0""BootFWui\0"		\
	"b\x1""TimeStamp\0"		\
	"b\x2""FileCap\0"		\
	"b\x3""FMPCap\0"		\
	"b\x4""CapResVar\0"		\
	"b\x5""StartOsRecovery\0"	\
	"b\x6""StartPltformRec\0"	\
	"b\x7""JSONConfig\0"		\

//********************************************************
// Boot Option Attributes
//********************************************************
#define EFI_BOOT_OPTION_SUPPORT_KEY	__BIT(0)	// 0x00000001
#define EFI_BOOT_OPTION_SUPPORT_APP	__BIT(1)	// 0x00000002
#define EFI_BOOT_OPTION_SUPPORT_SYSPREP	__BIT(4)	// 0x00000010
#define EFI_BOOT_OPTION_SUPPORT_COUNT	__BITS(8,9)	// 0x00000300
// All values 0x00000200-0x00001F00 are reserved
#define BOOT_OPTION_SUPPORT_BITS \
	"\177\020"		\
	"b\x0""Key\0"		\
	"b\x1""App\0"		\
	"b\x4""SysPrep\0"	\
	"f\x8\x2""Count\0"

/************************************************************************/

static int
show_filelist_data(efi_var_t *v, bool dbg)
{
	char *dmsg, *path;

	path = devpath_parse(v->ev.data, v->ev.datasize, dbg ? &dmsg : NULL);
	printf("%s: %s\n", v->name, path);
	free(path);
	if (dbg) {
		printf("%s", dmsg);
		free(dmsg);
	}
	return 0;
}

static int
show_asciiz_data(efi_var_t *v, bool dbg __unused)
{
	char *cp, *ep;

	printf("%s: ", v->name);

	cp = v->ev.data;
	ep = cp + v->ev.datasize;
	for (/*EMPTY*/; cp < ep; cp += strlen(cp) + 1) {
		printf("%s\n", cp);
	}
	if (cp != ep)
		warnx("short asciiz data\n");
	return 0;
}

static int
show_array8_data(efi_var_t *v, bool dbg __unused)
{
	size_t cnt, i;
	uint8_t *array = v->ev.data;

	printf("%s: ", v->name);

	cnt = v->ev.datasize / sizeof(array[0]);
	i = 0;
	for (;;) {
		printf("%02x", array[i]);
		if (++i == cnt) {
			printf("\n");
			break;
		}
		printf(",");
	}
	return 0;
}

static int
show_array16_data(efi_var_t *v, bool dbg __unused)
{
	size_t cnt, i;
	uint16_t *array = v->ev.data;

	printf("%s: ", v->name);

	cnt = v->ev.datasize / sizeof(array[0]);
	i = 0;
	for (;;) {
		printf("%04X", array[i]);
		if (++i == cnt) {
			printf("\n");
			break;
		}
		printf(",");
	}
	return 0;
}

static int
show_uuid_array_data(efi_var_t *v, bool dbg)
{
	size_t cnt, i;
	uuid_t *array = v->ev.data;
	const char *name;

	printf("%s: ", v->name);

	cnt = v->ev.datasize / sizeof(array[0]);

	for (i = 0; i < cnt; i++) {
		name = get_cert_name(&array[i]);
		printf("%s%c", name, i + 1 < cnt ? ' ' : '\n');
	}

	if (dbg) {
		for (i = 0; i < cnt; i++) {
			printf("  ");
			uuid_printf(&array[i]);
			name = get_cert_name(&array[i]);
			printf("  %s\n", name ? name : "unknown");
		}
	}
	return 0;
}

/************************************************************************/

static int
show_key_data(efi_var_t *v, bool dbg)
{
	typedef union {
		struct {
			uint32_t	Revision	: 8;
			uint32_t	ShiftPressed	: 1;
			uint32_t	ControlPressed	: 1;
			uint32_t	AltPressed	: 1;
			uint32_t	LogoPressed	: 1;
			uint32_t	MenuPressed	: 1;
			uint32_t	SysReqPressed	: 1;
			uint32_t	Reserved	: 16;
			uint32_t	InputKeyCount	: 2;
#define BOOT_KEY_OPTION_BITS \
	"\177\020"		\
	"f\x00\x08""Rev\0"	\
	"b\x08""SHFT\0"		\
	"b\x09""CTRL\0"		\
	"b\x0a""ALT\0"		\
	"b\x0b""LOGO\0"		\
	"b\x0c""MENU\0"		\
	"b\x0d""SysReq\0"	\
     /* "f\x0e\x10""Rsvd\0" */	\
	"f\x1e\x02""KeyCnt\0"
		} Options;
		uint32_t		PackedValue;
	} __packed EFI_BOOT_KEY_DATA;
	typedef struct {
		uint16_t		ScanCode;
		uint16_t		UnicodeChar;
	} __packed EFI_INPUT_KEY;
	typedef struct _EFI_KEY_OPTION {
		EFI_BOOT_KEY_DATA	KeyData;
		uint32_t		BootOptionCrc;
		uint16_t		BootOption;
		EFI_INPUT_KEY		Keys[3];
		uint16_t		DescLocation; /* XXX: a guess!  Not documented */
		uint8_t			UndocData[];
	} __packed EFI_KEY_OPTION;
	union {
		EFI_KEY_OPTION *ko;
		uint8_t        *bp;
	} u = { .bp = v->ev.data, };
	char buf[256], c, *cp, *desc;
	uint i;

	printf("%s:", v->name);

	/*
	 * Note: DescLocation is not documented in the UEFI spec, so
	 * do some sanity checking before using it.
	 */
	desc = NULL;
	if (offsetof(EFI_KEY_OPTION, UndocData) < v->ev.datasize &&
	    u.ko->DescLocation + sizeof(uint16_t) < v->ev.datasize) {
		size_t sz = v->ev.datasize - u.ko->DescLocation;
		desc = ucs2_to_utf8((uint16_t *)&u.bp[u.ko->DescLocation],
		    sz, NULL, NULL);
		printf(" %s", desc);
	}

	/*
	 * Parse the KeyData
	 */
	snprintb(buf, sizeof(buf), BOOT_KEY_OPTION_BITS,
	    u.ko->KeyData.PackedValue);

	/*
	 * Skip over the raw value
	 */
	c = '\0';
	if ((cp = strchr(buf, ',')) || (cp = strchr(buf, '<'))) {
		c = *cp;
		*cp = '<';
	}
	else
		cp = buf;

	printf("\tBoot%04X \t%s", u.ko->BootOption, cp);

	if (c != '\0')
		*cp = c;	/* restore the buffer */

	for (i = 0; i < u.ko->KeyData.Options.InputKeyCount; i++)
		printf(" {%04x, %04x}", u.ko->Keys[i].ScanCode,
		    u.ko->Keys[i].UnicodeChar);
	printf("\n");

	if (dbg) {
		printf("  KeyData: %s\n", buf);
		printf("  BootOptionCrc: 0x%08x\n", u.ko->BootOptionCrc);
		printf("  BootOption:    Boot%04X\n", u.ko->BootOption);
		for (i = 0; i < u.ko->KeyData.Options.InputKeyCount; i++) {
			printf("  Keys[%u].ScanCode:    0x%04x\n", i,
			    u.ko->Keys[i].ScanCode);
			printf("  Keys[%u].UnicodeChar: 0x%04x\n", i,
			    u.ko->Keys[i].UnicodeChar);
		}
		if (desc)
			printf("  Desc: %s\n", desc);
	}
	free(desc);
	return 0;
}

/************************************************************************/

static char *
format_optional_data(char *od, size_t sz)
{
	char *bp;
	size_t i;

	bp = emalloc(sz + 1);

	for (i = 0; i < sz; i++) {
		char c = od[i];
		bp[i] = isprint((unsigned char)c) ? c : '.';
	}
	bp[i] = '\0';
	return bp;
}

static int
show_boot_data(efi_var_t *v, uint debug, uint max_namelen)
{
	struct {
		char *name;
		uint32_t Attributes;
		char *Description;
		devpath_t *devpath;
		char *OptionalData;
		size_t OptionalDataSize;
	} info;
	union {
		char *cp;
		boot_var_t *bb;
	} u = { .cp = v->ev.data, };
	char *args, *dmsg, *path;
	size_t sz;
	bool dbg = debug & DEBUG_STRUCT_BIT;
	bool verbose = debug & (DEBUG_MASK | DEBUG_VERBOSE_BIT);

	memset(&info, 0, sizeof(info));
	info.name = v->name;
	info.Attributes = u.bb->Attributes;
	sz = (v->ev.datasize - sizeof(*u.bb)) / sizeof(uint16_t);
	sz = (ucs2nlen(u.bb->Description, sz) + 1) * sizeof(uint16_t);
	info.Description = ucs2_to_utf8(u.bb->Description, sz, NULL, NULL);
	info.devpath = (devpath_t *)((uint8_t *)u.bb->Description + sz);
	info.OptionalData = (char *)info.devpath + u.bb->FilePathListLength;

	char *ep = u.cp + v->ev.datasize;

	assert(info.OptionalData <= u.cp + v->ev.datasize);

	if (info.OptionalData <= u.cp + v->ev.datasize) {
		info.OptionalDataSize = (size_t)(ep - info.OptionalData);
	}
	else {
		printf("ARG!!! "
		    "FilePahList[] extends past end of data by %zd bytes\n",
		    info.OptionalData - ep);
		info.OptionalDataSize = 0;
	}
	printf("%s%c %-*s", v->name,
	    IS_ACTIVE(info.Attributes) ? '*' : ' ',
	    max_namelen, info.Description);

	dmsg = NULL;
	if (verbose) {
		path = devpath_parse(info.devpath, u.bb->FilePathListLength,
		    dbg ? &dmsg : NULL);

		args = format_optional_data(info.OptionalData,
		    info.OptionalDataSize);

		printf("\t%s%s", path, args);/* XXX: make this conditional on verbose? */
		free(args);
		free(path);
	}

	printf("\n");

	if (dbg) {
		char attr_str[256];

		snprintb(attr_str, sizeof(attr_str),
		    LOAD_OPTION_BITS, info.Attributes);
		printf("  Attr: %s\n", attr_str);
		printf("  Description: %s\n", info.Description);
		assert(dmsg != NULL);
		printf("%s", dmsg);
		if (info.OptionalDataSize > 0) {
			show_data((void *)info.OptionalData,
			    info.OptionalDataSize, "  ExtraData: ");
		}
		free(dmsg);
	}

	free(info.Description);
	return 0;
}

/************************************************************************/

static int
show_OsIndications_data(efi_var_t *e, bool dbg __unused)
{
	uint64_t OsIndications;
	char buf[256];

	assert(e->ev.datasize == 8);
	OsIndications = *(uint64_t *)e->ev.data;
	snprintb(buf, sizeof(buf), OS_INDICATIONS_BITS, OsIndications);
	printf("%s:\t%s\n", e->name, buf);
	return 0;
}

static int
show_BootOptionSupport_data(efi_var_t *e, bool dbg __unused)
{
	uint32_t boot_option_support;
	char buf[256];

	assert(e->ev.datasize == 4);
	boot_option_support = *(uint32_t *)e->ev.data;
	snprintb(buf, sizeof(buf), BOOT_OPTION_SUPPORT_BITS,
	    boot_option_support);
	printf("%s:\t%s\n", e->name, buf);
	return 0;
}

static int
show_Timeout_data(efi_var_t *e, bool dbg __unused)
{
	uint16_t *timeout = e->ev.data;

	if (e->ev.datasize != 2)
		printf("bad timeout datasize: %zu\n", e->ev.datasize);
	else
		printf("Timeout: %u seconds\n", *timeout);
	return 0;
}

PUBLIC int
show_generic_data(efi_var_t *e, uint var_width)
{
	char uuid_str[UUID_STR_LEN];
	char attr_str[256];

	uuid_snprintf(uuid_str, sizeof(uuid_str), &e->ev.vendor);
	snprintb(attr_str, sizeof(attr_str), EFI_VAR_ATTR_BITS, e->ev.attrib);
	printf("%-*s  %s %5zu %s\n", var_width, e->name, uuid_str,
	    e->ev.datasize, attr_str);

	return 0;
}

/************************************************************************/

struct vartbl {
	const char *name;
	int (*fn)(efi_var_t *, bool);
};

static int
varcmpsortfn(const void *a, const void *b)
{
	const struct vartbl *p = a;
	const struct vartbl *q = b;

	return strcmp(p->name, q->name);
}

static int
varcmpsrchfn(const void *a, const void *b)
{
	const struct vartbl *q = b;

	return strcmp(a, q->name);
}

PUBLIC int
show_variable(efi_var_t *v, uint debug, uint max_namelen)
{
#define REGEXP_BOOTXXXX	"^((Key)|(Boot)|(lBoot)|(Driver)|(SysPrep)|(OsRecovery))[0-9,A-F]{4}$"
	static regex_t preg = { .re_magic = 0, };
	static struct vartbl *tp, tbl[] = {
		{ "AuditMode",			show_array8_data, },
		{ "BootCurrent",		show_array16_data, },
		{ "BootNext",			show_array16_data, },
		{ "BootOptionSupport",		show_BootOptionSupport_data, },
		{ "BootOrder",			show_array16_data, },
		{ "BootOrderDefault",		show_array16_data, },
		{ "db",				show_cert_data, },
		{ "dbDefault",			show_cert_data, },
		{ "dbr",			show_cert_data, },
		{ "dbrDefault",			show_cert_data, },
		{ "dbt",			show_cert_data, },
		{ "dbtDefault",			show_cert_data, },
		{ "dbx",			show_cert_data, },
		{ "dbxDefault",			show_cert_data, },
		{ "devdbDefault",		show_cert_data, },
		{ "ConIn",			show_filelist_data, },
		{ "ConInDev",			show_filelist_data, },
		{ "ConOut",			show_filelist_data, },
		{ "ConOutDev",			show_filelist_data, },
		{ "DriverOrder",		show_array16_data, },
		{ "ErrOut",			show_filelist_data, },
		{ "ErrOutDev",			show_filelist_data, },
		{ "KEK",			show_cert_data, },
		{ "KEKDefault",			show_cert_data, },
		{ "OsIndications",		show_OsIndications_data, },
		{ "OsIndicationsSupported",	show_OsIndications_data, },
		{ "PK",				show_cert_data, },
		{ "PKDefault",			show_cert_data, },
		{ "PlatformLang",		show_asciiz_data, },
		{ "PlatformLangCodes",		show_asciiz_data, },
		{ "ProtectedBootOptions",	show_array16_data, },
		{ "SecureBoot",			show_array8_data, },
		{ "SetupMode",			show_array8_data, },
		{ "SignatureSupport",		show_uuid_array_data, },
		{ "SysPrepOrder",		show_array16_data, },
		{ "Timeout",			show_Timeout_data, },
		{ "VendorKeys",			show_array8_data, },
	};
	bool dbg = debug & DEBUG_STRUCT_BIT;
	int rv;

	if (preg.re_magic == 0) {
		const char *regexp = REGEXP_BOOTXXXX;
		if (regcomp(&preg, regexp, REG_EXTENDED) != 0)
			err(EXIT_FAILURE, "regcomp: %s", regexp);

		qsort(tbl, __arraycount(tbl), sizeof(*tbl), varcmpsortfn);
	}

	if (debug & DEBUG_EFI_IOC_BIT) {
		rv = show_generic_data(v, max_namelen);
		if (debug & DEBUG_VERBOSE_BIT)
			return rv;
	}

	if (regexec(&preg, v->name, 0, NULL, 0) == 0) { /* matched */
		if (v->name[0] == 'K')
			rv = show_key_data(v, dbg);
		else
			rv = show_boot_data(v, debug, max_namelen);
	}
	else {
		tp = bsearch(v->name, tbl, __arraycount(tbl), sizeof(*tbl),
		    varcmpsrchfn);

		if (tp != NULL)
			rv = tp->fn(v, dbg);
		else if(!(debug & DEBUG_EFI_IOC_BIT))
			rv = show_generic_data(v, max_namelen);
	}
	if (debug & DEBUG_DATA_BIT)
		show_data(v->ev.data, v->ev.datasize, "  ");

	return rv;
}
