/* $NetBSD: devpath3.c,v 1.6 2025/03/02 01:07:11 riastradh Exp $ */

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
__RCSID("$NetBSD: devpath3.c,v 1.6 2025/03/02 01:07:11 riastradh Exp $");
#endif /* not lint */

#include <arpa/inet.h>
#include <sys/uuid.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "defs.h"
#include "devpath.h"
#include "devpath3.h"
#include "utils.h"

#define easprintf	(size_t)easprintf

#if 0
GUID= EFI_PC_ANSI_GUID
GUID= EFI_VT_100_GIUD
GUID= EFI_VT_100_PLUS_GUID
GUID= EFI_VT_UTF8_GUID
GUID= DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL
GUID= EFI_DEBUGPORT_PROTOCOL_GUID
GUID= d487ddb4-008b-11d9-afdc-001083ffca4d
#endif

/* Used in sub-type 10 */
#define EFI_PC_ANSI_GUID\
  {0xe0c14753,0xf9be,0x11d2,0x9a,0x0c,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}

#define EFI_VT_100_GUID\
  {0xdfa66065,0xb419,0x11d3,0x9a,0x2d,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}

#define EFI_VT_100_PLUS_GUID\
  {0x7baec70b,0x57e0,0x4c76,0x8e,0x87,{0x2f,0x9e,0x28,0x08,0x83,0x43}}

#define EFI_VT_UTF8_GUID\
  {0xad15a0d6,0x8bec,0x4acf,0xa0,0x73,{0xd0,0x1d,0xe7,0x7e,0x2d,0x88}}

#define EFI_DEBUGPORT_PROTOCOL_GUID \
  {0xeba4e8d2,0x3858,0x41ec,0xa2,0x81,{0x26,0x47,0xba,0x96,0x60,0xd0}}

#define DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL \
  {0x37499a9d,0x542f,0x4c89,0xa0,0x26,{0x35,0xda,0x14,0x20,0x94,0xe4}}

#define EFI_VENDOR_SAS\
  {0xd487ddb4,0x008b,0x11d9,0xaf,0xdc,{0x00,0x10,0x83,0xff,0xca,0x4d}}

/************************************************************************/

static inline char *
upcase(char *str)
{

	for (char *p = str; *p != '\0'; p++)
		*p = (char)toupper((int)(unsigned char)*p);
	return str;
}

static inline char *
proto_name(uint16_t proto)	/* See RFC3233/RFC1700 */
{
	struct protoent *ent;
	char *name;
	char *bp;

	ent = getprotobynumber(proto);
	if (ent == NULL) {
		easprintf(&bp, "0x%04x", proto);
		return bp;
	}
	name = estrdup(ent->p_name);
	return upcase(name);
}

static inline char *
ipv4_addr(uint32_t addr)
{
	char *bp;

	bp = ecalloc(1, INET_ADDRSTRLEN);
	if (inet_ntop(AF_INET, &addr, bp, INET_ADDRSTRLEN) == NULL)
		err(EXIT_FAILURE, "%s: inet_ntop(AF_INET)", __func__);
	return bp;
}

static inline char *
ipv4_type(uint8_t type)
{

	switch (type) {
	case 0:		return estrdup("DHCP");
	case 1:		return estrdup("Static");
	default:	return estrdup("Unknown");
	}
}

static inline char *
ipv6_addr(struct in6_addr *addr)
{
	char *bp;

	bp = ecalloc(1, INET6_ADDRSTRLEN);
	if (inet_ntop(AF_INET6, addr, bp, INET6_ADDRSTRLEN) == NULL)
		err(EXIT_FAILURE, "%s: inet_ntop(AF_INET6)", __func__);
	return bp;
}

static inline char *
ipv6_type(uint8_t type)
{

	switch (type) {
	case 0:		return estrdup("Static");
	case 1:		return estrdup("StatelessAutoConfigure");
	case 2:		return estrdup("StatefulAutoConfigure");
	default:	return estrdup("Unknown");
	}
}

/************************************************************************
 * Type 3 - Messaging Device Path
 ************************************************************************/
static void
devpath_msg_atapi(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.1 */
	struct {			/* Sub-Type 1 */
		devpath_t	hdr;	/* Length = 8 */
		uint8_t		IsSecondary;
		uint8_t		IsSlave;
		uint16_t	LUN;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 8);

	path->sz = easprintf(&path->cp, "ATAPI(%u,%u,%u)",
	    p->IsSecondary, p->IsSlave, p->LUN);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(IsSecondary: %u\n)
		    DEVPATH_FMT(IsPrimary: %u\n)
		    DEVPATH_FMT(LUN: %u\n),
		    DEVPATH_DAT_HDR(dp),
		    p->IsSecondary,
		    p->IsSlave,
		    p->LUN);
	}
}

static void
devpath_msg_scsi(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.2 */
	struct {			/* Sub-Type 2 */
		devpath_t	hdr;	/* Length = 8 */
		uint16_t	SCSITargetID;		/* PUN */
		uint16_t	SCSILogicalUnitNum;	/* LUN */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 8);

	path->sz = easprintf(&path->cp, "SCSI(%u,%u)",
	    p->SCSITargetID, p->SCSILogicalUnitNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(SCSITargetID: %u\n)
		    DEVPATH_FMT(SCSILogicalUnitNum: %u\n),
		    DEVPATH_DAT_HDR(dp),
		    p->SCSITargetID,
		    p->SCSILogicalUnitNum);
	}
}

static void
devpath_msg_fibre(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.3 */
	struct {			/* Sub-Type 3 */
		devpath_t	hdr;	/* Length = 24 */
		uint32_t	Reserved;
		uint64_t	WWName;
		uint64_t	LUN;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);

	path->sz = easprintf(&path->cp, "Fibre(0x%016" PRIx64 ",0x%016"
	    PRIx64 ")", p->WWName, p->LUN);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Reserved: 0x%08x\n)
		    DEVPATH_FMT(WWName:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(LUN:) " 0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    p->Reserved,
		    p->WWName,
		    p->LUN);
	}
}

static void
devpath_msg_11394(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.4 */
	struct {			/* Sub-Type 4 */
		devpath_t	hdr;	/* Length = 16 */
		uint32_t	Reserved;
		uint64_t	GUID;	/* XXX: not an EFI_GUID */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 16);

	path->sz = easprintf(&path->cp, "11394(0x%016" PRIx64 ")", p->GUID);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Reserved: 0x%08x\n)
		    DEVPATH_FMT(GUID:) " 0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    p->Reserved,
		    p->GUID);
	}
}

static void
devpath_msg_usb(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.5 */
	struct {			/* Sub-Type 5 */
		devpath_t	hdr;	/* Length = 6 */
		uint8_t		USBParentPortNum;
		uint8_t		USBInterfaceNum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 6);

	path->sz = easprintf(&path->cp, "USB(%u,%u)",
	    p->USBParentPortNum, p->USBInterfaceNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(USBParentPortNum: %u\n)
		    DEVPATH_FMT(USBInterfaceNum: %u\n),
		    DEVPATH_DAT_HDR(dp),
		    p->USBParentPortNum,
		    p->USBInterfaceNum);
	}
}

static void
devpath_msg_i2o(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.9 */
	struct {			/* Sub-Type 6 */
		devpath_t	hdr;	/* Length = 8 */
		uint32_t	TID;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 8);

	path->sz = easprintf(&path->cp, "I2O(0x%08x)", p->TID);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(TID: 0x%08x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->TID);
	}
}

#define INFINIBAND_FLAG_BITS \
	"\177\020"		\
	"b\x0""Service\0"	\
	"b\x1""BootEnv\0"	\
	"b\x2""ConProto\0"	\
	"b\x3""Storage\0"	\
	"b\x4""NetWork\0"	\
	"f\x5\11""Resvd\0"

static void
devpath_msg_infiniband(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.14 */
	struct {			/* Sub-Type 9 */
		devpath_t	hdr;	/* Length = 48 */
		uint32_t	Flags;
		uuid_t		GUID;
		uint64_t	ServiceID;
		uint64_t	PortID;
		uint64_t	DeviceID;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 48);
	char uuid_str[UUID_STR_LEN];

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);

	path->sz = easprintf(&path->cp, "Infiniband(%#x,%s,0x%016" PRIx64
	    ",0x%016" PRIx64 ",0x%016" PRIx64 ")",
	    p->Flags, uuid_str, p->ServiceID, p->PortID, p->DeviceID);

	if (dbg != NULL) {
		char flags_str[128];

		snprintb(flags_str, sizeof(flags_str), INFINIBAND_FLAG_BITS,
		    p->Flags);

		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Flags: %s\n)
		    DEVPATH_FMT(GUID: %s\n)
		    DEVPATH_FMT(ServiceID:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(PortID:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(DeviceID:) " 0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    flags_str, uuid_str,
		    p->ServiceID, p->PortID, p->DeviceID);
	}
}

/****************************************
 * Sub-Type 10 variants
 */
static void
devpath_msg_debugport(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 18.3.7 */
	struct {			/* Sub-Type 10 */
		devpath_t	hdr;	/* Length = 20 */
		uuid_t		GUID;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);

	path->sz = easprintf(&path->cp, "DebugPort()");

	if (dbg != NULL) {
		char uuid_str[UUID_STR_LEN];

		uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);

		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(GUID: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str);
	}
}

static void
devpath_msg_uartflowctl(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.17 */
	struct {			/* Sub-Type 10 */
		devpath_t	hdr;	/* Length = 24 */
		uuid_t		GUID;
		uint32_t	FlowCtl;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);
	const char *fc_str;

	switch (p->FlowCtl) {
	case 0:		fc_str = "None";	break;
	case 1:		fc_str = "Hardware";	break;
	case 2:		fc_str = "XonXoff";	break;
	default:	fc_str = "????";	break;
	}

	path->sz = easprintf(&path->cp, "UartFlowCtrl(%s)", fc_str);

	if (dbg != NULL) {
		char uuid_str[UUID_STR_LEN];

		uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);

		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(GUID: %s\n)
		    DEVPATH_FMT(FlowCtl: 0x%04x(%s)\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str,
		    p->FlowCtl, fc_str);
	}
}

/****************************************
 * Macros for dealing with SAS/SATA device and topology info field.
 */

#define SASINFO_BYTES_MASK	__BITS(3,0)
#define SASINFO_SATA_BIT	__BIT(4)
#define SASINFO_EXTERNAL_BIT	__BIT(5)
#define SASINFO_EXPANDER_BIT	__BIT(6)

#define SASINFO_BYTES(b)	__SHIFTOUT((b), SASINFO_BYTES_MASK)
#define SASINFO_IS_SATA(b)	((b) % SASINFO_SATA_BIT)
#define SASINFO_IS_EXTERNAL(b)	((b) % SASINFO_EXTERNAL_BIT)
#define SASINFO_IS_EXPANDER(b)	((b) % SASINFO_EXPANDER_BIT)

#define SASINFO_SASSATA(b)  (SASINFO_IS_SATA(b)     ? "SATA"     : "SAS")
#define SASINFO_EXTERNAL(b) (SASINFO_IS_EXTERNAL(b) ? "External" : "Internal")
#define SASINFO_EXPANDER(b) (SASINFO_IS_EXPANDER(b) ? "Expander" : "direct")

/****************************************/

static void
devpath_msg_sas(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.18 */
	struct {			/* Sub-Type 10 */
		devpath_t	hdr;	/* Length = 44 */
		uuid_t		GUID;
		uint32_t	resv;
		uint64_t	addr;	/* display as big-endian */
		uint64_t	LUN;	/* display as big-endian */
		union {
			uint8_t b[2];
			uint16_t val;
		} __packed info;	/* device/topology info */
		uint16_t	RTP;	/* Relative Target Port */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 44);

	/*
	 * There seems to be a discrepency between the display
	 * examples in 10.3.4.18, 10.3.4.18.4, and Table 10.67.  The
	 * first example in 10.3.4.18 also refers to a third number
	 * (RTP) which isn't present.  I also find the info in Table
	 * 10.67 a bit unclear.
	 */
	if (SASINFO_BYTES(p->info.b[0]) == 0) {
		path->sz = easprintf(&path->cp, "SAS(0x%064" PRIx64
		    ",0x%064" PRIx64 ")",
		    htobe64(p->addr), htobe64(p->LUN));
	}
	else if (SASINFO_BYTES(p->info.b[0]) == 1) {
		if (SASINFO_IS_SATA(p->info.b[0])) {
			path->sz = easprintf(&path->cp, "SAS(0x%064" PRIx64
			    ",0x%064" PRIx64 ",SATA)",
			    htobe64(p->addr), htobe64(p->LUN));
		}
		else {
			path->sz = easprintf(&path->cp, "SAS(0x%064" PRIx64
			    ",SAS)", htobe64(p->addr));
		}
	}
	else {
		assert(SASINFO_BYTES(p->info.b[0]) == 2);
		uint drivebay = p->info.b[1] + 1;
		path->sz = easprintf(&path->cp, "SAS(0x%064" PRIx64
		    ",0x%064" PRIx64 ",0x%04x,%s,%s,%s,%d,0x%04x)",
		    htobe64(p->addr), htobe64(p->LUN), p->RTP,
		    SASINFO_SASSATA(p->info.b[0]),
		    SASINFO_EXTERNAL(p->info.b[0]),
		    SASINFO_EXPANDER(p->info.b[0]),
		    drivebay,
		    p->resv);
	}

	if (dbg != NULL) {
		char uuid_str[UUID_STR_LEN];

		uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);

		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(GUID: %s\n)
		    DEVPATH_FMT(resv: 0x%08x\n)
		    DEVPATH_FMT(addr:) " 0x%064" PRIx64 "(0x%064" PRIx64 ")\n"
		    DEVPATH_FMT(LUN:) "  0x%064" PRIx64 "(0x%064" PRIx64 ")\n"
		    DEVPATH_FMT(info: 0x%02x 0x%02x\n)
		    DEVPATH_FMT(RPT:  0x%04x\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str,
		    p->resv,
		    p->addr, htobe64(p->addr),
		    p->LUN,  htobe64(p->LUN),
		    p->info.b[0], p->info.b[1],
		    p->RTP);
	}
}

static struct vendor_type {
	uuid_t GUID;
	void(*fn)(devpath_t *, devpath_elm_t *, devpath_elm_t *);
	const char *name;
} *
devpath_msg_vendor_type(uuid_t *uuid)
{
	static struct vendor_type tbl[] = {
		{ .GUID = EFI_PC_ANSI_GUID,
		  .fn   = NULL,
		  .name = "VenPcAnsi",
		},
		{ .GUID = EFI_VT_100_GUID,
		  .fn   = NULL,
		  .name = "VenVt100",
		},
		{ .GUID = EFI_VT_100_PLUS_GUID,
		  .fn   = NULL,
		  .name = "VenVt100Plus",
		},
		{ .GUID = EFI_VT_UTF8_GUID,
		  .fn   = NULL,
		  .name = "VenUtf8",
		},
		/********/
		/* See 18.3.7 */
		{ .GUID = EFI_DEBUGPORT_PROTOCOL_GUID,
		  .fn   = devpath_msg_debugport,
		  .name = "DebugPort",
		},
		/********/
		/* See 10.3.4.17 */
		{ .GUID = DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL,
		  .fn   = devpath_msg_uartflowctl,
		  .name = "UartFlowCtrl",
		},
		/* See 10.3.4.18 */
		{ .GUID = EFI_VENDOR_SAS,
		  .fn   = devpath_msg_sas,
		  .name = "SAS",
		},
	};

	for (size_t i = 0; i < __arraycount(tbl); i++) {
		if (memcmp(uuid, &tbl[i].GUID, sizeof(*uuid)) == 0) {
			return &tbl[i];
		}
	}
	return NULL;
}

static void
devpath_msg_vendor(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.16 */
	struct {			/* Sub-Type 10 */
		devpath_t	hdr;	/* Length = 20 + n */
		uuid_t		GUID;
		uint8_t		data[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);
	char uuid_str[UUID_STR_LEN];
	size_t len;
	struct vendor_type *vt;
	char *data_str;

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);

	len = dp->Length - sizeof(*p);
	data_str = encode_data(p->data, len);

	vt = devpath_msg_vendor_type(&p->GUID);
	if (vt == NULL) {
		path->sz = easprintf(&path->cp, "VenMsg(%s,%s)",
		    uuid_str, data_str);
	}
	else if (vt->fn == NULL) {
		assert(vt->name != NULL);
		path->sz = easprintf(&path->cp, "%s(%s)", vt->name, data_str);
	}
	else {
		vt->fn(dp, path, dbg);
		return;
	}
	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(GUID: %s\n)
		    DEVPATH_FMT(Data: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str,
		    data_str);
	}
	free(data_str);
}

static void
devpath_msg_mac(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.10 */
	struct {			/* Sub-Type 11 */
		devpath_t	hdr;	/* Length = 37 */
		uint8_t		mac_addr[32];
		uint8_t		IfType;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 37);
	size_t addr_len;
	char *mac_addr;

	switch (p->IfType) {
	case 0:
	case 1:
		addr_len = 6;
		break;
	default:
		addr_len = 32;
		break;
	}
#if 0
 From: RFC3232 says everything is now online, found here:
 https://www.iana.org/assignments/arp-parameters/arp-parameters.xhtml

XXX: Should we make a table so we can show a name?

    0  Reserved [RFC5494]
    1  Ethernet (10Mb) [Jon_Postel]
    2  Experimental Ethernet (3Mb) [Jon_Postel]
    3  Amateur Radio AX.25 [Philip_Koch]
    4  Proteon ProNET Token Ring [Avri_Doria]
    5  Chaos [Gill_Pratt]
    6  IEEE 802 Networks [Jon_Postel]
    7  ARCNET [RFC1201]
    8  Hyperchannel [Jon_Postel]
    9  Lanstar [Tom_Unger]
    10 Autonet Short Address [Mike_Burrows]
    11 LocalTalk [Joyce_K_Reynolds]
    12 LocalNet (IBM PCNet or SYTEK LocalNET) [Joseph Murdock]
    13 Ultra link [Rajiv_Dhingra]
    14 SMDS [George_Clapp]
    15 Frame Relay [Andy_Malis]
    16 Asynchronous Transmission Mode (ATM) [[JXB2]]
    17 HDLC [Jon_Postel]
    18 Fibre Channel [RFC4338]
    19 Asynchronous Transmission Mode (ATM) [RFC2225]
    20 Serial Line [Jon_Postel]
    21 Asynchronous Transmission Mode (ATM) [Mike_Burrows]
    22 MIL-STD-188-220 [Herb_Jensen]
    23 Metricom [Jonathan_Stone]
    24 IEEE 1394.1995 [Myron_Hattig]
    25 MAPOS [Mitsuru_Maruyama][RFC2176]
    26 Twinaxial [Marion_Pitts]
    27 EUI-64 [Kenji_Fujisawa]
    28 HIPARP [Jean_Michel_Pittet]
    29 IP and ARP over ISO 7816-3 [Scott_Guthery]
    30 ARPSec [Jerome_Etienne]
    31 IPsec tunnel [RFC3456]
    32 InfiniBand (TM) [RFC4391]
    33 TIA-102 Project 25 Common Air Interface (CAI) [Jeff Anderson, Telecommunications Industry of America (TIA) TR-8.5 Formulating Group, <cja015&motorola.com>, June 2004]
    34 Wiegand Interface [Scott_Guthery_2]
    35 Pure IP [Inaky_Perez-Gonzalez]
    36 HW_EXP1 [RFC5494]
    37 HFI [Tseng-Hui_Lin]
    38 Unified Bus (UB) [Wei_Pan]
    39-255 Unassigned
    256 HW_EXP2 [RFC5494]
    257 AEthernet [Geoffroy_Gramaize]
    258-65534 Unassigned
    65535 Reserved [RFC5494]
#endif

	mac_addr = encode_data(p->mac_addr, addr_len);
	path->sz = easprintf(&path->cp, "MAC(%s,%d)", mac_addr, p->IfType);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(MAC_addr: %s\n)
		    DEVPATH_FMT(IfType: %x\n),
		    DEVPATH_DAT_HDR(dp),
		    mac_addr,
		    p->IfType);
	}
	free(mac_addr);
}

static void
devpath_msg_ipv4(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.11 */
	struct {			/* Sub-Type 12 */
		devpath_t	hdr;	/* Length = 27 */
		uint32_t	LocalIPAddr;
		uint32_t	RemoteIPAddr;
		uint16_t	LocalPort;
		uint16_t	RemotePort;
		uint16_t	Protocol;
		uint8_t		StaticIPAddr;
		uint32_t	GatewayIPAddr;
		uint32_t	SubnetMask;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 27);
	char *gaddr, *laddr, *raddr, *mask;
	char *proto, *atype;

	laddr = ipv4_addr(p->LocalIPAddr);
	gaddr = ipv4_addr(p->GatewayIPAddr);
	raddr = ipv4_addr(p->RemoteIPAddr);
	mask  = ipv4_addr(p->SubnetMask);
	proto = proto_name(p->Protocol);
	atype = ipv4_type(p->StaticIPAddr);

	path->sz = easprintf(&path->cp, "IPv4(%s,%s,%s,%s,%s,%s)",
	    raddr, proto, atype, laddr, gaddr, mask);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(LocalIPAddr: 0x%08x(%s)\n)
		    DEVPATH_FMT(RemoteIPAddr: 0x%08x(%s)\n)
		    DEVPATH_FMT(LocalPort: 0x%04x\n)
		    DEVPATH_FMT(RemotePort: 0x%04x\n)
		    DEVPATH_FMT(Protocol: 0x%04x(%s)\n)
		    DEVPATH_FMT(StaticIPAddr: 0x%02x(%s)\n)
		    DEVPATH_FMT(GatewayIPAddr: 0x%08x(%s)\n)
		    DEVPATH_FMT(SubnetMask: 0x%08x(%s)\n),
		    DEVPATH_DAT_HDR(dp),
		    p->LocalIPAddr, laddr,
		    p->RemoteIPAddr, raddr,
		    p->LocalPort,
		    p->RemotePort,
		    p->Protocol, proto,
		    p->StaticIPAddr, atype,
		    p->GatewayIPAddr, gaddr,
		    p->SubnetMask, mask);
	}
	free(atype);
	free(proto);
	free(mask);
	free(raddr);
	free(gaddr);
	free(laddr);
}

static void
devpath_msg_ipv6(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.12 */
	struct {			/* Sub-Type 13 */
		devpath_t	hdr;	/* Length = 60 */
		struct in6_addr	LocalIPAddr;
		struct in6_addr	RemoteIPAddr;
		uint16_t	LocalPort;
		uint16_t	RemotePort;
		uint16_t	Protocol;
		uint8_t		IPAddrOrigin;
		uint8_t		PrefixLength;
		struct in6_addr	GatewayIPAddr;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 60);
	char *gaddr, *laddr, *raddr;
	char *proto, *atype;

	laddr = ipv6_addr(&p->LocalIPAddr);
	gaddr = ipv6_addr(&p->GatewayIPAddr);
	raddr = ipv6_addr(&p->RemoteIPAddr);
	proto = proto_name(p->Protocol);
	atype = ipv6_type(p->IPAddrOrigin);

	path->sz = easprintf(&path->cp, "IPv6(%s,%s,%s,%s,%s)",
	    raddr, proto, atype, laddr, gaddr);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(LocalIPAddr: %s\n)
		    DEVPATH_FMT(RemoteIPAddr: %s\n)
		    DEVPATH_FMT(LocalPort: 0x%04x\n)
		    DEVPATH_FMT(RemotePort: 0x%04x\n)
		    DEVPATH_FMT(Protocol: 0x%04x(%s)\n)
		    DEVPATH_FMT(IPAddrOrigin: 0x%02x(%s)\n)
		    DEVPATH_FMT(PrefixLength: 0x%02x\n)
		    DEVPATH_FMT(GatewayIPAddr: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    laddr,
		    raddr,
		    p->LocalPort,
		    p->RemotePort,
		    p->Protocol, proto,
		    p->IPAddrOrigin, atype,
		    p->PrefixLength,
		    gaddr);
	}
	free(atype);
	free(proto);
	free(raddr);
	free(gaddr);
	free(laddr);
}

static inline const char *
uart_parity(uint8_t n)
{

	switch (n) {
	case 0:		return "D";
	case 1:		return "N";
	case 2:		return "E";
	case 3:		return "O";
	case 4:		return "M";
	case 5:		return "S";
	default:	return "?";
	}
}

static inline const char *
uart_stopbits(uint8_t n)
{

	switch (n) {
	case 0:		return "D";
	case 1:		return "1";
	case 2:		return "1.5";
	case 3:		return "2";
	default:	return "?";
	}
}

static void
devpath_msg_uart(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.15 */
	struct {			/* Sub-Type 14 */
		devpath_t	hdr;	/* Length = 19 */
		uint32_t	Reserved;
		uint64_t	BaudRate;
		uint8_t		DataBits;
		uint8_t		Parity;
		uint8_t		StopBits;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 19);
	const char *parity, *stopbits;

	parity = uart_parity(p->Parity);
	stopbits = uart_stopbits(p->StopBits);

	path->sz = easprintf(&path->cp, "Uart(%" PRIx64 ",%u,%s,%s)",
	    p->BaudRate, p->DataBits, parity, stopbits);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Reserved: 0x%08x\n)
		    DEVPATH_FMT(BaudRate:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(DataBits: 0x%1x\n)
		    DEVPATH_FMT(Parity:   0x%1x(%s)\n)
		    DEVPATH_FMT(StopBits: 0x%1x(%s)\n),
		    DEVPATH_DAT_HDR(dp),
		    p->Reserved,
		    p->BaudRate,
		    p->DataBits,
		    p->Parity, parity,
		    p->StopBits, stopbits);
	}
}

static inline const char *
usbclass_name(uint8_t class, uint8_t subclass)
{

	switch (class) {
	case 1:		return "UsbAudio";
	case 2:		return "UsbCDCControl";
	case 3:		return "UsbHID";
	case 6:		return "UsbImage";
	case 7:		return "UsbPrintr";
	case 8:		return "UsbMassStorage";
	case 9:		return "UsbHub";
	case 10:	return "UsbCDCData";
	case 11:	return "UsbSmartCard";
	case 14:	return "UsbVideo";
	case 220:	return "UsbDiagnostic";
	case 224:	return "UsbWireless";
	case 254:
		switch (subclass) {
		case 1:		return "UsbDeviceFirmwareUpdate";
		case 2:		return "UsbIrdaBridge";
		case 3:		return "UsbTestAndMeasurement";
		default:	return NULL;
		}
	default:	return NULL;
	}
}

static void
devpath_msg_usbclass(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.8 */
	struct {			/* Sub-Type 15 */
		devpath_t	hdr;	/* Length = 11 */
		uint16_t	VendorID;
		uint16_t	ProductID;
		uint8_t		DeviceClass;
		uint8_t		DeviceSubClass;
		uint8_t		DeviceProtocol;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 11);
	const char *name;

	name = usbclass_name(p->DeviceClass, p->DeviceSubClass);
	if (name == NULL) {
		path->sz = easprintf(&path->cp,
		    "UsbClass(0x%04x,0x%04x,0x%02x,0x%02x,0x%02x,)",
		    p->VendorID, p->ProductID, p->DeviceClass,
		    p->DeviceSubClass, p->DeviceProtocol);
	}
	else if (p->DeviceClass != 254) {
		path->sz = easprintf(&path->cp,
		    "%s(0x%04x,0x%04x,0x%02x,0x%02x,)",
		    name, p->VendorID, p->ProductID, p->DeviceSubClass,
		    p->DeviceProtocol);
	}
	else {
		path->sz = easprintf(&path->cp, "%s(0x%04x,0x%04x,0x%02x,)",
		    name, p->VendorID, p->ProductID, p->DeviceProtocol);
	}

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(VendorID:  0x%04x\n)
		    DEVPATH_FMT(ProductID: 0x%04x\n)
		    DEVPATH_FMT(DeviceClass: 0x%02x(%u)\n)
		    DEVPATH_FMT(DeviceSubClass: 0x%02x(%u)\n)
		    DEVPATH_FMT(DeviceProtocol: 0x%02x(%u)\n),
		    DEVPATH_DAT_HDR(dp),
		    p->VendorID,
		    p->ProductID,
		    p->DeviceClass, p->DeviceClass,
		    p->DeviceSubClass, p->DeviceSubClass,
		    p->DeviceProtocol, p->DeviceProtocol);
	}
}

static void
devpath_msg_usbwwid(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.7 */
	struct {			/* Sub-Type 16 */
		devpath_t	hdr;	/* Length = 10 + n */
		uint16_t	IFaceNum;
		uint16_t	VendorID;
		uint16_t	ProductID;
		uint8_t		SerialNum[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 10);
	char *serialnum;
	size_t seriallen;

	seriallen = p->hdr.Length - offsetof(typeof(*p), SerialNum);
	if (seriallen == 0) {
		serialnum = __UNCONST("WWID");	/* XXX: This is an error! */
		assert(0);
	}
	else {
		size_t sz = p->hdr.Length - sizeof(*p);
		serialnum = ucs2_to_utf8((uint16_t *)p->SerialNum, sz, NULL,
		    NULL);
	}

	path->sz = easprintf(&path->cp, "UsbWwid(0x%04x,0x%04x,0x%02x,%s)",
	    p->VendorID, p->ProductID, p->IFaceNum, serialnum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(IFaceNum: 0x%04x\n)
		    DEVPATH_FMT(VendorID: 0x%04x\n)
		    DEVPATH_FMT(ProductID: 0x%04x\n)
		    DEVPATH_FMT(SerialNum: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    p->IFaceNum, p->VendorID, p->ProductID, serialnum);
	}

	if (seriallen)
		free(serialnum);
}

static void
devpath_msg_unit(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.8 */
	struct {			/* Sub-Type 17 */
		devpath_t	hdr;	/* Length = 5 */
		uint8_t		LUN;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 5);

	path->sz = easprintf(&path->cp, "Unit(%u)", p->LUN);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(LUN: %u\n),
		    DEVPATH_DAT_HDR(dp),
		    p->LUN);
	}
}

static void
devpath_msg_sata(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.6 */
	struct {			/* Sub-Type 18 */
		devpath_t	hdr;	/* Length = 10 */
		uint16_t	SATAHBAPortNum;
		uint16_t	SATAPortMultiplierPortNum;
		uint16_t	SATALogicalUnitNum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 10);

	path->sz = easprintf(&path->cp, "SATA(%u,%u,%u)", p->SATAHBAPortNum,
	    p->SATAPortMultiplierPortNum, p->SATALogicalUnitNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(SATAHBAPortNum: %u\n)
		    DEVPATH_FMT(SATAPortMultiplierPortNum: %u\n)
		    DEVPATH_FMT(SATALogicalUnitNum: %u\n),
		    DEVPATH_DAT_HDR(dp),
		    p->SATAHBAPortNum,
		    p->SATAPortMultiplierPortNum,
		    p->SATALogicalUnitNum);
	}
}

static inline const char *
hdrdgst_name(uint16_t LoginOptions)
{

	switch (__SHIFTOUT(LoginOptions, __BITS(1,0))) {
	case 0:		return "None";
	case 2:		return "CRC32C";
	default: 	return "???";
	}
}

static inline const char *
datdgst_name(uint16_t LoginOptions)
{

	switch (__SHIFTOUT(LoginOptions, __BITS(3,2))) {
	case 0:		return "None";
	case 2:		return "CRC32C";
	default: 	return "???";
	}
}

static inline const char *
auth_name(uint16_t LoginOptions)
{

	switch (__SHIFTOUT(LoginOptions, __BITS(12,10))) {
	case 0:		return "CHAP_BI";
	case 2:		return "None";
	case 4:		return "CHAP_UNI";
	default:	return "???";
	}
}

static inline const char *
protocol_name(uint16_t Protocol)
{

	return Protocol == 0 ? "TCP" : "RSVD";
}

static void
devpath_msg_iscsi(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.20 */
#define LOGIN_OPTION_BITS \
	"\177\020"		\
	"f\0\2""HdrDgst\0"	\
	"=\x0""None\0"		\
	"=\x2""CRC32C\0"	\
	"f\x2\x2""DatDgst\0"	\
	"=\x0""None\0"		\
	"=\x2""CRC32C\0"	\
	"f\xA\x3""Auth\0"	\
	"=\x0""CHAP_BI\0"	\
	"=\x2""None\0"		\
	"=\x4""CHAP_UNI\0"
	struct {			/* Sub-Type 19 */
		devpath_t	hdr;	/* Length = 18 + n */
		uint16_t	Protocol;
		uint16_t	LoginOptions;
		uint64_t	LUN;	/* display as big-endian */
		uint16_t	TargetPortalGrp;
		char 		TargetName[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 18);
	char *name;
	const char *auth, *datdgst, *hdrdgst, *proto;
	size_t len;

	/*
	 * Make sure the TargetName is NUL terminated.  The spec is
	 * unclear on this, though their example is asciiz.  I have
	 * no real examples to test, so be safe.
	 */
	len = p->hdr.Length - sizeof(*p);
	name = emalloc(len + 1);
	memcpy(name, p->TargetName, len);
	name[len] = '\0';

	auth = auth_name(p->LoginOptions);
	datdgst = datdgst_name(p->LoginOptions);
	hdrdgst = hdrdgst_name(p->LoginOptions);
	proto = protocol_name(p->Protocol);

	path->sz = easprintf(&path->cp, "iSCSI(%s,0x%04x,0x%064" PRIx64
	    ",%s,%s,%s,%s)", p->TargetName, p->TargetPortalGrp,
	    htobe64(p->LUN), hdrdgst, datdgst, auth, proto);

	if (dbg != NULL) {
		char liopt[256];

		snprintb(liopt, sizeof(liopt), LOGIN_OPTION_BITS,
		    p->LoginOptions);
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Protocol: 0x%04x(%s)\n)
		    DEVPATH_FMT(LoginOptions: %s\n)
		    DEVPATH_FMT(LUN:) " 0x%064" PRIx64 "\n"
		    DEVPATH_FMT(TargetPortalGrp: 0x%04x\n)
		    DEVPATH_FMT(TargetName: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    p->Protocol, proto,
		    liopt,
		    htobe64(p->LUN),
		    p->TargetPortalGrp,
		    name);
	}

	free(name);
}

static void
devpath_msg_vlan(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.13 */
	struct {			/* Sub-Type 20 */
		devpath_t	hdr;	/* Length = 6 */
		uint16_t	Vlanid;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 6);

	path->sz = easprintf(&path->cp, "Vlan(0x%04x)", p->Vlanid);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Vlanid: 0x%04x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->Vlanid);
	}
}

static void
devpath_msg_fibreex(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.3 */
	struct {			/* Sub-Type 21 */
		devpath_t	hdr;	/* Length = 24 */
		uint32_t	Reserved;
		uint64_t	WorldWideName;	/* bit-endian */
		uint64_t	LUN;		/* bit-endian */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);

	path->sz = easprintf(&path->cp, "FibreEx(0x%064" PRIx64 ",0x%064"
	    PRIx64 ")", htobe64(p->WorldWideName), htobe64(p->LUN));

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(WorldWideName:) " 0x%064" PRIx64 "\n"
		    DEVPATH_FMT(LUN:) " 0x%064" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    htobe64(p->WorldWideName),
		    htobe64(p->LUN));
	}
}

static void
devpath_msg_sasex(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.18 */
	struct {			/* Sub-Type 22 */
		devpath_t	hdr;	/* Length = 24 XXX: 32 in text */
		uint64_t	addr;	/* display as big-endian */
		uint64_t	LUN;	/* display as big-endian */
		union {
			uint8_t b[2];
			uint16_t val;
		} __packed info;	/* device/topology info */
		uint16_t	RTP;	/* Relative Target Port */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);

	/*
	 * XXX: This should match what is in devpath_msg_sas().
	 * Should we share code?
	 */
	if (SASINFO_BYTES(p->info.b[0]) == 0) {
		path->sz = easprintf(&path->cp, "SasEx(0x%064" PRIx64
		    ",0x%064" PRIx64 ")",
		    htobe64(p->addr), htobe64(p->LUN));
	}
	else if (SASINFO_BYTES(p->info.b[0]) == 1) {
		if (SASINFO_IS_SATA(p->info.b[0])) {
			path->sz = easprintf(&path->cp, "SasEx(0x%064" PRIx64
			    ",0x%064" PRIx64 ",SATA)",
			    htobe64(p->addr), htobe64(p->LUN));
		}
		else {
			path->sz = easprintf(&path->cp, "SasEx(0x%064" PRIx64
			    ",SAS)", htobe64(p->addr));
		}
	}
	else {
		assert(SASINFO_BYTES(p->info.b[0]) == 2);
		uint drivebay = p->info.b[1] + 1;
		path->sz = easprintf(&path->cp, "SasEx(0x%064" PRIx64
		    ",0x%064" PRIx64 ",0x%04x,%s,%s,%s,%d)",
		    htobe64(p->addr), htobe64(p->LUN), p->RTP,
		    SASINFO_SASSATA(p->info.b[0]),
		    SASINFO_EXTERNAL(p->info.b[0]),
		    SASINFO_EXPANDER(p->info.b[0]),
		    drivebay);
	}

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(addr:) " 0x%064" PRIx64 "(0x%064" PRIx64 ")\n"
		    DEVPATH_FMT(LUN:) "  0x%064" PRIx64 "(0x%064" PRIx64 ")\n"
		    DEVPATH_FMT(info: 0x%02x 0x%02x\n)
		    DEVPATH_FMT(RPT:  0x%04x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->addr, htobe64(p->addr),
		    p->LUN,  htobe64(p->LUN),
		    p->info.b[0], p->info.b[1],
		    p->RTP);
	}
}

static void
devpath_msg_nvme(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.21 */
	struct {			/* Sub-Type 23 */
		devpath_t	hdr;	/* Length = 16 */
		uint32_t	NSID;	/* Name Space Identifier */
		union {
			uint8_t		b[8];
			uint64_t	val;
		} EUI;	/* IEEE Extended Unique Identifier */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 16);

	path->sz = easprintf(&path->cp, "NVMe(0x%x,"
	    "%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X)",
	    p->NSID,
	    p->EUI.b[0], p->EUI.b[1], p->EUI.b[2], p->EUI.b[3],
	    p->EUI.b[4], p->EUI.b[5], p->EUI.b[6], p->EUI.b[7]);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(NSID: 0x%08x\n)
		    DEVPATH_FMT(EUI:) "  0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    p->NSID, p->EUI.val);
	}
}

static void
devpath_msg_uri(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.22 */
	struct {			/* Sub-Type 24 */
		devpath_t	hdr;	/* Length = 4 + n */
		uint8_t		data[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 4);
	size_t len;
	char *buf;

	len = dp->Length - 4;

	buf = emalloc(len + 1);
	if (len > 0)
		memcpy(buf, p->data, len);
	buf[len] = '\0';

	path->sz = easprintf(&path->cp, "Uri(%s)", buf);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Data: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    buf);
	}
	free(buf);
}

static void
devpath_msg_ufs(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.23 */
	struct {			/* Sub-Type 25 */
		devpath_t	hdr;	/* Length = 6 */
		uint8_t		PUN;	/* Target ID */
		uint8_t		LUN;	/* Logical Unit Number */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 6);

	path->sz = easprintf(&path->cp, "UFS(%#x,0x%02x)", p->PUN, p->LUN);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(PUN: 0x%02x\n)
		    DEVPATH_FMT(LUN: 0x%02x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->PUN, p->LUN);
	}
}

static void
devpath_msg_sd(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.24 */
	struct {			/* Sub-Type 26 */
		devpath_t	hdr;	/* Length = 5 */
		uint8_t		SlotNum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 5);

	path->sz = easprintf(&path->cp, "SD(%d)", p->SlotNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(SlotNum: 0x%02x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->SlotNum);
	}
}

static void
devpath_msg_bluetooth(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.25 */
	struct {			/* Sub-Type 27 */
		devpath_t	hdr;	/* Length = 10 */
		uint8_t		bdaddr[6];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 10);

	path->sz = easprintf(&path->cp,
	    "Bluetooth(%02x:%02x:%02x:%02x:%02x:%02x)",
	    p->bdaddr[0], p->bdaddr[1], p->bdaddr[2],
	    p->bdaddr[3], p->bdaddr[4], p->bdaddr[5]);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(bdaddr: 0x%02x%02x%02x%02x%02x%02x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->bdaddr[0], p->bdaddr[1], p->bdaddr[2],
		    p->bdaddr[3], p->bdaddr[4], p->bdaddr[5]);
	}
}

static void
devpath_msg_wifi(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.26 */
	struct {			/* Sub-Type 28 */
		devpath_t	hdr;	/* Length = 36 */
		char		SSID[32];/* XXX: ascii? */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 36);

	path->sz = easprintf(&path->cp, "Wi-Fi(%s)", p->SSID);

	if (dbg != NULL) {
		char *ssid;

		ssid = encode_data((uint8_t *)p->SSID, sizeof(p->SSID));
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(SSID: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    ssid);
		free(ssid);
	}
}

static void
devpath_msg_emmc(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.27 */
	struct {			/* Sub-Type 29 */
		devpath_t	hdr;	/* Length = 5 */
		uint8_t		SlotNum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 5);

	path->sz = easprintf(&path->cp, "eMMC(%d)", p->SlotNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(SlotNum: 0x%02x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->SlotNum);
	}
}

static void
devpath_msg_bluetoothle(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.28 */
	struct {			/* Sub-Type 30 */
		devpath_t	hdr;	/* Length = 11 */
		uint8_t		bdaddr[6];
		uint8_t		addr_type;
#define BDADDR_TYPE_PUBLIC	0
#define BDADDR_TYPE_RANDOM	1
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 11);

	path->sz = easprintf(&path->cp,
	    "BluetoothLE(%02x:%02x:%02x:%02x:%02x:%02x,%d)",
	    p->bdaddr[0], p->bdaddr[1], p->bdaddr[2],
	    p->bdaddr[3], p->bdaddr[4], p->bdaddr[5],
	    p->addr_type);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(bdaddr: 0x%02x%02x%02x%02x%02x%02x\n)
		    DEVPATH_FMT(addr_type: 0x%02x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->bdaddr[0], p->bdaddr[1], p->bdaddr[2],
		    p->bdaddr[3], p->bdaddr[4], p->bdaddr[5],
		    p->addr_type);
	}
}

static void
devpath_msg_dns(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.29 */
	struct {			/* Sub-Type 31 */
		devpath_t	hdr;	/* Length = 5 + n */
		uint8_t		IsIPv6;
		uint8_t		addr[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 5);
	size_t cnt, n, sz;

	/* XXX: This is ugly at best */

	n = p->hdr.Length - sizeof(*p);
	sz = p->IsIPv6 ? sizeof(struct in6_addr) : sizeof(uint32_t);
	cnt = n / sz;
	assert(n == cnt * sz);
	assert(cnt > 0);

	if (cnt > 0) { /* XXX: should always be true */
		char *bp;

#define DNS_PREFIX	"Dns("
		if (p->IsIPv6) {
			struct in6_addr *ip6addr = (void *)p->addr;

			bp = path->cp = malloc(sizeof(DNS_PREFIX) +
			    cnt * INET6_ADDRSTRLEN);
			bp = stpcpy(bp, DNS_PREFIX);
			bp = stpcpy(bp, ipv6_addr(&ip6addr[0]));
			for (size_t i = 1; i < cnt; i++) {
				*bp++ = ',';
				bp = stpcpy(bp, ipv6_addr(&ip6addr[i]));
			}
			*bp = ')';
		}
		else {	/* IPv4 */
			uint32_t *ip4addr = (void *)p->addr;

			bp = path->cp = malloc(sizeof(DNS_PREFIX) +
			    cnt * INET_ADDRSTRLEN);
			bp = stpcpy(bp, DNS_PREFIX);
			bp = stpcpy(bp, ipv4_addr(ip4addr[0]));
			for (size_t i = 1; i < cnt; i++) {
				*bp++ = ',';
				bp = stpcpy(bp, ipv4_addr(ip4addr[i]));
			}
			*bp = ')';
		}
#undef DNS_PREFIX
		path->sz = strlen(bp);
	}

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(IsIPv6: %d\n),
		    DEVPATH_DAT_HDR(dp),
		    p->IsIPv6);

		if (cnt > 0) { /* XXX: should always be true */
			char *bp, *tp;

			bp = dbg->cp;
			if (p->IsIPv6) {
				struct in6_addr *ip6addr = (void *)p->addr;

				for (size_t i = 0; i < cnt; i++) {
					uint8_t *cp;

					cp = ip6addr[i].__u6_addr.__u6_addr8;
					tp = bp;
					dbg->sz += easprintf(&bp,
					    "%s"
					    DEVPATH_FMT(addr[%zu]:)
					    " %02x %02x %02x %02x"
					    " %02x %02x %02x %02x"
					    " %02x %02x %02x %02x"
					    " %02x %02x %02x %02x\n",
					    tp,
					    i,
					    cp[0],  cp[1],  cp[2],  cp[3],
					    cp[4],  cp[5],  cp[6],  cp[7],
					    cp[8],  cp[9],  cp[10], cp[11],
					    cp[12], cp[13], cp[14], cp[15]);
					free(tp);
				}
			}
			else {	/* IPv4 */
				uint32_t *ip4addr = (void *)p->addr;

				for (size_t i = 0; i < cnt; i++) {
					tp = bp;
					dbg->sz += easprintf(&bp,
					    "%s"
					    DEVPATH_FMT(addr[i]: 0x%08x\n),
					    tp, ip4addr[i]);
					free(tp);
				}
			}
		}
	}
}

static void
devpath_msg_nvdimm(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.30 */
	struct {			/* Sub-Type 32 */
		devpath_t	hdr;	/* Length = 20 */
		uuid_t		UUID;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);
	char uuid_str[UUID_STR_LEN];

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->UUID);
	path->sz = easprintf(&path->cp, "NVDIMM(%s)", uuid_str);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR,
		    DEVPATH_DAT_HDR(dp)
		);
	}
}

static void
devpath_msg_restservice(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.31 */
	struct {			/* Sub-Type 33 */
		devpath_t	hdr;	/* Length = 6 */
		uint8_t		RestService;
#define REST_SERVICE_REDFISH	0x01
#define REST_SERVICE_ODATA	0x02
		uint8_t		AccessMode;
#define ACCESS_MODE_IN_BAND	0x01
#define ACCESS_MODE_OUT_OF_BAND	0x02
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 6);

	path->sz = easprintf(&path->cp, "RestService(%d,%d)",
	    p->RestService, p->AccessMode);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(RestService: 0x%02x(%s)\n)
		    DEVPATH_FMT(AccessMode:  0x%02x(%s)\n),
		    DEVPATH_DAT_HDR(dp),
		    p->RestService,
		    p->RestService == REST_SERVICE_REDFISH ? "RedFish" :
		    p->RestService == REST_SERVICE_ODATA ? "OData" : "???",
		    p->AccessMode,
		    p->AccessMode == ACCESS_MODE_IN_BAND ? "In-Band" :
		    p->AccessMode == ACCESS_MODE_OUT_OF_BAND ? "Out-of-Band"
			: "???");
	}
}

static void
devpath_msg_nvmeof(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.4.32 */
	struct {			/* Sub-Type 34 */
		devpath_t	hdr;	/* Length = 21 XXX: 20 in text */
		uint8_t		NIDT;
		union {
			uint64_t	dw[2];
			uint8_t		csi;
			uint64_t	ieuid;
			uuid_t		uuid;
		} NID;	/* big-endian */
		uint8_t		SubsystemNQN[];  /* UTF-8 null-terminated */
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 21);
	char uuid_str[UUID_STR_LEN];

	/*
	 * See 5.1.13.2.3 of NVM-Express Base Specification Rev 2.1,
	 * in particular, Fig 315 on page 332.
	 */
	switch (p->NIDT) {
	case 0:
		path->sz = easprintf(&path->cp, "NVMEoF(%s)", p->SubsystemNQN);
		break;
	case 1:
		path->sz = easprintf(&path->cp, "NVMEoF(%s,0x%016" PRIx64
		    ")", p->SubsystemNQN, p->NID.ieuid);
		break;
	case 2:
	case 3:
		uuid_snprintf(uuid_str, sizeof(uuid_str), &p->NID.uuid);
		path->sz = easprintf(&path->cp, "NVMEoF(%s,%s)",
		    p->SubsystemNQN, uuid_str);
		break;
	case 4:
		path->sz = easprintf(&path->cp, "NVMEoF(%s,0x%02x)",
		    p->SubsystemNQN, p->NID.csi);
		break;
	default:
		path->sz = easprintf(&path->cp, "NVMEoF(%s,unknown)",
		    p->SubsystemNQN);
		break;
	}

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(NIDT: 0x%02x\n)
		    DEVPATH_FMT(NID:) " 0x%016" PRIx64 "%016" PRIx64 "\n"
		    DEVPATH_FMT(SubsystemNQN: '%s'\n),
		    DEVPATH_DAT_HDR(dp),
		    p->NIDT,
		    p->NID.dw[0], p->NID.dw[1],
		    p->SubsystemNQN);
	}
}

PUBLIC void
devpath_msg(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	assert(dp->Type = 3);

	switch (dp->SubType) {
	case 1:     devpath_msg_atapi(dp, path, dbg);		return;
	case 2:     devpath_msg_scsi(dp, path, dbg);		return;
	case 3:     devpath_msg_fibre(dp, path, dbg);		return;
	case 4:     devpath_msg_11394(dp, path, dbg);		return;
	case 5:     devpath_msg_usb(dp, path, dbg);		return;
	case 6:     devpath_msg_i2o(dp, path, dbg);		return;
	case 9:     devpath_msg_infiniband(dp, path, dbg);	return;
	case 10:    devpath_msg_vendor(dp, path, dbg);		return;
	case 11:    devpath_msg_mac(dp, path, dbg);		return;
	case 12:    devpath_msg_ipv4(dp, path, dbg);		return;
	case 13:    devpath_msg_ipv6(dp, path, dbg);		return;
	case 14:    devpath_msg_uart(dp, path, dbg);		return;
	case 15:    devpath_msg_usbclass(dp, path, dbg);	return;
	case 16:    devpath_msg_usbwwid(dp, path, dbg);		return;
	case 17:    devpath_msg_unit(dp, path, dbg);		return;
	case 18:    devpath_msg_sata(dp, path, dbg);		return;
	case 19:    devpath_msg_iscsi(dp, path, dbg);		return;
	case 20:    devpath_msg_vlan(dp, path, dbg);		return;
	case 21:    devpath_msg_fibreex(dp, path, dbg);		return;
	case 22:    devpath_msg_sasex(dp, path, dbg);		return;
	case 23:    devpath_msg_nvme(dp, path, dbg);		return;
	case 24:    devpath_msg_uri(dp, path, dbg);		return;
	case 25:    devpath_msg_ufs(dp, path, dbg);		return;
	case 26:    devpath_msg_sd(dp, path, dbg);		return;
	case 27:    devpath_msg_bluetooth(dp, path, dbg);	return;
	case 28:    devpath_msg_wifi(dp, path, dbg);		return;
	case 29:    devpath_msg_emmc(dp, path, dbg);		return;
	case 30:    devpath_msg_bluetoothle(dp, path, dbg);	return;
	case 31:    devpath_msg_dns(dp, path, dbg);		return;
	case 32:    devpath_msg_nvdimm(dp, path, dbg);		return;
	case 33:    devpath_msg_restservice(dp, path, dbg);	return;
	case 34:    devpath_msg_nvmeof(dp, path, dbg);		return;
	default:    devpath_unsupported(dp, path, dbg);		return;
	}
}
