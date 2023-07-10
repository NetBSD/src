/* $NetBSD: efi_runtime.h,v 1.5 2023/07/10 07:00:12 rin Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_EFI_RUNTIME_H
#define _ARM_EFI_RUNTIME_H

#include <arm/efi.h>

int		arm_efirt_init(paddr_t);
efi_status	arm_efirt_gettime(struct efi_tm *, struct efi_tmcap *);
efi_status	arm_efirt_settime(struct efi_tm *);
efi_status	arm_efirt_getvar(uint16_t *, struct uuid *, uint32_t *,
				 u_long *, void *);
efi_status	arm_efirt_nextvar(u_long *, uint16_t *, struct uuid *);
efi_status	arm_efirt_setvar(uint16_t *, struct uuid *, uint32_t,
				 u_long, void *);
int		arm_efirt_reset(enum efi_reset);

int		arm_efirt_md_enter(void);
void		arm_efirt_md_exit(void);

#endif /* !_ARM_EFI_RUNTIME_H */
