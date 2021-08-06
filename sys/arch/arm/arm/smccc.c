/* $NetBSD: smccc.c,v 1.1 2021/08/06 19:38:53 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: smccc.c,v 1.1 2021/08/06 19:38:53 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <arm/arm/psci.h>
#include <arm/arm/smccc.h>

/* Minimum supported PSCI version for SMCCC discovery */
#define	PSCI_VERSION_1_0	0x10000

/* Retrieve the implemented version of the SMC Calling Convention. */
#define	SMCCC_VERSION		0x80000000

/* True if SMCCC is detected. */
static bool	smccc_present;

/*
 * smccc_probe --
 *
 *	Returns true if SMCCC is supported by platform firmware.
 */
bool
smccc_probe(void)
{
	if (cold && !smccc_present) {
		if (!psci_available() || psci_version() < PSCI_VERSION_1_0) {
			return false;
		}

		smccc_present = psci_features(SMCCC_VERSION) == PSCI_SUCCESS;
	}
	return smccc_present;
}

/*
 * smccc_version --
 *
 *	Return the implemented SMCCC version, or a negative error code on failure.
 */
int
smccc_version(void)
{
	return smccc_call(SMCCC_VERSION, 0, 0, 0);
}

/*
 * smccc_call --
 *
 *	Generic call interface for SMC/HVC calls.
 */
int
smccc_call(register_t fid, register_t arg1, register_t arg2, register_t arg3)
{
	if (!smccc_present) {
		return SMCCC_NOT_SUPPORTED;
	}

	return psci_call(fid, arg1, arg2, arg3);
}
