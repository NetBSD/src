/* $NetBSD: defs.h,v 1.2 2025/03/30 14:36:48 riastradh Exp $ */

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

#ifndef _DEFS_H_
#define _DEFS_H_

#ifndef lint
__RCSID("$NetBSD: defs.h,v 1.2 2025/03/30 14:36:48 riastradh Exp $");
#endif /* not lint */

#define PUBLIC
#define VERSION 18

/* debug bit flags */

#define DEBUG_STRUCT_BIT	__BIT(0)
#define DEBUG_DATA_BIT		__BIT(1)
#define DEBUG_EFI_IOC_BIT	__BIT(2)
#define DEBUG_MASK		__BITS(15,0)
#define DEBUG_BRIEF_BIT		__BIT(16)
#define DEBUG_VERBOSE_BIT	__BIT(17)

#define EFI_GLOBAL_VARIABLE \
	((uuid_t){0x8be4df61,0x93ca,0x11d2,0xaa,0x0d,\
	  {0x00,0xe0,0x98,0x03,0x2b,0x8c}})
#if 0
/* XXX: same as EFI_GLOBAL_VARIABLE */
#define EFI_VARIABLE_GUID \
	((uuid_t){0x8be4df61,0x93ca,0x11d2,0xaa,0x0d,\
	  {0x00,0xe0,0x98,0x03,0x2b,0x8c}})
#endif

#define	EFI_TABLE_SMBIOS \
	((uuid_t){0xeb9d2d31,0x2d88,0x11d3,0x9a,0x16,\
	  {0x00,0x90,0x27,0x3f,0xc1,0x4d}}

#define	EFI_TABLE_SMBIOS3 \
	((uuid_t){0xf2fd1544,0x9794,0x4a2c,0x99,0x2e,\
	  {0xe5,0xbb,0xcf,0x20,0xe3,0x94}}

#define LINUX_EFI_MEMRESERVE_TABLE \
	((uuid_t){0x888eb0c6,0x8ede,0x4ff5,0xa8,0xf0,\
	  {0x9a,0xee,0x5c,0xb9,0x77,0xc2}}

#define EFI_UNKNOWN_GUID \
	((uuid_t){0x47c7b225,0xc42a,0x11d2,0x57,0x8e,\
	  {0x00,0xa0,0xc9,0x69,0x72,0x3b}}

#define EFI_ADDRESS_RANGE_MIRROR_VARIABLE_GUID \
	((uuid_t){0x7b9be2e0,0xe28a,0x4197,0x3e,0xad,\
	  {0x32,0xf0,0x62,0xf9,0x46,0x2c}}

/*
 * XXX: It would be nice to have QueryVariableInfo() support to get
 * maximum variable size ... or at least export EFI_VARNAME_MAXBYTES
 * from efi.h!
 */
/* From sys/arch/x86/include/efi.h */
#define	EFI_PAGE_SHIFT		12
#define	EFI_PAGE_SIZE		(1 << EFI_PAGE_SHIFT)
#define	EFI_VARNAME_MAXBYTES	EFI_PAGE_SIZE

/* XXX: Why is this not exposed in sys/uuid.h? */
#define	UUID_STR_LEN	_UUID_STR_LEN

#endif /* _DEFS_H_ */
