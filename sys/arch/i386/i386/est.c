/*	$NetBSD: est.c,v 1.24.4.1 2006/06/19 03:44:25 chap Exp $	*/
/*
 * Copyright (c) 2003 Michael Eriksson.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * This is a driver for Intel's Enhanced SpeedStep Technology (EST),
 * as implemented in Pentium M processors.
 *
 * Reference documentation:
 *
 * - IA-32 Intel Architecture Software Developer's Manual, Volume 3:
 *   System Programming Guide.
 *   Section 13.14, Enhanced Intel SpeedStep technology.
 *   Table B-2, MSRs in Pentium M Processors.
 *   http://www.intel.com/design/pentium4/manuals/253668.htm
 *
 * - Intel Pentium M Processor Datasheet.
 *   Table 5, Voltage and Current Specifications.
 *   http://www.intel.com/design/mobile/datashts/252612.htm
 *
 * - Intel Pentium M Processor on 90 nm Process with 2-MB L2 Cache Datasheet
 *   Table 3-4, 3-5, 3-6, Voltage and Current Specifications.
 *   http://www.intel.com/design/mobile/datashts/302189.htm
 *
 * - Linux cpufreq patches, speedstep-centrino.c.
 *   Encoding of MSR_PERF_CTL and MSR_PERF_STATUS.
 *   http://www.codemonkey.org.uk/projects/cpufreq/cpufreq-2.4.22-pre6-1.gz
 *
 *   ACPI objects: _PCT is MSR location, _PSS is freq/voltage, _PPC is caps.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: est.c,v 1.24.4.1 2006/06/19 03:44:25 chap Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/specialreg.h>

#include "opt_est.h"
#ifdef EST_FREQ_USERWRITE
#define	EST_TARGET_CTLFLAG	(CTLFLAG_READWRITE | CTLFLAG_ANYWRITE)
#else
#define	EST_TARGET_CTLFLAG	CTLFLAG_READWRITE
#endif

struct fq_info {
	int mhz;
	int mv;
};

/* Ultra Low Voltage Intel Pentium M processor 900 MHz */
static const struct fq_info pentium_m_900[] = {
	{  900, 1004 },
	{  800,  988 },
	{  600,  844 },
};

/* Ultra Low Voltage Intel Pentium M processor 1.00 GHz */
static const struct fq_info pentium_m_1000[] = {
	{ 1000, 1004 },
	{  900,  988 },
	{  800,  972 },
	{  600,  844 },
};

/* Low Voltage Intel Pentium M processor 1.10 GHz */
static const struct fq_info pentium_m_1100[] = {
	{ 1100, 1180 },
	{ 1000, 1164 },
	{  900, 1100 },
	{  800, 1020 },
	{  600,  956 },
};

/* Low Voltage Intel Pentium M processor 1.20 GHz */
static const struct fq_info pentium_m_1200[] = {
	{ 1200, 1180 },
	{ 1100, 1164 },
	{ 1000, 1100 },
	{  900, 1020 },
	{  800, 1004 },
	{  600,  956 },
};

/* Low Voltage Intel Pentium M processor 1.30 GHz */
static const struct fq_info pentium_m_1300_lv[] = {
	{ 1300, 1180 },
	{ 1200, 1164 },
	{ 1100, 1100 },
	{ 1000, 1020 },
	{  900, 1004 },
	{  800,  988 },
	{  600,  956 },
};

/* Intel Pentium M processor 1.30 GHz */
static const struct fq_info pentium_m_1300[] = {
	{ 1300, 1388 },
	{ 1200, 1356 },
	{ 1000, 1292 },
	{  800, 1260 },
	{  600,  956 },
};

/* Intel Pentium M processor 1.40 GHz */
static const struct fq_info pentium_m_1400[] = {
	{ 1400, 1484 },
	{ 1200, 1436 },
	{ 1000, 1308 },
	{  800, 1180 },
	{  600,  956 }
};

/* Intel Pentium M processor 1.50 GHz */
static const struct fq_info pentium_m_1500[] = {
	{ 1500, 1484 },
	{ 1400, 1452 },
	{ 1200, 1356 },
	{ 1000, 1228 },
	{  800, 1116 },
	{  600,  956 }
};

/* Intel Pentium M processor 1.60 GHz */
static const struct fq_info pentium_m_1600[] = {
	{ 1600, 1484 },
	{ 1400, 1420 },
	{ 1200, 1276 },
	{ 1000, 1164 },
	{  800, 1036 },
	{  600,  956 }
};

/* Intel Pentium M processor 1.70 GHz */
static const struct fq_info pentium_m_1700[] = {
	{ 1700, 1484 },
	{ 1400, 1308 },
	{ 1200, 1228 },
	{ 1000, 1116 },
	{  800, 1004 },
	{  600,  956 }
};

/* Intel Pentium M processor 723 Ultra Low Voltage 1.0 GHz */
static const struct fq_info pentium_m_n723[] = {
	{ 1000,  940 },
	{  900,  908 },
	{  800,  876 },
	{  600,  812 } 
};

/* Intel Pentium M processor 733 Ultra Low Voltage 1.1 GHz */
static const struct fq_info pentium_m_n733[] = {
	{ 1100,  940 },
	{ 1000,  924 },
	{  900,  892 },
	{  800,  876 },
	{  600,  812 }
};

/* Intel Pentium M processor 753 Ultra Low Voltage 1.2 GHz */
static const struct fq_info pentium_m_n753[] = {
	{ 1200,  940 },
	{ 1100,  924 },
	{ 1000,  908 },
	{  900,  876 },
	{  800,  860 },
	{  600,  812 }
};

/* Intel Pentium M processor 773 Ultra Low Voltage 1.3 GHz */
static const struct fq_info pentium_m_n773[] = {
	{ 1300,  940 },
	{ 1200,  924 },
	{ 1100,  908 },
	{ 1000,  892 },
	{  900,  876 },
	{  800,  860 },
	{  600,  812 }
};

/* Intel Pentium M processor 738 Low Voltage 1.4 GHz */
static const struct fq_info pentium_m_n738[] = {
	{ 1400, 1116 },
	{ 1300, 1116 },
	{ 1200, 1100 },
	{ 1100, 1068 },
	{ 1000, 1052 },
	{  900, 1036 },
	{  800, 1020 },
	{  600,  988 }
};

/* Intel Pentium M processor 758 Low Voltage 1.5 GHz */
static const struct fq_info pentium_m_n758[] = {
	{ 1500, 1116 },
	{ 1400, 1116 },
	{ 1300, 1100 },
	{ 1200, 1084 },
	{ 1100, 1068 },
	{ 1000, 1052 },
	{  900, 1036 },
	{  800, 1020 },
	{  600,  988 }
};

/* Intel Pentium M processor 778 Low Voltage 1.6 GHz */
static const struct fq_info pentium_m_n778[] = {
	{ 1600, 1116 },
	{ 1500, 1116 },
	{ 1400, 1100 },
	{ 1300, 1184 },
	{ 1200, 1068 },
	{ 1100, 1052 },
	{ 1000, 1052 },
	{  900, 1036 },
	{  800, 1020 },
	{  600,  988 } 
};

/* Intel Pentium M processor 710 1.4 GHz */
static const struct fq_info pentium_m_n710[] = {
	{ 1400, 1340 },
	{ 1200, 1228 },
	{ 1000, 1148 },
	{  800, 1068 },
	{  600,  998 }
};

/* Intel Pentium M processor 715 1.5 GHz */
static const struct fq_info pentium_m_n715[] = {
	{ 1500, 1340 },
	{ 1200, 1228 },
	{ 1000, 1148 },
	{  800, 1068 },
	{  600,  988 }
};

/* Intel Pentium M processor 725 1.6 GHz */
static const struct fq_info pentium_m_n725[] = {
	{ 1600, 1340 },
	{ 1400, 1276 },
	{ 1200, 1212 },
	{ 1000, 1132 },
	{  800, 1068 },
	{  600,  988 }
};

/* Intel Pentium M processor 730 1.6 GHz */
static const struct fq_info pentium_m_n730[] = {
       { 1600, 1308 },
       { 1333, 1260 }, 
       { 1200, 1212 },
       { 1067, 1180 },
       {  800,  988 }
};     

/* Intel Pentium M processor 735 1.7 GHz */
static const struct fq_info pentium_m_n735[] = {
	{ 1700, 1340 },
	{ 1400, 1244 },
	{ 1200, 1180 },
	{ 1000, 1116 },
	{  800, 1052 },
	{  600,  988 }
};

/* Intel Pentium M processor 740 1.73 GHz */
static const struct fq_info pentium_m_n740[] = {
       { 1733, 1356 },
       { 1333, 1212 },
       { 1067, 1100 },
       {  800,  988 },
};

/* Intel Pentium M processor 745 1.8 GHz */
static const struct fq_info pentium_m_n745[] = {
	{ 1800, 1340 },
	{ 1600, 1292 },
	{ 1400, 1228 },
	{ 1200, 1164 },
	{ 1000, 1116 },
	{  800, 1052 },
	{  600,  988 }
};

/* Intel Pentium M processor 750 1.86 GHz */
/* values extracted from \_PR\NPSS (via _PSS) SDST ACPI table */
static const struct fq_info pentium_m_n750[] = {
	{ 1867, 1308 },
	{ 1600, 1228 },
	{ 1333, 1148 },
	{ 1067, 1068 },
	{  800,  988 }
};

/* Intel Pentium M processor 755 2.0 GHz */
static const struct fq_info pentium_m_n755[] = {
	{ 2000, 1340 },
	{ 1800, 1292 },
	{ 1600, 1244 },
	{ 1400, 1196 },
	{ 1200, 1148 },
	{ 1000, 1100 },
	{  800, 1052 },
	{  600,  988 }
};

/* Intel Pentium M processor 760 2.0 GHz */
static const struct fq_info pentium_m_n760[] = {
	{ 2000, 1356 },
	{ 1600, 1244 },
	{ 1333, 1164 },
	{ 1067, 1084 },
	{  800,  988 }
};

/* Intel Pentium M processor 765 2.1 GHz */
static const struct fq_info pentium_m_n765[] = {
	{ 2100, 1340 },
	{ 1800, 1276 },
	{ 1600, 1228 },
	{ 1400, 1180 },
	{ 1200, 1132 },
	{ 1000, 1084 },
	{  800, 1036 },
	{  600,  988 }
};

/* Intel Pentium M processor 770 2.13 GHz */
static const struct fq_info pentium_m_n770[] = {
	{ 2133, 1551 },
	{ 1800, 1429 },
	{ 1600, 1356 },
	{ 1400, 1180 },
	{ 1200, 1132 },
	{ 1000, 1084 },
	{  800, 1036 },
	{  600,  988 }
};

struct fqlist {
	const char *brand_tag;
	const int cpu_id;
	size_t tablec;
	const struct fq_info *table;
	const int fsbmult; /* in multiples of 133 MHz */
};

#define ENTRY(s, i, v, f)	{ s, i, sizeof(v) / sizeof((v)[0]), v, f }
static const struct fqlist pentium_m[] = { /* Banias */
	ENTRY(" 900", 0x0695, pentium_m_900,  3),
	ENTRY("1000", 0x0695, pentium_m_1000, 3),
	ENTRY("1100", 0x0695, pentium_m_1100, 3),
	ENTRY("1200", 0x0695, pentium_m_1200, 3),
	ENTRY("1300", 0x0695, pentium_m_1300_lv, 3),
	ENTRY("1300", 0x0695, pentium_m_1300, 3),
	ENTRY("1400", 0x0695, pentium_m_1400, 3),
	ENTRY("1500", 0x0695, pentium_m_1500, 3),
	ENTRY("1600", 0x0695, pentium_m_1600, 3),
	ENTRY("1700", 0x0695, pentium_m_1700, 3),
};

static const struct fqlist pentium_m_dothan[] = {

	/* low voltage CPUs */
	ENTRY("1.00", 0x06d8, pentium_m_n723, 3),
	ENTRY("1.10", 0x06d6, pentium_m_n733, 3),
	ENTRY("1.20", 0x06d8, pentium_m_n753, 3),
	ENTRY("1.30", 0, pentium_m_n773, 3), /* does this exist? */
	
	/* ultra low voltage CPUs */
	ENTRY("1.40", 0x06d6, pentium_m_n738, 3),
	ENTRY("1.50", 0x06d8, pentium_m_n758, 3),
	ENTRY("1.60", 0x06d8, pentium_m_n778, 3),

	/* 'regular' 400 MHz FSB CPUs */
	ENTRY("1.40", 0x06d6, pentium_m_n710, 3),
	ENTRY("1.50", 0x06d6, pentium_m_n715, 3),
	ENTRY("1.60", 0x06d6, pentium_m_n725, 3),
	ENTRY("1.70", 0x06d6, pentium_m_n735, 3),
	ENTRY("1.80", 0x06d6, pentium_m_n745, 3),
	ENTRY("2.00", 0x06d6, pentium_m_n755, 3),
	ENTRY("2.10", 0x06d6, pentium_m_n765, 3),

	/* 533 MHz FSB CPUs */
	ENTRY("1.60", 0x06d8, pentium_m_n730, 4),
	ENTRY("1.73", 0x06d8, pentium_m_n740, 4),
	ENTRY("1.86", 0x06d8, pentium_m_n750, 4),
	ENTRY("2.00", 0x06d8, pentium_m_n760, 4),
	ENTRY("2.13", 0x06d8, pentium_m_n770, 4),

};
#undef ENTRY

struct est_cpu {
	const char *brand_prefix;
	const char *brand_suffix;
	size_t listc;
	const struct fqlist *list;
};

static const struct est_cpu est_cpus[] = {
	{
		"Intel(R) Pentium(R) M processor ", "MHz",
		(sizeof(pentium_m) / sizeof(pentium_m[0])),
		pentium_m
	},
	{
		"Intel(R) Pentium(R) M processor ", "GHz",
		(sizeof(pentium_m_dothan) / sizeof(pentium_m_dothan[0])),
		pentium_m_dothan
	},
};

#define NESTCPUS  (sizeof(est_cpus) / sizeof(est_cpus[0]))

#define MSR2MV(msr)	(((int) (msr) & 0xff) * 16 + 700)
#define MSR2MHZ(msr)	(((((int) (msr) >> 8) & 0xff) * 100 * fsbmult + 1)/ 3)
#define MV2MSR(mv)	((((int) (mv) - 700) >> 4) & 0xff)
#define MHZ2MSR(mhz)	(((3 * (mhz + 30) / (100 * fsbmult)) & 0xff) << 8)
/* XXX 30 is slop to deal with the 33.333 MHz roundoff values */

static const struct fqlist *est_fqlist;	/* not NULL if functional */
static int	est_node_target, est_node_current;
static int	fsbmult;

static const char est_desc[] = "Enhanced SpeedStep";

static int
est_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode	node;
	int			fq, oldfq, error;

	if (est_fqlist == NULL)
		return (EOPNOTSUPP);

	node = *rnode;
	node.sysctl_data = &fq;

	oldfq = 0;
	if (rnode->sysctl_num == est_node_target)
		fq = oldfq = MSR2MHZ(rdmsr(MSR_PERF_CTL));
	else if (rnode->sysctl_num == est_node_current)
		fq = MSR2MHZ(rdmsr(MSR_PERF_STATUS));
	else
		return (EOPNOTSUPP);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

		/* support writing to ...frequency.target */
	if (rnode->sysctl_num == est_node_target && fq != oldfq) {
		int		i;
		uint64_t	msr;

		for (i = est_fqlist->tablec - 1; i > 0; i--)
			if (est_fqlist->table[i].mhz >= fq)
				break;
		fq = est_fqlist->table[i].mhz;
		msr = (rdmsr(MSR_PERF_CTL) & ~0xffffULL) |
		    MV2MSR(est_fqlist->table[i].mv) |
		    MHZ2MSR(est_fqlist->table[i].mhz);
		wrmsr(MSR_PERF_CTL, msr);
	}

	return (0);
}


void
est_init(struct cpu_info *ci)
{
	const struct est_cpu	*ccpu;
	const struct fqlist	*fql;
	const struct sysctlnode	*node, *estnode, *freqnode;
	uint64_t		msr;
	int			i, j, k, rc;
	int			mv;
	size_t			len, freq_len;
	char			*tag, *freq_names;

	if ((cpu_feature2 & CPUID2_EST) == 0)
		return;

	msr = rdmsr(MSR_PERF_STATUS);
	mv = MSR2MV(msr);
	aprint_normal("%s: %s (%d mV) ",
	       ci->ci_dev->dv_xname, est_desc, mv);

	/*
	 * Look for a CPU matching cpu_brand_string.
	 */
	for (i = 0; est_fqlist == NULL && i < NESTCPUS; i++) {
		ccpu = &est_cpus[i];
		len = strlen(ccpu->brand_prefix);
		if (strncmp(ccpu->brand_prefix, cpu_brand_string, len) != 0)
			continue;
		tag = cpu_brand_string + len;
		for (j = 0; j < ccpu->listc; j++) {
			fql = &ccpu->list[j];
			len = strlen(fql->brand_tag);
			if (!strncmp(fql->brand_tag, tag, len) &&
			    !strcmp(ccpu->brand_suffix, tag + len) &&
			    (fql->cpu_id == 0 ||
			     fql->cpu_id == ci->ci_signature)) {
				/* verify operating point is in table, because
				   CPUID + brand_tag still isn't unique. */
				for (k = fql->tablec - 1; k >= 0; k--) {
					if (fql->table[k].mv == mv) {
						est_fqlist = fql;
						break;
					}
				}
			}
			if (NULL != est_fqlist) break;
		}
	}
	if (est_fqlist == NULL) {
		aprint_normal(" - unknown CPU or operating point.\n");
		return;
	}

	/*
	 * OK, tell the user the available frequencies.
	 */
	fsbmult = est_fqlist->fsbmult;
	aprint_normal("%d MHz\n", MSR2MHZ(msr));

	freq_len = est_fqlist->tablec * (sizeof("9999 ")-1) + 1;
	freq_names = malloc(freq_len, M_SYSCTLDATA, M_WAITOK);
	freq_names[0] = '\0';
	len = 0;
	for (i = 0; i < est_fqlist->tablec; i++) {
		len += snprintf(freq_names + len, freq_len - len, "%d%s",
		    est_fqlist->table[i].mhz,
		    i < est_fqlist->tablec - 1 ? " " : "");
	}
	aprint_normal("%s: %s frequencies available (MHz): %s\n",
	    ci->ci_dev->dv_xname, est_desc, freq_names);

	/*
	 * Setup the sysctl sub-tree machdep.est.*
	 */
	if ((rc = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(NULL, 0, &node, &estnode,
	    0, CTLTYPE_NODE, "est", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(NULL, 0, &estnode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(NULL, 0, &freqnode, &node,
	    EST_TARGET_CTLFLAG, CTLTYPE_INT, "target", NULL,
	    est_sysctl_helper, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	est_node_target = node->sysctl_num;

	if ((rc = sysctl_createv(NULL, 0, &freqnode, &node,
	    0, CTLTYPE_INT, "current", NULL,
	    est_sysctl_helper, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
	est_node_current = node->sysctl_num;

	if ((rc = sysctl_createv(NULL, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, freq_names, freq_len, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return;
 err:
	free(freq_names, M_SYSCTLDATA);
	aprint_normal("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}
