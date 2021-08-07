/* $NetBSD: pci_smccc.h,v 1.1 2021/08/07 21:23:37 jmcneill Exp $ */

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

#ifndef _PCI_SMCCC_H
#define _PCI_SMCCC_H

#define	PCI_SMCCC_SUCCESS(_r)	(((_r) & 0x80000000) == 0)

#define	PCI_SMCCC_SBDF(s, b, d, f) \
	(((unsigned)(s) << 16) | ((b) << 8) | ((d) << 3) | (f))

#define	PCI_SMCCC_ACCESS_32BIT	4

int	pci_smccc_version(void);
int	pci_smccc_features(uint32_t);
int	pci_smccc_read(uint32_t, uint32_t, uint32_t, uint32_t *);
int	pci_smccc_write(uint32_t, uint32_t, uint32_t, uint32_t);
int	pci_smccc_get_seg_info(uint16_t, uint8_t *, uint8_t *, uint16_t *);

#endif /* !_PCI_SMCCC_H */
