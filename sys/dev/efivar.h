/* $NetBSD: efivar.h,v 1.2 2023/05/22 16:27:58 riastradh Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_EFIVAR_H
#define _DEV_EFIVAR_H

#include <sys/uuid.h>
#include <sys/types.h>

#include <machine/efi.h>

struct efi_ops {
	efi_status	(*efi_gettime)(struct efi_tm *, struct efi_tmcap *);
	efi_status	(*efi_settime)(struct efi_tm *);
	efi_status	(*efi_getvar)(uint16_t *, struct uuid *, uint32_t *,
			    u_long *, void *);
	efi_status	(*efi_setvar)(uint16_t *, struct uuid *, uint32_t,
			    u_long, void *);
	efi_status	(*efi_nextvar)(u_long *, uint16_t *, struct uuid *);
	efi_status	(*efi_gettab)(const struct uuid *, uint64_t *);
};

void	efi_register_ops(const struct efi_ops *);

#endif /* !_DEV_EFIVAR_H */
