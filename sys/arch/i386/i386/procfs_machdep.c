/*	$NetBSD: procfs_machdep.c,v 1.5.2.5 2002/05/18 17:27:33 sommerfeld Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.5.2.5 2002/05/18 17:27:33 sommerfeld Exp $");

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
extern int cpu_feature;
extern char cpu_model[];
extern int cpu_class;

static const char * const i386_features[] = {
	"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
	"cx8", "apic", "10", "sep", "mtrr", "pge", "mca", "cmov",
	"fgpat", "pse36", "psn", "19", "20", "21", "22", "mmx",
	"fxsr", "xmm", "26", "27", "28", "29", "30", "31"
};


/*
 * Linux-style /proc/cpuinfo.
 * Only used when procfs is mounted with -o linux.
 *
 * In the multiprocessor case, this should be a loop over all CPUs.
 */
int
procfs_getcpuinfstr(char *buf, int *len)
{
	int left, l, i;
	char featurebuf[256], *p;
	u_int64_t freq, fraq;

	if (cpu_info_list->ci_tsc_freq != 0) {
		freq = (cpu_info_list->ci_tsc_freq + 4999) / 1000000;
		fraq = ((cpu_info_list->ci_tsc_freq + 4999) / 10000) % 100;
	}

	p = featurebuf;
	left = sizeof featurebuf;
	for (i = 0; i < 32; i++) {
		if (cpu_feature & (1 << i)) {
			l = snprintf(p, left, "%s ", i386_features[i]);
			left -= l;
			p += l;
			if (left <= 0)
				break;
		}
	}

	p = buf;
	left = *len;
	l = snprintf(p, left,
		"processor\t: %d\n"
		"vendor_id\t: %s\n"
		"cpu family\t: %d\n"
		"model\t\t: %d\n"
		"model name\t: %s\n"
		"stepping\t: ",
		0,
		(char *)cpu_info_list->ci_vendor,
		cpu_info_list->ci_cpuid_level >= 0 ?
		    ((cpu_info_list->ci_signature >> 8) & 15) : cpu_class + 3,
		cpu_info_list->ci_cpuid_level >= 0 ?
		    ((cpu_info_list->ci_signature >> 4) & 15) : 0,
		cpu_model
	    );

	left -= l;
	p += l;
	if (left <= 0)
		return 0;

	if (cpu_info_list->ci_cpuid_level >= 0)
		l = snprintf(p, left, "%d\n", cpu_info_list->ci_signature & 15);
	else
		l = snprintf(p, left, "unknown\n");

	left -= l;
	p += l;
	if (left <= 0)
		return 0;

		
	if (cpu_info_list->ci_tsc_freq != 0)
		l = snprintf(p, left, "cpu MHz\t\t: %qd.%qd\n",
		    freq, fraq);
	else
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
		i386_fpu_fdivbug ? "yes" : "no",
		i386_fpu_present ? "yes" : "no",
		i386_fpu_exception ? "yes" : "no",
		cpu_info_list->ci_cpuid_level,
		(rcr0() & CR0_WP) ? "yes" : "no",
		featurebuf);

	*len = (p + l) - buf;

	return 0;
}

#ifdef __HAVE_PROCFS_MACHDEP
void
procfs_machdep_allocvp(struct vnode *vp)
{
	struct pfsnode *pfs = vp->v_data;

	switch (pfs->pfs_type) {
	case Pmachdep_xmmregs:	/* /proc/N/xmmregs = -rw------- */
		pfs->pfs_mode = S_IRUSR|S_IWUSR;
		vp->v_type = VREG;
		break;

	default:
		panic("procfs_machdep_allocvp");
	}
}

int
procfs_machdep_rw(struct proc *curp, struct proc *p, struct pfsnode *pfs,
    struct uio *uio)
{

	switch (pfs->pfs_type) {
	case Pmachdep_xmmregs:
		return (procfs_machdep_doxmmregs(curp, p, pfs, uio));

	default:
		panic("procfs_machdep_rw");
	}

	/* NOTREACHED */
	return (EINVAL);
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
		panic("procfs_machdep_getattr");
	}

	return (0);
}

int
procfs_machdep_doxmmregs(struct proc *curp, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{

	return (process_machdep_doxmmregs(curp, p, uio));
}

int
procfs_machdep_validxmmregs(struct proc *p, struct mount *mp)
{

	return (process_machdep_validxmmregs(p));
}
#endif
