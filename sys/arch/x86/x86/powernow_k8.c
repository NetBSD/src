/*	$NetBSD: powernow_k8.c,v 1.24.4.2 2010/10/24 22:48:19 jym Exp $ */
/*	$OpenBSD: powernow-k8.c,v 1.8 2006/06/16 05:58:50 gwk Exp $ */

/*-
 * Copyright (c) 2004, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines and Martin Vegiard.
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

/*
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

/* AMD POWERNOW K8 driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powernow_k8.c,v 1.24.4.2 2010/10/24 22:48:19 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/once.h>
#include <sys/xcall.h>

#include <x86/cpu_msr.h>
#include <x86/powernow.h>

#include <dev/isa/isareg.h>

#include <machine/isa_machdep.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/bus.h>

#ifdef _MODULE
static struct sysctllog *sysctllog;
#define SYSCTLLOG	&sysctllog
#else
#define SYSCTLLOG	NULL
#endif

static struct powernow_cpu_state *k8pnow_current_state;
static unsigned int cur_freq;
static int powernow_node_target, powernow_node_current;
static char *freq_names;
static size_t freq_names_len;

static int k8pnow_sysctl_helper(SYSCTLFN_PROTO);
static int k8pnow_decode_pst(struct powernow_cpu_state *, uint8_t *);
static int k8pnow_states(struct powernow_cpu_state *, uint32_t, unsigned int,
    unsigned int);
static int k8_powernow_setperf(unsigned int);
static int k8_powernow_init_once(void);
static void k8_powernow_init_main(void);

static uint64_t
k8pnow_wr_fidvid(u_int fid, uint64_t vid, uint64_t ctrl)
{
	struct msr_rw_info msr;
	uint64_t where, status;

	msr.msr_read = false;
	msr.msr_value = (ctrl << 32) | (1ULL << 16) | (vid << 8) | fid;
	msr.msr_type = MSR_AMDK7_FIDVID_CTL;

	where = xc_broadcast(0, (xcfunc_t)x86_msr_xcall, &msr, NULL);
	xc_wait(where);

	do {
		status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	} while (PN8_STA_PENDING(status));

	return status;
}

static int
k8pnow_sysctl_helper(SYSCTLFN_ARGS)
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
		if (k8_powernow_setperf(fq) == 0)
			cur_freq = fq;
		else
			return EINVAL;
	}

	return 0;
}

static int
k8_powernow_setperf(unsigned int freq)
{
	unsigned int i;
	uint64_t status;
	uint32_t val;
	int cfid, cvid, fid = 0, vid = 0;
	int rvo;
	struct powernow_cpu_state *cstate;

	/*
	 * We dont do a k8pnow_read_pending_wait here, need to ensure that the
	 * change pending bit isn't stuck,
	 */
	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	if (PN8_STA_PENDING(status))
		return 1;
	cfid = PN8_STA_CFID(status);
	cvid = PN8_STA_CVID(status);

	cstate = k8pnow_current_state;

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

	if (fid == cfid && vid == cvid)
		return 0;

	/*
	 * Phase 1: Raise core voltage to requested VID if frequency is
	 * going up.
	 */
	while (cvid > vid) {
		val = cvid - (1 << cstate->mvs);
		status = k8pnow_wr_fidvid(cfid, (val > 0) ? val : 0, 1ULL);
		cvid = PN8_STA_CVID(status);
		COUNT_OFF_VST(cstate->vst);
	}

	/* ... then raise to voltage + RVO (if required) */
	for (rvo = cstate->rvo; rvo > 0 && cvid > 0; --rvo) {
		/* XXX It's not clear from spec if we have to do that
		 * in 0.25 step or in MVS.  Therefore do it as it's done
		 * under Linux */
		status = k8pnow_wr_fidvid(cfid, cvid - 1, 1ULL);
		cvid = PN8_STA_CVID(status);
		COUNT_OFF_VST(cstate->vst);
	}

	/* Phase 2: change to requested core frequency */
	if (cfid != fid) {
		uint32_t vco_fid, vco_cfid;

		vco_fid = FID_TO_VCO_FID(fid);
		vco_cfid = FID_TO_VCO_FID(cfid);

		while (abs(vco_fid - vco_cfid) > 2) {
			if (fid > cfid) {
				if (cfid > 6)
					val = cfid + 2;
				else
					val = FID_TO_VCO_FID(cfid) + 2;
			} else
				val = cfid - 2;
			status = k8pnow_wr_fidvid(val, cvid,
			    (uint64_t)cstate->pll * 1000 / 5);
			cfid = PN8_STA_CFID(status);
			COUNT_OFF_IRT(cstate->irt);

			vco_cfid = FID_TO_VCO_FID(cfid);
		}

		status = k8pnow_wr_fidvid(fid, cvid,
		    (uint64_t)cstate->pll * 1000 / 5);
		cfid = PN8_STA_CFID(status);
		COUNT_OFF_IRT(cstate->irt);
	}

	/* Phase 3: change to requested voltage */
	if (cvid != vid) {
		status = k8pnow_wr_fidvid(cfid, vid, 1ULL);
		cvid = PN8_STA_CVID(status);
		COUNT_OFF_VST(cstate->vst);
	}

	if (cfid == fid || cvid == vid)
		freq = cstate->state_table[i].freq;

	return 0;
}

/*
 * Given a set of pair of fid/vid, and number of performance states,
 * compute state_table via an insertion sort.
 */
static int
k8pnow_decode_pst(struct powernow_cpu_state *cstate, uint8_t *p)
{
	int i, j, n;
	struct powernow_state state;

	for (n = 0, i = 0; i < cstate->n_states; i++) {
		state.fid = *p++;
		state.vid = *p++;
	
		/*
		 * The minimum supported frequency per the data sheet is 800MHz
		 * The maximum supported frequency is 5000MHz.
		 */
		state.freq = 800 + state.fid * 100;
		j = n;
		while (j > 0 && cstate->state_table[j - 1].freq > state.freq) {
			memcpy(&cstate->state_table[j],
			    &cstate->state_table[j - 1],
			    sizeof(struct powernow_state));
			--j;
		}
		memcpy(&cstate->state_table[j], &state,
		    sizeof(struct powernow_state));
		n++;
	}
	return 1;
}

static int
k8pnow_states(struct powernow_cpu_state *cstate, uint32_t cpusig,
    unsigned int fid, unsigned int vid)
{
	struct powernow_psb_s *psb;
	struct powernow_pst_s *pst;
	uint8_t *p;
	int i;

	DPRINTF(("%s: before the for loop\n", __func__));

	for (p = (uint8_t *)ISA_HOLE_VADDR(BIOS_START);
	    p < (uint8_t *)ISA_HOLE_VADDR(BIOS_START + BIOS_LEN); p += 16) {
		if (memcmp(p, "AMDK7PNOW!", 10) != 0)
			continue;

		DPRINTF(("%s: inside the for loop\n", __func__));
		psb = (struct powernow_psb_s *)p;
		if (psb->version != 0x14) {
			DPRINTF(("%s: psb->version != 0x14\n",
			    __func__));
			return 0;
		}

		cstate->vst = psb->ttime;
		cstate->rvo = PN8_PSB_TO_RVO(psb->reserved);
		cstate->irt = PN8_PSB_TO_IRT(psb->reserved);
		cstate->mvs = PN8_PSB_TO_MVS(psb->reserved);
		cstate->low = PN8_PSB_TO_BATT(psb->reserved);
		p+= sizeof(struct powernow_psb_s);

		for(i = 0; i < psb->n_pst; ++i) {
			pst = (struct powernow_pst_s *) p;

			cstate->pll = pst->pll;
			cstate->n_states = pst->n_states;
			if (cpusig == pst->signature &&
			    pst->fid == fid && pst->vid == vid) {
				DPRINTF(("%s: cpusig = signature\n",
				    __func__));
				return (k8pnow_decode_pst(cstate,
				    p+= sizeof(struct powernow_pst_s)));
			}
			p += sizeof(struct powernow_pst_s) +
			    2 * cstate->n_states;
		}
	}

	DPRINTF(("%s: returns 0!\n", __func__));
	return 0;

}

static int
k8_powernow_init_once(void)
{
	k8_powernow_init_main();
	return 0;
}

void
k8_powernow_init(void)
{
	int error;
	static ONCE_DECL(powernow_initialized);

	error = RUN_ONCE(&powernow_initialized, k8_powernow_init_once);
	if (__predict_false(error != 0)) {
		return;
	}
}

static void
k8_powernow_init_main(void)
{
	uint64_t status;
	uint32_t maxfid, maxvid, i;
	const struct sysctlnode *freqnode, *node, *pnownode;
	struct powernow_cpu_state *cstate;
	const char *cpuname, *techname;
	size_t len;

	freq_names_len = 0;
	cpuname = device_xname(curcpu()->ci_dev);

	k8pnow_current_state = NULL;

	cstate = malloc(sizeof(struct powernow_cpu_state), M_DEVBUF, M_NOWAIT);
	if (!cstate) {
		DPRINTF(("%s: cstate failed to malloc!\n", __func__));
		return;
	}

	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	maxfid = PN8_STA_MFID(status);
	maxvid = PN8_STA_MVID(status);

	/*
	* If start FID is different to max FID, then it is a
	* mobile processor.  If not, it is a low powered desktop
	* processor.
	*/

	if (PN8_STA_SFID(status) != PN8_STA_MFID(status))
		techname = "PowerNow!";
	else
		techname = "Cool`n'Quiet";

	if (k8pnow_states(cstate, curcpu()->ci_signature, maxfid, maxvid)) {
		freq_names_len = cstate->n_states * (sizeof("9999 ")-1) + 1;
		freq_names = malloc(freq_names_len, M_SYSCTLDATA, M_WAITOK);
		freq_names[0] = '\0';
		len = 0;

		if (cstate->n_states) {
			for (i = 0; i < cstate->n_states; i++) {
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
			k8pnow_current_state = cstate;
			DPRINTF(("%s: freq_names=%s\n", __func__, freq_names));
		}
	} else {
		DPRINTF(("%s: returned 0!\n", __func__));
	}

	if (k8pnow_current_state == NULL) {
		DPRINTF(("%s: k8pnow_current_state is NULL!\n", __func__));
		goto err;
	}

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
	    k8pnow_sysctl_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL) != 0)
		goto err;

	powernow_node_target = node->sysctl_num;

	if (sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
	    0,
	    CTLTYPE_INT, "current", NULL,
	    k8pnow_sysctl_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL) != 0)
		goto err;

	powernow_node_current = node->sysctl_num;

	if (sysctl_createv(SYSCTLLOG, 0, &freqnode, &node,
	    0,
	    CTLTYPE_STRING, "available", NULL,
	    NULL, 0, freq_names, freq_names_len,
	    CTL_CREATE, CTL_EOL) != 0)
		goto err;

	cur_freq = cstate->state_table[cstate->n_states-1].freq;

	aprint_normal("%s: AMD %s Technology %d MHz\n",
	    cpuname, techname, cur_freq);
	aprint_normal("%s: available frequencies (MHz): %s\n",
	    cpuname, freq_names);

	return;

  err:
	if (cstate)
		free(cstate, M_DEVBUF);
	if (freq_names)
		free(freq_names, M_SYSCTLDATA);
}

void
k8_powernow_destroy(void)
{
#ifdef _MODULE
	sysctl_teardown(SYSCTLLOG);

	if (k8pnow_current_state)
		free(k8pnow_current_state, M_DEVBUF);
	if (freq_names)
		free(freq_names, M_SYSCTLDATA);
#endif
}
