/*	$NetBSD: arm.c,v 1.2 2018/01/16 08:23:18 mrg Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: arm.c,v 1.2 2018/01/16 08:23:18 mrg Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/cpuio.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <err.h>

#include "../cpuctl.h"

static const char * const id_isar_fieldnames[][8] = {
	{
		"Swap", "Bitcount", "Bitfield", "CmpBranch",
		"Coproc", "Debug", "Divde", NULL
	}, {
		"Endian", "Except", "Except_AR", "Extend",
		"IfThen", "Immediate", "Interwork", "Jazelle"
	}, {
		"LoadStore", "MemHint", "MultAccessInt", "Mult",
		"MultS", "MultU", "PSR_AR", "Reversal"
	}, {
		"Saturate", "SIMD", "SVC", "SynchPrim",
		"TabBranch", "ThumbCopy", "TrueNOP", "ThumbEE_Extn"
	}, {
		"Unpriv", "WithShifts", "Writeback", "SMC",
		"Barrier", "SynchPrim_frac", "PSR_M", "SWP"
	}
};

static const uint8_t id_isar_boolean[] = {
	0x2f, 0xb7, 0x41, 0xf5, 0xfc
};

static const char * const id_mmfr_fieldnames[][8] = {
	{
		"VMSA-Support",
		"PMSA-Support",
		"Outermost-Shareablity",
		"Shareability-Levels",
		"TCM-Support",
		"Auxilary-Registers",
		"FCSE-Support",
		"Innermost-Shareability"
	}, {
		"L1-Harvard-Cache-VA", 
		"L1-Unified-Cache-VA", 
		"L1-Harvard-Cache-Set/Way", 
		"L1-Unified-Cache-Set/Way", 
		"L1-Harvard-Cache", 
		"L1-Unified-Cache", 
		"L1-Cache-Test-and-Clean", 
		"Branch-Predictor", 
	}, {
		"L1-Harvard-Foreground-Fetch", 
		"L1-Unified-Background-Fetch", 
		"L1-Harvard-Range", 
		"Harvard-TLB",
		"Unified-TLB", 
		"Mem-Barrier", 
		"WFI-Stall", 
		"HW-Access", 
	}, {
		"Cache-Maintenance-MVA", 
		"Cache-Maintenance-Set/Way", 
		"BP-Maintenance", 
		"Maintenance-Broadcast",
		NULL,
		"Coherent-Tablewalk", 
		"Cached-Memory-Size", 
		"Supersection-Support", 
	},
};

static const uint8_t id_mmfr_present[] = {
	0x8c, 0x00, 0x00, 0x68
};

static const char * const id_pfr_fieldnames[][8] = {
	{
		"ThumbEE",
		"Jazelle",
		"Thumb",
		"ARM",
	}, {
		"Programmer",
		"Security",
		"M-profile",
		"Virtualization",
		"Generic-Timer",
	},
};

static const char * const id_mvfr_fieldnames[][8] = {
	{
		"ASIMD-Registers",
		"Single-Precision",
		"Double-Precision",
		"VFP-Exception-Trapping",
		"Divide",
		"Square-Root",
		"Short-Vectors",
		"VFP-Rounding-Modes",
	}, {
		"Flush-To-Zero",
		"Default-NaN",
		"ASIMD-Load/Store",
		"ASIMD-Integer",
		"ASIMD-SPFP",
		"ASIMD-HPFP",
		"VFP-HPFP",
		"ASIMD-FMAC",
	},
};

static const uint8_t id_mvfr_present[] = {
	0x80, 0x03,
};

static void
print_features(const char *cpuname, const char *setname,
    const int *id_data, size_t id_len, const char * const id_fieldnames[][8],
    size_t id_nfieldnames, const uint8_t *id_boolean, const uint8_t *id_present)
{
	char buf[81];
	size_t len = 0;
	const char *sep = "";
	for (size_t i = 0; i < id_len / sizeof(id_data[0]); i++) {
		int isar = id_data[i];
		for (u_int j = 0; isar != 0 && j < 8; j++, isar >>= 4) {
			const char *name = NULL;
			const char *value = "";
			char namebuf[12], valuebuf[12], tmpbuf[30];
			if ((isar & 0x0f) == 0
			    && (id_present == NULL
				|| (id_present[i] & (1 << j))) == 0) {
				continue;
			}
			if (len == 0) {
				len = snprintf(buf, sizeof(buf),
				    "%s: %s: ", cpuname, setname);
			}
			if (i < id_nfieldnames) {
				name = id_fieldnames[i][j];
			}
			if (name == NULL) {
				name = namebuf;
				snprintf(namebuf, sizeof(namebuf),
				    "%zu[%u]", i, j);
			}
			if (id_boolean == NULL
			    || (id_boolean[i] & (1 << j)) == 0
			    || (isar & 0xe) != 0) {
				value = valuebuf;
				snprintf(valuebuf, sizeof(valuebuf),
				    "=%u", isar & 0x0f);
			}
			size_t tmplen = snprintf(tmpbuf, sizeof(tmpbuf),
			     "%s%s%s", sep, name, value);   
			if (len + tmplen > 78) {
				printf("%s\n", buf);
				len = snprintf(buf, sizeof(buf),
				    "%s: %s: %s", cpuname, setname, tmpbuf + 2);
			} else {
				len = strlcat(buf, tmpbuf, sizeof(buf));
			}
			sep = ", ";
		}
	}
	if (len > 0) {
		printf("%s\n", buf);
	}
}

bool
identifycpu_bind(void)
{

	return false;
}

void
identifycpu(int fd, const char *cpuname)
{
	int *id_data;
	size_t id_isar_len = 0;
	size_t id_mmfr_len = 0;
	size_t id_pfr_len = 0;
	size_t id_mvfr_len = 0;

	if (sysctlbyname("machdep.id_isar", NULL, &id_isar_len, NULL, 0) < 0
	    || sysctlbyname("machdep.id_mmfr", NULL, &id_mmfr_len, NULL, 0) < 0
	    || sysctlbyname("machdep.id_pfr", NULL, &id_pfr_len, NULL, 0) < 0
	    || sysctlbyname("machdep.id_mvfr", NULL, &id_mvfr_len, NULL, 0) < 0) {
		warn("sysctlbyname");
		return;
	}

	id_data = malloc(id_isar_len);

	sysctlbyname("machdep.id_isar", id_data, &id_isar_len, NULL, 0);
	print_features(cpuname, "isa features", id_data, id_isar_len,
	    id_isar_fieldnames, __arraycount(id_isar_fieldnames),
	    id_isar_boolean, NULL);

	free(id_data);
	id_data = malloc(id_mmfr_len);

	sysctlbyname("machdep.id_mmfr", id_data, &id_mmfr_len, NULL, 0);
	print_features(cpuname, "memory model", id_data, id_mmfr_len,
	    id_mmfr_fieldnames, __arraycount(id_mmfr_fieldnames),
	    NULL /*id_mmfr_boolean*/, id_mmfr_present);

	free(id_data);
	id_data = malloc(id_pfr_len);

	sysctlbyname("machdep.id_pfr", id_data, &id_pfr_len, NULL, 0);
	print_features(cpuname, "processor features", id_data, id_pfr_len,
	    id_pfr_fieldnames, __arraycount(id_pfr_fieldnames),
	    NULL /*id_pfr_boolean*/, NULL /*id_pfr_present*/);

	free(id_data);
	id_data = malloc(id_mvfr_len);

	sysctlbyname("machdep.id_mvfr", id_data, &id_mvfr_len, NULL, 0);
	print_features(cpuname, "media and VFP features", id_data, id_mvfr_len,
	    id_mvfr_fieldnames, __arraycount(id_mvfr_fieldnames),
	    NULL /*id_mvfr_boolean*/, id_mvfr_present);

	free(id_data);
}

int
ucodeupdate_check(int fd, struct cpu_ucode *uc)
{

	return 0;
}
