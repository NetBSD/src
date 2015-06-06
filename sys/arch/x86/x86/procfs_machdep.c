/*	$NetBSD: procfs_machdep.c,v 1.6.6.1 2015/06/06 14:40:04 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.6.6.1 2015/06/06 14:40:04 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vnode.h>

#include <miscfs/procfs/procfs.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/specialreg.h>

static const char * const x86_features[] = {
	/* Intel-defined */
	"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
	"cx8", "apic", NULL, "sep", "mtrr", "pge", "mca", "cmov",
	"pat", "pse36", "pn", "clflush", NULL, "dts", "acpi", "mmx",
	"fxsr", "sse", "sse2", "ss", "ht", "tm", "ia64", NULL,
#ifdef __x86_64__
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
#endif
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
		if (used + 1 < size) {
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
procfs_getonecpu(int xcpu, struct cpu_info *ci, char *bf, size_t *len)
{
	size_t left, l, size;
	char featurebuf[1024], *p;

	p = featurebuf;
	left = sizeof(featurebuf);
	size = *len;
	for (size_t i = 0; i < 32; i++) {
		if ((ci->ci_feat_val[0] & (1 << i)) && x86_features[i]) {
			l = snprintf(p, left, "%s ", x86_features[i]);
			if (l < left) {
				left -= l;
				p += l;
			} else
				break;
		}
	}

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
	    cpuid_level >= 0 ? ((ci->ci_signature >> 8) & 15) : cpu_class + 3,
	    cpuid_level >= 0 ? ((ci->ci_signature >> 4) & 15) : 0,
	    cpu_brand_string
	);
	size += l;
	if (l < left) {
		left -= l;
		p += l;
	} else
		left = 0;

	if (cpuid_level >= 0)
		l = snprintf(p, left, "%d\n", ci->ci_signature & 15);
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
		l = snprintf(p, left, "cpu MHz\t\t: %" PRIu64 ".%02" PRIu64 "\n",
		    freq, fraq);
	} else
		l = snprintf(p, left, "cpu MHz\t\t: unknown\n");

	size += l;
	if (l < left) {
		left -= l;
		p += l;
	} else
		left = 0;

	l = snprintf(p, left,
	    "fdiv_bug\t: %s\n"
	    "fpu\t\t: %s\n"
	    "fpu_exception\t: yes\n"
	    "cpuid level\t: %d\n"
	    "wp\t\t: %s\n"
	    "flags\t\t: %s\n",
	    i386_fpu_fdivbug ? "yes" : "no",	/* an old pentium */
	    i386_fpu_present ? "yes" : "no",	/* not a 486SX */
	    cpuid_level,
	    (rcr0() & CR0_WP) ? "yes" : "no",
	    featurebuf
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
