/* $NetBSD: pci_smccc.c,v 1.1 2021/08/07 21:23:37 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_smccc.c,v 1.1 2021/08/07 21:23:37 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <arm/arm/smccc.h>
#include <arm/pci/pci_smccc.h>

/* Minimum SMCCC version required for PCI_VERSION call. */
#define	SMCCC_VERSION_1_1	0x10001

/* PCI Configuration Space Access ABI functions */
#define	PCI_VERSION		0x84000130
#define	PCI_FEATURES		0x84000131
#define	PCI_READ		0x84000132
#define	PCI_WRITE		0x84000133
#define	PCI_GET_SEG_INFO	0x84000134
#define	 GET_SEG_INFO_BUS_START		__BITS(7,0)
#define	 GET_SEG_INFO_BUS_END		__BITS(15,8)

static int
pci_smccc_call(uint32_t fid,
    register_t arg1, register_t arg2, register_t arg3, register_t arg4,
    register_t *res0, register_t *res1, register_t *res2, register_t *res3)
{
	static int smccc_ver;

	if (smccc_ver == 0) {
		smccc_ver = smccc_version();
	}
	if (smccc_ver < SMCCC_VERSION_1_1) {
		return SMCCC_NOT_SUPPORTED;
	}

	return smccc_call(fid, arg1, arg2, arg3, arg4,
			  res0, res1, res2, res3);
}

int
pci_smccc_version(void)
{
	return pci_smccc_call(PCI_VERSION, 0, 0, 0, 0,
			      NULL, NULL, NULL, NULL);
}

int
pci_smccc_features(uint32_t fid)
{
	return pci_smccc_call(PCI_FEATURES, fid, 0, 0, 0,
			      NULL, NULL, NULL, NULL);
}

int
pci_smccc_read(uint32_t sbdf, uint32_t offset, uint32_t access_size,
    uint32_t *data)
{
	register_t value;
	int status;

	status = pci_smccc_call(PCI_READ, sbdf, offset, access_size, 0,
				NULL, &value, NULL, NULL);
	if (status == SMCCC_SUCCESS) {
		*data = value;
	}

	return status;
}

int
pci_smccc_write(uint32_t sbdf, uint32_t offset, uint32_t access_size,
    uint32_t data)
{
	return pci_smccc_call(PCI_WRITE, sbdf, offset, access_size, data,
			      NULL, NULL, NULL, NULL);
}

int
pci_smccc_get_seg_info(uint16_t seg, uint8_t *bus_start, uint8_t *bus_end,
    uint16_t *next_seg)
{
	register_t res1, res2;
	int status;

	status = pci_smccc_call(PCI_GET_SEG_INFO, seg, 0, 0, 0,
				NULL, &res1, &res2, NULL);
	if (status == SMCCC_SUCCESS) {
		*bus_start = __SHIFTOUT(res1, GET_SEG_INFO_BUS_START);
		*bus_end = __SHIFTOUT(res1, GET_SEG_INFO_BUS_END);
		*next_seg = (uint16_t)res2;
	}

	return status;
}
