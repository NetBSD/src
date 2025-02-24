/* $NetBSD: efiio.h,v 1.1 2025/02/24 13:47:56 christos Exp $ */

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

#ifndef _EFIIO_H_
#define _EFIIO_H_

#ifndef lint
__RCSID("$NetBSD: efiio.h,v 1.1 2025/02/24 13:47:56 christos Exp $");
#endif /* not lint */

#include <sys/efiio.h>
#include <stdbool.h>

#include "utils.h"

/*
 * {Get,Set}Variable() attribute bits: See sys/efiio.h
 */
#define	EFI_VAR_ATTR_BITS \
	"\177\020"		\
	"b\x0""nvmem\0"		\
	"b\x1""BSAcc\0"		\
	"b\x2""RTAcc\0"		\
	"b\x3""HWErr\0"		\
	"b\x4""WrAcc\0"		\
	"b\x5""TmAuthWr\0"	\
	"b\x6""AppWr\0"		\
	"b\x7""AuthAcc\0"

typedef struct efi_var_ioc efi_var_ioc_t;

typedef struct efi_var {
	efi_var_ioc_t ev;
	char *name;		/* asciiz name string */
} efi_var_t;

static inline void
efi_var_init(efi_var_ioc_t *ev, const char *name,
    uuid_t *vendor, uint32_t attrib)
{
	size_t isz = strlen(name) + 1;

	memset(ev, 0, sizeof(*ev));
	ev->name = utf8_to_ucs2(name, isz, NULL, &ev->namesize);
	ev->attrib = attrib;
	memcpy(&ev->vendor, vendor, sizeof(ev->vendor));
}

int set_variable(int, struct efi_var_ioc *);
struct efi_var_ioc get_variable(int, const char *, struct uuid *, uint32_t);
struct efi_var_ioc *get_next_variable(int, struct efi_var_ioc *);
void *get_table(int, struct uuid *, size_t *);
size_t get_variable_info(int, bool (*)(struct efi_var_ioc *, void *),
    int (*)(struct efi_var_ioc *ev, void *), void *);

#endif /* _EFIIO_H_ */
