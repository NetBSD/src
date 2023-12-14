/* $NetBSD: smccc.c,v 1.3.6.1 2023/12/14 17:43:10 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: smccc.c,v 1.3.6.1 2023/12/14 17:43:10 martin Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <arm/arm/psci.h>
#include <arm/arm/smccc.h>

#if defined(__arm__)
#  if defined(__clang__)
#define	SMCCC_ARCH_ATTRIBUTE  __attribute__ ((target("armv7ve")))
#  else /* gcc */
#define	SMCCC_ARCH_ATTRIBUTE  __attribute__ ((target("arch=armv7ve")))
#  endif
#else
#define	SMCCC_ARCH_ATTRIBUTE
#endif

/* Minimum supported PSCI version for SMCCC discovery */
#define	PSCI_VERSION_1_0	0x10000

/* Retrieve the implemented version of the SMC Calling Convention. */
#define	SMCCC_VERSION		0x80000000

/* True if SMCCC is detected. */
static bool			smccc_present;

/* SMCCC conduit (SMC or HVC) */
static enum psci_conduit	smccc_conduit = PSCI_CONDUIT_NONE;

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
		if (smccc_present) {
			smccc_conduit = psci_conduit();

			aprint_debug("SMCCC: Version %#x (%s)\n",
			    smccc_version(),
			    smccc_conduit == PSCI_CONDUIT_SMC ? "SMC" : "HVC");
		}
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
	return smccc_call(SMCCC_VERSION, 0, 0, 0, 0,
			  NULL, NULL, NULL, NULL);
}

/*
 * smccc_call --
 *
 *	Generic call interface for SMC/HVC calls.
 */
SMCCC_ARCH_ATTRIBUTE int
smccc_call(uint32_t fid,
    register_t arg1, register_t arg2, register_t arg3, register_t arg4,
    register_t *res0, register_t *res1, register_t *res2, register_t *res3)
{
	register_t args[5] = { fid, arg1, arg2, arg3, arg4 };

	register register_t r0 asm ("r0");
	register register_t r1 asm ("r1");
	register register_t r2 asm ("r2");
	register register_t r3 asm ("r3");
	register register_t r4 asm ("r4");

	if (!smccc_present) {
		return SMCCC_NOT_SUPPORTED;
	}

	KASSERT(smccc_conduit != PSCI_CONDUIT_NONE);

	r0 = args[0];
	r1 = args[1];
	r2 = args[2];
	r3 = args[3];
	r4 = args[4];

	if (smccc_conduit == PSCI_CONDUIT_SMC) {
		asm volatile ("smc #0" :
			      "=r" (r0), "=r" (r1), "=r" (r2), "=r" (r3) :
			      "r" (r0), "r" (r1), "r" (r2), "r" (r3), "r" (r4) :
			      "memory");
	} else {
		asm volatile ("hvc #0" :
			      "=r" (r0), "=r" (r1), "=r" (r2), "=r" (r3) :
			      "r" (r0), "r" (r1), "r" (r2), "r" (r3), "r" (r4) :
			      "memory");
	}
	
	if (res0) {
		*res0 = r0;
	}
	if (res1) {
		*res1 = r1;
	}
	if (res2) {
		*res2 = r2;
	}
	if (res3) {
		*res3 = r3;
	}

	return r0;
}
