/*	$NetBSD: procfs_machdep.c,v 1.6.28.1 2008/01/09 01:44:47 matt Exp $ */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden and Jason R. Thorpe for
 * Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NOTE: We simply use the primary CPU's cpuid_level and tsc_freq
 * here.  Might want to change this later.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.6.28.1 2008/01/09 01:44:47 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vnode.h>

#include <miscfs/procfs/procfs.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/specialreg.h>

extern int i386_fpu_present, i386_fpu_exception, i386_fpu_fdivbug;
extern char cpu_model[];
extern int cpu_class;

static const char * const i386_features[] = {
	/* Intel-defined */
	"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
	"cx8", "apic", NULL, "sep", "mtrr", "pge", "mca", "cmov",
	"pat", "pse36", "pn", "clflush", NULL, "dts", "acpi", "mmx",
	"fxsr", "sse", "sse2", "ss", "ht", "tm", "ia64", NULL,
	
	/* AMD-defined */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, "syscall", NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, "nx", NULL, "mmxext", NULL, 
	NULL, "fxsr_opt", "rdtscp", NULL, NULL, "lm", "3dnowext", "3dnow",

	/* Transmeta-defined */
	"recovery", "longrun", NULL, "lrti", NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

	/* Linux-defined */
	"cxmmx", NULL, "cyrix_arr", "centaur_mcr", NULL, 
	"constant_tsc", NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

	/* Intel-defined */
	"pni", NULL, NULL, "monitor", "ds_cpi", "vmx", NULL, "est", 
	"tm2", NULL, "cid", NULL, NULL, "cx16", "xtpr", NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

	/* VIA/Cyrix/Centaur-defined */
	NULL, NULL, "rng", "rng_en", NULL, NULL, "ace", "ace_en", 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

	/* AMD defined */
	"lahf_lm", "cmp_legacy", "svm", NULL, "cr8_legacy", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
};

static int procfs_getonecpu(int, struct cpu_info *, char *, int *);

/*
 * Linux-style /proc/cpuinfo.
 * Only used when procfs is mounted with -o linux.
 *
 * In the multiprocessor case, this should be a loop over all CPUs.
 */
int
procfs_getcpuinfstr(char *bf, int *len)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	int i = 0, used = *len, total = *len;

	*len = 0;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (procfs_getonecpu(i++, ci, bf, &used) == 0) {
			*len += used;
			total = 0;
			break;
		}
		total -= used;
		if (total > 0) {
			bf += used;
			*bf++ = '\n';
			*len += used + 1;
			used = --total;
			if (used == 0)
			    break;
		} else {
			*len += used;
			break;
		}
	}
	return total == 0 ? -1 : 0;
}

static int
procfs_getonecpu(int xcpu, struct cpu_info *ci, char *bf, int *len)
{
	int left, l, i;
	char featurebuf[256], *p;

	p = featurebuf;
	left = sizeof featurebuf;
	for (i = 0; i < sizeof(i386_features)/sizeof(*i386_features); i++) {
		if ((ci->ci_feature_flags & (1 << i)) &&
		    (i386_features[i] != NULL)) {
			l = snprintf(p, left, "%s ", i386_features[i]);
			left -= l;
			p += l;
			if (left <= 0)
				break;
		}
	}

	p = bf;
	left = *len;
	l = snprintf(p, left,
		"processor\t: %d\n"
		"vendor_id\t: %s\n"
		"cpu family\t: %d\n"
		"model\t\t: %d\n"
		"model name\t: %s\n"
		"stepping\t: ",
		xcpu,
		cpu_vendorname,
		cpuid_level >= 0 ?
		    ((ci->ci_signature >> 8) & 15) : cpu_class + 3,
		cpuid_level >= 0 ?
		    ((ci->ci_signature >> 4) & 15) : 0,
		cpu_model
	    );

	left -= l;
	p += l;
	if (left <= 0)
		return 0;

	if (cpuid_level >= 0)
		l = snprintf(p, left, "%d\n", ci->ci_signature & 15);
	else
		l = snprintf(p, left, "unknown\n");

	left -= l;
	p += l;
	if (left <= 0)
		return 0;

		
	if (ci->ci_tsc_freq != 0) {
		uint64_t freq, fraq;

		freq = (ci->ci_tsc_freq + 4999) / 1000000;
		fraq = ((ci->ci_tsc_freq + 4999) / 10000) % 100;
		l = snprintf(p, left, "cpu MHz\t\t: %ld.%ld\n",
		    freq, fraq);
	} else
		l = snprintf(p, left, "cpu MHz\t\t: unknown\n");

	left -= l;
	p += l;
	if (left <= 0)
		return 0;

	l = snprintf(p, left,
		"fdiv_bug\t: %s\n"
		"fpu\t\t: %s\n"
		"fpu_exception:\t: %s\n"
		"cpuid level\t: %d\n"
		"wp\t\t: %s\n"
		"flags\t\t: %s\n",
		"no",	/* XXX */
		"yes",	/* XXX */
		"yes",	/* XXX */
		cpuid_level,
		(rcr0() & CR0_WP) ? "yes" : "no",
		featurebuf);

	if (l > left)
		return 0;
	*len = (p + l) - bf;

	return 1;
}
