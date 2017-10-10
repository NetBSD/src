/*	$NetBSD: procfs_machdep.c,v 1.20 2017/10/10 03:05:29 msaitoh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.20 2017/10/10 03:05:29 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vnode.h>

#include <miscfs/procfs/procfs.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <x86/cputypes.h>
#include <x86/cpuvar.h>

/*
 *  The feature table. The order is the same as Linux's
 *  x86/include/asm/cpufeatures.h.
 */
static const char * const x86_features[][32] = {
	{ /* (0) Common: 0x0000001 edx */
	"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
	"cx8", "apic", NULL, "sep", "mtrr", "pge", "mca", "cmov",
	"pat", "pse36", "pn", "clflush", NULL, "dts", "acpi", "mmx",
	"fxsr", "sse", "sse2", "ss", "ht", "tm", "ia64", "pbe"},

	{ /* (1) AMD-defined: 0x80000001 edx */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "syscall", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "mp", "nx", NULL, "mmxext", NULL,
	NULL, "fxsr_opt", "pdpe1gb", "rdtscp", NULL, "lm", "3dnowext","3dnow"},

	{ /* (2) Transmeta-defined */
	"recovery", "longrun", NULL, "lrti", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (3) Linux mapping */
	"cxmmx", NULL, "cyrix_arr", "centaur_mcr", NULL,
	"constant_tsc", NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (4) Intel-defined: 0x00000001 ecx */
	"pni", "pclmulqdq", "dtes64", "monitor", "ds_cpl", "vmx", "smx", "est",
	"tm2", "ssse3", "cid", "sdbg", "fma", "cx16", "xtpr", "pdcm",
	NULL, "pcid", "dca", "sse4_1", "sse4_2", "x2apic", "movbe", "popcnt",
	"tsc_deadline_timer", "aes", "xsave", NULL,
	"avx", "f16c", "rdrand", "hypervisor"},

	{ /* (5) VIA/Cyrix/Centaur-defined */
	NULL, NULL, "rng", "rng_en", NULL, NULL, "ace", "ace_en",
	"ace2", "ace2_en", "phe", "phe_en", "pmm", "pmm_en", NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (6) AMD defined 80000001 ecx */
	"lahf_lm", "cmp_legacy", "svm", "extapic",
	"cr8_legacy", "abm", "sse4a", "misalignsse",
	"3dnowprefetch", "osvw", "ibs", "xop", "skinit", "wdt", NULL, "lwp",
	"fma4", "tce", NULL, "nodeid_msr",
	NULL, "tbm", "topoext", "perfctr_core",
	"perfctr_nb", NULL, "bpext", "ptsc",
	"perfctr_llc", "mwaitx", NULL, NULL},

	{ /* (7) Linux mapping */
	NULL, NULL, "cpb", "ebp", NULL, "pln", "pts", "dtherm",
	"hw_pstate", "proc_feedback", "sme", NULL,
	NULL, NULL, NULL, "intel_pt",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (8) Linux mapping */
	"tpr_shadow", "vnmi", "flexpriority", "ept",
	"vpid", "npt", "lbrv", "svm_lock",
	"nrip_save", "tsc_scale", "vmcb_clean", "flushbyasid",
	"decodeassists", "pausefilter", "pfthreshold", "vmmcall",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (9) Intel-defined: 00000007 ebx */
	"fsgsbase", "tsc_adjust", NULL, "bmi1", "hle", "avx2", NULL, "smep",
	"bmi2", "erms", "invpcid", "rtm", "cqm", NULL, "mpx", "rdt_a",
	"avx512f", "avx512dq", "rdseed", "adx",
	"smap", NULL, NULL, "clflushopt",
	"clwb", NULL, "avx512pf", "avx512er",
	"avx512cd", "sha_ni", "avx512bw", "avx512vl"},

	{ /* (10) 0x0000000d:1 eax */
	"xsaveopt", "xsavec", "xgetbv1", "xsaves", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (11) 0x0000000f:0 edx */
	NULL, "cqm_llc", NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (12) 0x0000000f:1 edx */
	"cqm_occup_llc", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (13) 0x80000008 ebx */
	"clzero", "irperf", NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (14) 0x00000006 eax */
	"dtherm", "ida", "arat", NULL, "pln", NULL, "pts", "hwp",
	"hwp_notify", "hwp_act_window", "hwp_epp","hwp_pkg_req",
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (15) 0x8000000a edx */
	"npt", "lbrv", "svm_lock", "nrip_save",
	"tsc_scale", "vmcb_clean", "flushbyasid", "decodeassists",
	NULL, NULL, "pausefilter", NULL, "pfthreshold", "avic", NULL,
	"v_vmsave_vmload",
	"vgif", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (16) 0x00000007:0 ecx */
	NULL, "avx512vbmi", NULL, "pku", "ospke", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, "avx512_vpopcntdq", NULL,
	"la57", NULL, NULL, NULL, NULL, NULL, "rdpid", NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},

	{ /* (17) 0x80000007 ebx */
	"overflow_recov", "succor", NULL, "smca", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
};

static int	procfs_getonecpu(int, struct cpu_info *, char *, size_t *);

/*
 * Linux-style /proc/cpuinfo.
 * Only used when procfs is mounted with -o linux.
 *
 * In the multiprocessor case, this should be a loop over all CPUs.
 */
int
procfs_getcpuinfstr(char *bf, size_t *len)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	size_t i, total, size, used;

	i = total = 0;
	used = size = *len;

	for (CPU_INFO_FOREACH(cii, ci)) {
		procfs_getonecpu(i++, ci, bf, &used);
		total += used + 1;
		if (used + 1 <= size) {
			bf += used;
			*bf++ = '\n';
			size -= used + 1;
			used = size;
		} else
			used = 0;
	}
	size = *len;
	*len = total;
	return size < *len ? -1 : 0;
}

static int
procfs_getonefeatreg(uint32_t reg, const char * const *table, char *p,
    size_t *left)
{
	size_t l;

	for (size_t i = 0; i < 32; i++) {
		if ((reg & (1 << i)) && table[i]) {
			l = snprintf(p, *left, "%s ", table[i]);
			if (l < *left) {
				*left -= l;
				p += l;
			} else
				break;
		}
	}

	return 0; /* XXX */
}

/*
 * Print feature bits. The code assume that unused entry of x86_features[]
 * is zero-cleared.
 *
 * XXX This function will be rewritten when all of linux entries are
 * decoded.
 */
static int
procfs_getonecpufeatures(struct cpu_info *ci, char *p, size_t *left)
{
	size_t last = *left;
	size_t diff;
	u_int descs[4];

	procfs_getonefeatreg(ci->ci_feat_val[0], x86_features[0], p, left);
	diff = last - *left;

	procfs_getonefeatreg(ci->ci_feat_val[2], x86_features[1], p + diff,
	    left);
	diff = last - *left;

	/* x86_features[2] is for Transmeta */
	/* x86_features[3] is Linux defined mapping */
	
	procfs_getonefeatreg(ci->ci_feat_val[1], x86_features[4], p + diff,
	    left);
	diff = last - *left;

	procfs_getonefeatreg(ci->ci_feat_val[4], x86_features[5], p + diff,
	    left);
	diff = last - *left;

	procfs_getonefeatreg(ci->ci_feat_val[3], x86_features[6], p + diff,
	    left);
	diff = last - *left;

	/* x86_features[7] is Linux defined mapping */
	/* x86_features[8] is Linux defined mapping */

	procfs_getonefeatreg(ci->ci_feat_val[5], x86_features[9], p + diff,
	    left);
	diff = last - *left;

	if (ci->ci_max_cpuid >= 0x0d) {
		x86_cpuid2(0x0d, 1, descs);
		procfs_getonefeatreg(descs[0], x86_features[10], p + diff,
		    left);
		diff = last - *left;
	}

	if (ci->ci_max_cpuid >= 0x0f) {
		x86_cpuid2(0x0f, 0, descs);
		procfs_getonefeatreg(descs[3], x86_features[11], p + diff,
		    left);
		diff = last - *left;

		x86_cpuid2(0x0f, 1, descs);
		procfs_getonefeatreg(descs[3], x86_features[12], p + diff,
		    left);
		diff = last - *left;
	}

	if ((cpu_vendor == CPUVENDOR_AMD)
	    && (ci->ci_max_ext_cpuid >= 0x80000008)) {
		x86_cpuid(0x80000008, descs);
		procfs_getonefeatreg(descs[1], x86_features[13], p + diff,
		    left);
		diff = last - *left;
	}

	if (ci->ci_max_cpuid >= 0x06) {
		x86_cpuid(0x06, descs);
		procfs_getonefeatreg(descs[0], x86_features[14], p + diff,
		    left);
		diff = last - *left;
	}

	if ((cpu_vendor == CPUVENDOR_AMD)
	    && (ci->ci_max_ext_cpuid >= 0x8000000a)) {
		x86_cpuid(0x8000000a, descs);
		procfs_getonefeatreg(descs[3], x86_features[15], p + diff,
		    left);
		diff = last - *left;
	}

	procfs_getonefeatreg(ci->ci_feat_val[6], x86_features[16], p + diff,
	    left);
	diff = last - *left;

	if ((cpu_vendor == CPUVENDOR_AMD)
	    && (ci->ci_max_ext_cpuid >= 0x80000007)) {
		x86_cpuid(0x80000007, descs);
		procfs_getonefeatreg(descs[1], x86_features[17], p + diff,
		    left);
		diff = last - *left;
	}

	return 0; /* XXX */
}

static int
procfs_getonecpu(int xcpu, struct cpu_info *ci, char *bf, size_t *len)
{
	size_t left, l, size;
	char featurebuf[1024], *p;

	p = featurebuf;
	left = sizeof(featurebuf);
	size = *len;
	procfs_getonecpufeatures(ci, p, &left);

	p = bf;
	left = *len;
	size = 0;
	l = snprintf(p, left,
	    "processor\t: %d\n"
	    "vendor_id\t: %s\n"
	    "cpu family\t: %d\n"
	    "model\t\t: %d\n"
	    "model name\t: %s\n"
	    "stepping\t: ",
	    xcpu,
	    (char *)ci->ci_vendor,
	    CPUID_TO_FAMILY(ci->ci_signature),
	    CPUID_TO_MODEL(ci->ci_signature),
	    cpu_brand_string
	);
	size += l;
	if (l < left) {
		left -= l;
		p += l;
	} else
		left = 0;

	if (cpuid_level >= 0)
		l = snprintf(p, left, "%d\n",
		    CPUID_TO_STEPPING(ci->ci_signature));
	else
		l = snprintf(p, left, "unknown\n");

	size += l;
	if (l < left) {
		left -= l;
		p += l;
	} else
		left = 0;

	if (ci->ci_data.cpu_cc_freq != 0) {
		uint64_t freq, fraq;

		freq = (ci->ci_data.cpu_cc_freq + 4999) / 1000000;
		fraq = ((ci->ci_data.cpu_cc_freq + 4999) / 10000) % 100;
		l = snprintf(p, left, "cpu MHz\t\t: %" PRIu64 ".%02" PRIu64
		    "\n", freq, fraq);
	} else
		l = snprintf(p, left, "cpu MHz\t\t: unknown\n");

	size += l;
	if (l < left) {
		left -= l;
		p += l;
	} else
		left = 0;

	l = snprintf(p, left,
	    "apicid\t\t: %d\n"
	    "initial apicid\t: %d\n",
	    ci->ci_acpiid,
	    ci->ci_initapicid
	);
	size += l;
	if (l < left) {
		left -= l;
		p += l;
	} else
		left = 0;

	l = snprintf(p, left,
#ifdef __i386__
	    "fdiv_bug\t: %s\n"
#endif
	    "fpu\t\t: yes\n"
	    "fpu_exception\t: yes\n"
	    "cpuid level\t: %d\n"
	    "wp\t\t: %s\n"
	    "flags\t\t: %s\n"
	    "clflush size\t: %d\n",
#ifdef __i386__
	    i386_fpu_fdivbug ? "yes" : "no",	/* an old pentium */
#endif
	    ci->ci_max_cpuid,
	    (rcr0() & CR0_WP) ? "yes" : "no",
	    featurebuf,
	    ci->ci_cflush_lsize
	);
	size += l;

	left = *len;
	*len = size;
	return left < *len ? -1 : 0;
}

#if defined(__HAVE_PROCFS_MACHDEP) && !defined(__x86_64__)

void
procfs_machdep_allocvp(struct vnode *vp)
{
	struct pfsnode *pfs = vp->v_data;

	switch (pfs->pfs_type) {
	case Pmachdep_xmmregs:
		/* /proc/N/xmmregs = -rw------- */
		pfs->pfs_mode = S_IRUSR|S_IWUSR;
		vp->v_type = VREG;
		break;
	default:
		KASSERT(false);
	}
}

int
procfs_machdep_rw(struct lwp *curl, struct lwp *l, struct pfsnode *pfs,
    struct uio *uio)
{

	switch (pfs->pfs_type) {
	case Pmachdep_xmmregs:
		return (procfs_machdep_doxmmregs(curl, l, pfs, uio));
	default:
		KASSERT(false);
	}
	return EINVAL;
}

int
procfs_machdep_getattr(struct vnode *vp, struct vattr *vap, struct proc *procp)
{
	struct pfsnode *pfs = VTOPFS(vp);

	switch (pfs->pfs_type) {
	case Pmachdep_xmmregs:
		vap->va_bytes = vap->va_size = sizeof(struct xmmregs);
		break;
	default:
		KASSERT(false);
	}
	return 0;
}

int
procfs_machdep_doxmmregs(struct lwp *curl, struct lwp *l,
    struct pfsnode *pfs, struct uio *uio)
{

	return process_machdep_doxmmregs(curl, l, uio);
}

int
procfs_machdep_validxmmregs(struct lwp *l, struct mount *mp)
{

	return process_machdep_validxmmregs(l->l_proc);
}

#endif
