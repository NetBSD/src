/* $NetBSD: devpath1.c,v 1.5 2025/12/15 17:06:42 joe Exp $ */

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
__RCSID("$NetBSD: devpath1.c,v 1.5 2025/12/15 17:06:42 joe Exp $");
#endif /* not lint */

#include <sys/uuid.h>

#include <assert.h>
#include <stdio.h>
#include <util.h>

#include "defs.h"
#include "devpath.h"
#include "devpath1.h"
#include "utils.h"

#define easprintf	(size_t)easprintf

/************************************************************************
 * Type 1 - PCI Device Path
 ************************************************************************/

/*
 * XXX: I can't find this GUID documented anywhere online.  I snarfed
 * it from
 * https://github.com/rhboot/efivar::efivar-main/src/include/efivar/efivar-dp.h
 * Comment out the define if you don't want to use it.
 */
/* Used in subtype 4 */
#if 1
#define EFI_EDD10_PATH_GUID						      \
	((uuid_t){0xcf31fac5,0xc24e,0x11d2,0x85,0xf3,			      \
	    {0x00,0xa0,0xc9,0x3e,0xc9,0x3b}})
#endif

#define EFI_MEMORY_TYPE \
	_X(ReservedMemoryType)		\
	_X(LoaderCode)			\
	_X(LoaderData)			\
	_X(BootServicesCode)		\
	_X(BootServicesData)		\
	_X(RuntimeServicesCode)		\
	_X(RuntimeServicesData)		\
	_X(ConventionalMemory)		\
	_X(UnusableMemory)		\
	_X(ACPIReclaimMemory)		\
	_X(ACPIMemoryNVS)		\
	_X(MemoryMappedIO)		\
	_X(MemoryMappedIOPortSpace)	\
	_X(PalCode)			\
	_X(PersistentMemory)		\
	_X(UnacceptedMemoryType)	\
	_X(MaxMemoryTyp)

typedef enum {
#define _X(n)	Efi ## n,
	EFI_MEMORY_TYPE
#undef _X
} EfiMemoryType_t;

static const char *
efi_memory_type_name(EfiMemoryType_t t)
{
	static const char *memtype[] = {
#define _X(n)	#n,
		EFI_MEMORY_TYPE
#undef _X
	};

	if (t >= __arraycount(memtype))
		return "Unknown";

	return memtype[t];
}

/****************************************/

static inline void
devpath_hw_pci(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.2.2 */
	struct { /* Sub-Type 1 */
		devpath_t	hdr;	/* Length = 6 */
		uint8_t		FunctionNum;
		uint8_t		DeviceNum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 6);

	path->sz = easprintf(&path->cp, "Pci(0x%x,0x%x)",
	    p->DeviceNum, p->FunctionNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(FunctionNum: %u\n)
		    DEVPATH_FMT(DeviceNum: %u\n),
		    DEVPATH_DAT_HDR(dp),
		    p->FunctionNum,
		    p->DeviceNum);
	}
}

static inline void
devpath_hw_pccard(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.2.2 */
	struct { /* Sub-Type 2 */
		devpath_t	hdr;	/* Length = 5 */
		uint8_t		FunctionNum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 5);

	path->sz = easprintf(&path->cp, "PcCard(0x%x)", p->FunctionNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(FunctionNum: %u\n),
		    DEVPATH_DAT_HDR(dp),
		    p->FunctionNum);
	}
}

static inline void
devpath_hw_memmap(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.2.3 */
	struct { /* Sub-Type 3 */
		devpath_t	hdr;	/* Length = 24 */
		uint32_t	MemoryType;
		uint64_t	StartAddress;
		uint64_t	EndAddress;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);
	const char *typename;

	typename = efi_memory_type_name(p->MemoryType);

	path->sz = easprintf(&path->cp, "MemMap(%s,0x%016" PRIx64
	    ",0x%016" PRIx64 ")", typename,
	    p->StartAddress, p->EndAddress);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(MemoryType: 0x%08x(%s)\n)
		    DEVPATH_FMT(StartAddress:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(EndAddress:) " 0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    p->MemoryType, typename,
		    p->StartAddress,
		    p->EndAddress);

	}
}

#ifdef EFI_EDD10_PATH_GUID
static inline void
devpath_hw_edd10(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See unknown - snarfed from efivar-main.zip */
	struct { /* Sub-Type 4 */
		devpath_t	hdr;	/* Length = 24 */
		uuid_t		GUID;
		uint32_t	devnum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);

	assert(p->hdr.Length == 24);

	/*
	 * p->data will be printed by debug
	 */
	path->sz = easprintf(&path->cp, "EDD10(0x%02x)", p->devnum);

	if (dbg != NULL) {
		char uuid_str[UUID_STR_LEN];

		uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(GUID: %s\n)
		    DEVPATH_FMT(devnum: 0x%08x\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str,
		    p->devnum);
	}
}
#endif /* EFI_EDD10_PATH_GUID */

static inline void
devpath_hw_vendor(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.2.4 */
	struct { /* Sub-Type 4 */
		devpath_t	hdr;	/* Length = 20 + n */
		uuid_t		GUID;
		uint8_t		data[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);
	char uuid_str[UUID_STR_LEN];

#ifdef EFI_EDD10_PATH_GUID
	if (memcmp(&p->GUID, &EFI_EDD10_PATH_GUID, sizeof(uuid_t)) == 0) {
		devpath_hw_edd10(dp, path, dbg);
		return;
	}
#endif /* EFI_EDD10_PATH_GUID */

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);

	/*
	 * p->data will be printed by debug
	 */
	path->sz = easprintf(&path->cp, "VenHw(%s)", uuid_str);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(GUID: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str);
	}
}

static inline void
devpath_hw_controller(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.2.5 */
	struct { /* Sub-Type 5 */
		devpath_t	hdr;	/* Length = 8 */
		uint32_t	CtrlNum;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 8);

	path->sz = easprintf(&path->cp, "Ctrl(0x%x)", p->CtrlNum);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(CtrlNum: 0x%x\n),
		    DEVPATH_DAT_HDR(dp),
		    p->CtrlNum);
	}
}

static inline const char *
devpath_hw_bmc_iftype(uint type)
{
	static const char *tbl[] = {
		[0] = "Unknown",
		[1] = "KCS",	// Keyboard Controller Style
		[2] = "SMIC",	// Server Management Interface Chip
		[3] = "BT",	// Block Transfer
	};

	if (type >= __arraycount(tbl))
		type = 0;

	return tbl[type];
}

/* Baseboard Management Controller */
static inline void
devpath_hw_bmc(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.2.6 */
	struct { /* Sub-Type 6 */
		devpath_t	hdr;	/* Length = 13 */
		uint8_t		IfaceType;
		uint64_t	BaseAddress;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 13);
	const char *iftype;

	iftype = devpath_hw_bmc_iftype(p->IfaceType);

	path->sz = easprintf(&path->cp, "(%s,0x%016" PRIx64 ")",
	    iftype, p->BaseAddress);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(IfaceType: %u(%s)\n)
		    DEVPATH_FMT(BaseAddress:) " 0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    p->IfaceType, iftype,
		    p->BaseAddress);
	}
}

PUBLIC void
devpath_hw(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	assert(dp->Type == 1);

	switch (dp->SubType) {
	case 1:   devpath_hw_pci(dp, path, dbg);		return;
	case 2:   devpath_hw_pccard(dp, path, dbg);		return;
	case 3:   devpath_hw_memmap(dp, path, dbg);		return;
	case 4:   devpath_hw_vendor(dp, path, dbg);		return;
	case 5:   devpath_hw_controller(dp, path, dbg);		return;
	case 6:   devpath_hw_bmc(dp, path, dbg);		return;
	default:  devpath_unsupported(dp, path, dbg);		return;
	}
}
