/*	$NetBSD: powernow_k7.c,v 1.19 2006/09/24 11:12:40 xtraeme Exp $ */
/*	$OpenBSD: powernow-k7.c,v 1.24 2006/06/16 05:58:50 gwk Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Vegiard.
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

/*-
 * Copyright (c) 2004-2005 Bruno Ducrot
 * Copyright (c) 2004 FUKUDA Nobuhiko <nfukuda@spa.is.uec.ac.jp>
 * Copyright (c) 2004 Martin Vegiard
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* AMD POWERNOW K7 driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powernow_k7.c,v 1.19 2006/09/24 11:12:40 xtraeme Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/bus.h>

#include <dev/isa/isareg.h>

#include <x86/include/isa_machdep.h>
#include <x86/include/powernow.h>

#ifdef _LKM
static struct sysctllog	*sysctllog;
#define SYSCTLLOG	&sysctllog
#else
#define SYSCTLLOG	NULL
#endif

/*
 * Divide each value by 10 to get the processor multiplier.
 * Taken from powernow-k7.c/Linux by Dave Jones
 */
static int k7pnow_fid_to_mult[32] = {
	110, 115, 120, 125, 50, 55, 60, 65,
	70, 75, 80, 85, 90, 95, 100, 105,
	30, 190, 40, 200, 130, 135, 140, 210,
	150, 225, 160, 165, 170, 180, -1, -1
};

/*
 * The following CPUs do not have same CPUID signature
 * in the PST tables, but they have correct FID/VID values.
 */
static struct pnow_cpu_quirk {
	uint32_t rcpusig;	/* Correct cpu signature */
	uint32_t pcpusig;	/* PST cpu signature */
} pnow_cpu_quirk[] = {
	{ 0x6a0, 0x781 },	/* Reported by Eric Schnoebelen */
	{ 0x6a0, 0x7a0 },	/* Reported by Nino Denhe */
	{ 0, 0}
};

static struct powernow_cpu_state *k7pnow_current_state;
static unsigned int cur_freq, cpu_mhz;
static int powernow_node_target, powernow_node_current;
static char *freq_names;
static size_t freq_names_len;
static uint8_t k7pnow_flag;

static boolean_t pnow_cpu_quirk_check(uint32_t, uint32_t);
int k7_powernow_setperf(unsigned int);
int k7pnow_sysctl_helper(SYSCTLFN_PROTO);
int k7pnow_decode_pst(struct powernow_cpu_state *, uint8_t *, int);
int k7pnow_states(struct powernow_cpu_state *, uint32_t, unsigned int, unsigned int);

static boolean_t
pnow_cpu_quirk_check(uint32_t real_cpusig, uint32_t pst_cpusig)
{
	int j;

	for (j = 0; pnow_cpu_quirk[j].rcpusig != 0; j++) {
		if ((real_cpusig == pnow_cpu_quirk[j].rcpusig) &&
		    (pst_cpusig == pnow_cpu_quirk[j].pcpusig))
			return TRUE;
	}

	return FALSE;
}

int
k7pnow_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int fq, oldfq, error;

	node = *rnode;
	node.sysctl_data = &fq;

	oldfq = 0;
	if (rnode->sysctl_num == powernow_node_target)
		fq = oldfq = cur_freq;
	else if (rnode->sysctl_num == powernow_node_current)
		fq = cur_freq;
	else
		return EOPNOTSUPP;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* support writing to ...frequency.target */
	if (rnode->sysctl_num == powernow_node_target && fq != oldfq) {
		if (k7_powernow_setperf(fq) == 0)
			cur_freq = fq;
		else
			return EINVAL;
	}

	return 0;
}

int
k7_powernow_setperf(unsigned int freq)
{
	unsigned int i;
	int cvid, cfid, vid = 0, fid = 0;
	uint64_t status, ctl;
	struct powernow_cpu_state * cstate;

	cstate = k7pnow_current_state;

	DPRINTF(("%s: cstate->n_states=%d\n", __func__, cstate->n_states));
	for (i = 0; i < cstate->n_states; i++) {
		if (cstate->state_table[i].freq >= freq) {
			DPRINTF(("%s: freq=%d\n", __func__, freq));
			fid = cstate->state_table[i].fid;
			vid = cstate->state_table[i].vid;
			DPRINTF(("%s: fid=%d vid=%d\n", __func__, fid, vid));
			break;
		}
	}

	if (fid == 0 || vid == 0)
		return 0;

	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	cfid = PN7_STA_CFID(status);
	cvid = PN7_STA_CVID(status);

	/*
	 * We're already at the requested level.
	 */
	if (fid == cfid && vid == cvid)
		return 0;

	ctl = rdmsr(MSR_AMDK7_FIDVID_CTL) & PN7_CTR_FIDCHRATIO;

	ctl |= PN7_CTR_FID(fid);
	ctl |= PN7_CTR_VID(vid);
	ctl |= PN7_CTR_SGTC(cstate->sgtc);

	if (k7pnow_flag & PN7_FLAG_ERRATA_A0)
		disable_intr();

	if (k7pnow_fid_to_mult[fid] < k7pnow_fid_to_mult[cfid]) {
		wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_FIDC);
		if (vid != cvid)
			wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_VIDC);
	} else {
		wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_VIDC);
		if (fid != cfid)
			wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_FIDC);
	}

	if (k7pnow_flag & PN7_FLAG_ERRATA_A0)
		enable_intr();

	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	cfid = PN7_STA_CFID(status);
	cvid = PN7_STA_CVID(status);
	if (cfid == fid || cvid == vid)
		freq = cstate->state_table[i].freq;

	return 0;
}

/*
 * Given a set of pair of fid/vid, and number of performance states,
 * compute state_table via an insertion sort.
 */
int
k7pnow_decode_pst(struct powernow_cpu_state *cstate, uint8_t *p, int npst)
{
	int i, j, n;
	struct powernow_state state;

	for (n = 0, i = 0; i < npst; ++i) {
		state.fid = *p++;
		state.vid = *p++;
		state.freq = k7pnow_fid_to_mult[state.fid]/10 * cstate->fsb;
		if ((k7pnow_flag & PN7_FLAG_ERRATA_A0) &&
		    (k7pnow_fid_to_mult[state.fid] % 10) == 5)
			continue;

		j = n;
		while (j > 0 && cstate->state_table[j - 1].freq > state.freq) {
			memcpy(&cstate->state_table[j],
			    &cstate->state_table[j - 1],
			    sizeof(struct powernow_state));
			--j;
		}
		memcpy(&cstate->state_table[j], &state,
		    sizeof(struct powernow_state));
		++n;
	}
	/*
	 * Fix powernow_max_states, if errata_a0 give us less states
	 * than expected.
	 */
	cstate->n_states = n;
	return 1;
}

int
k7pnow_states(struct powernow_cpu_state *cstate, uint32_t cpusig,
    unsigned int fid, unsigned int vid)
{
	int j, maxpst;
	struct powernow_psb_s *psb;
	struct powernow_pst_s *pst;
	uint8_t *p;
	boolean_t cpusig_ok;

	j = 0;
	/*
	 * Look in the 0xe0000 - 0x100000 physical address
	 * range for the pst tables; 16 byte blocks
	 */
	for (p = (uint8_t *)ISA_HOLE_VADDR(BIOS_START);
	    p < (uint8_t *)ISA_HOLE_VADDR(BIOS_START + BIOS_LEN);
	    p+= BIOS_STEP) {
		if (memcmp(p, "AMDK7PNOW!", 10) == 0) {
			psb = (struct powernow_psb_s *)p;
			if (psb->version != PN7_PSB_VERSION)
				return 0;

			cstate->sgtc = psb->ttime * cstate->fsb;
			if (cstate->sgtc < 100 * cstate->fsb)
				cstate->sgtc = 100 * cstate->fsb;
			if (psb->flags & 1)
				k7pnow_flag |= PN7_FLAG_DESKTOP_VRM;
			p += sizeof(struct powernow_psb_s);

			for (maxpst = 0; maxpst < psb->n_pst; maxpst++) {
				pst = (struct powernow_pst_s*) p;

				/*
				 * The model with cpuid 0x6a0 has a different
				 * signature in the PST table. Accept to
				 * report frequencies via a quirk table
				 * and fid/vid match.
				 */
				DPRINTF(("%s: cpusig=0x%x pst->signature:0x%x\n",
				    __func__, cpusig, pst->signature));

				cpusig_ok = pnow_cpu_quirk_check(cpusig,
						pst->signature);

				if (((cpusig == pst->signature) &&
				    (fid == pst->fid && vid == pst->vid)) ||
				    ((cpusig_ok == TRUE) && 
				    (fid == pst->fid && vid == pst->vid))) {
					DPRINTF(("%s: pst->signature ok\n",
					    __func__));
					if (abs(cstate->fsb - pst->pll) > 5)
						continue;
					cstate->n_states = pst->n_states;
					return (k7pnow_decode_pst(cstate,
					    p + sizeof(struct powernow_pst_s),
					    cstate->n_states));
				}
				p += sizeof(struct powernow_pst_s) +
				    (2 * pst->n_states);
			}
		}
	}

	return 0;
}

void
k7_powernow_init(void)
{
	uint64_t status;
	uint32_t maxfid, startvid, currentfid;
	const struct sysctlnode *freqnode, *node, *pnownode;
	struct powernow_cpu_state *cstate;
	struct cpu_info *ci;
	char *cpuname;
	const char *techname;
	size_t len;
	int i;

	ci = curcpu();

	freq_names_len = 0;
	cpuname = ci->ci_dev->dv_xname;

	cstate = malloc(sizeof(struct powernow_cpu_state), M_DEVBUF, M_NOWAIT);
	if (!cstate) {
		DPRINTF(("%s: cstate failed to malloc!\n", __func__));
		return;
	}

	k7pnow_flag = 0;
	if (ci->ci_signature == AMD_ERRATA_A0_CPUSIG)
		k7pnow_flag |= PN7_FLAG_ERRATA_A0;

	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	maxfid = PN7_STA_MFID(status);
	startvid = PN7_STA_SVID(status);
	currentfid = PN7_STA_CFID(status);

	cpu_mhz = ci->ci_tsc_freq / 1000000;
	cstate->fsb = cpu_mhz / (k7pnow_fid_to_mult[currentfid]/10);
	if (k7pnow_states(cstate, ci->ci_signature, maxfid, startvid)) {
		freq_names_len = cstate->n_states * (sizeof("9999 ")-1) + 1;
		freq_names = malloc(freq_names_len, M_SYSCTLDATA, M_WAITOK);
		freq_names[0] = '\0';
		len = 0;

		if (cstate->n_states) {
			for (i = 0; i < cstate->n_states; i++) {
				/* skip duplicated matches... */
				if (cstate->state_table[i].freq ==
				    cstate->state_table[i-1].freq)
					continue;

				DPRINTF(("%s: cstate->state_table.freq=%d\n",
				    __func__, cstate->state_table[i].freq));
				DPRINTF(("%s: fid=%d vid=%d\n", __func__,
				    cstate->state_table[i].fid,
				    cstate->state_table[i].vid));
				char tmp[6];
				len += snprintf(tmp, sizeof(tmp), "%d%s",
				    cstate->state_table[i].freq,
				    i < cstate->n_states - 1 ? " " : "");
				DPRINTF(("%s: tmp=%s\n", __func__, tmp));
				(void)strlcat(freq_names, tmp, freq_names_len);
			}
			k7pnow_current_state = cstate;
			DPRINTF(("%s: freq_names=%s\n", __func__, freq_names));
		}
	}

	if (k7pnow_current_state == NULL) {
		if (freq_names)
			free(freq_names, M_SYSCTLDATA);
		free(cstate, M_DEVBUF);
		return;
	}

	if (k7pnow_flag & PN7_FLAG_DESKTOP_VRM)
		techname = "Cool`n'Quiet K7";
	else
		techname = "Powernow! K7";

	/* Create sysctl machdep.powernow.frequency. */
	if (sysctl_createv(SYSCTLLOG, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL) != 0)
		goto err;

	if (sysctl_createv(SYSCTLLOG, 0, &node, &pnownode,
	    0,
	    CTLTYPE_NODE, "powernow", NULL,
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL) != 0)
		goto err;

	if (sysctl_createv(SYSCTLLOG, 0, &pnownode, &freqnode,
	    0,
	    CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL) != 0)
		goto err;

	if (sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "target", NULL,
	    k7pnow_sysctl_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL) != 0)
		goto err;

	powernow_node_target = node->sysctl_num;

	if (sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
	    0,
	    CTLTYPE_INT, "current", NULL,
	    k7pnow_sysctl_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL) != 0)
		goto err;

	powernow_node_current = node->sysctl_num;

	if ((sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
	    0,
	    CTLTYPE_STRING, "available", NULL,
	    NULL, 0, freq_names, freq_names_len,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	cur_freq = cstate->state_table[cstate->n_states-1].freq;

	aprint_normal("%s: AMD %s Technology %d (MHz)\n",
	    cpuname, techname, cur_freq);
	aprint_normal("%s: frequencies available (Mhz): %s\n",
	    cpuname, freq_names);

	return;

  err:
	free(freq_names, M_SYSCTLDATA);
	free(cstate, M_DEVBUF);
}

void
k7_powernow_destroy(void)
{
#ifdef _LKM
	sysctl_teardown(SYSCTLLOG);

	if (freq_names)
		free(freq_names, M_SYSCTLDATA);	
#endif
}
