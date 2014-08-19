/*	$NetBSD: powernow.c,v 1.6.2.2 2014/08/20 00:03:29 tls Exp $ */
/*	$OpenBSD: powernow-k8.c,v 1.8 2006/06/16 05:58:50 gwk Exp $ */

/*-
 * Copyright (c) 2004, 2006, 2008 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powernow.c,v 1.6.2.2 2014/08/20 00:03:29 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <dev/isa/isareg.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/cpu_msr.h>
#include <x86/isa_machdep.h>
#include <x86/powernow.h>
#include <x86/specialreg.h>

#ifdef __i386__
/*
 * Divide each value by 10 to get the processor multiplier.
 * Taken from powernow-k7.c/Linux by Dave Jones.
 */
static int powernow_k7_fidtomult[32] = {
	110, 115, 120, 125, 50, 55, 60, 65,
	70, 75, 80, 85, 90, 95, 100, 105,
	30, 190, 40, 200, 130, 135, 140, 210,
	150, 225, 160, 165, 170, 180, -1, -1
};

/*
 * The following CPUs do not have same CPUID signature
 * in the PST tables, but they have correct FID/VID values.
 */
static struct powernow_k7_quirk {
	uint32_t rcpusig;	/* Correct cpu signature */
	uint32_t pcpusig;	/* PST cpu signature */
} powernow_k7_quirk[] = {
	{ 0x680, 0x780 }, 	/* Reported by Olaf 'Rhialto' Seibert */
	{ 0x6a0, 0x781 },	/* Reported by Eric Schnoebelen */
	{ 0x6a0, 0x7a0 },	/* Reported by Nino Dehne */
	{ 0, 0 }
};

#endif	/* __i386__ */

struct powernow_softc {
	device_t		   sc_dev;
	struct cpu_info		  *sc_ci;
	struct sysctllog	  *sc_log;
	struct powernow_cpu_state *sc_state;
	const char		  *sc_tech;
	char			  *sc_freqs;
	size_t			   sc_freqs_len;
	unsigned int		   sc_freqs_cur;
	int			   sc_node_target;
	int			   sc_node_current;
	int			   sc_flags;
};

static int	powernow_match(device_t, cfdata_t, void *);
static void	powernow_attach(device_t, device_t, void *);
static int	powernow_detach(device_t, int);
static int	powernow_sysctl(device_t);
static int	powernow_sysctl_helper(SYSCTLFN_PROTO);

#ifdef __i386__
static int	powernow_k7_init(device_t);
static int	powernow_k7_states(device_t, unsigned int, unsigned int);
static int	powernow_k7_setperf(device_t, unsigned int);
static bool	powernow_k7_check(uint32_t, uint32_t);
static int	powernow_k7_decode_pst(struct powernow_softc *, uint8_t *,int);
#endif

static int	powernow_k8_init(device_t);
static int	powernow_k8_states(device_t, unsigned int, unsigned int);
static int	powernow_k8_setperf(device_t, unsigned int);
static uint64_t powernow_k8_fidvid(u_int fid, uint64_t vid, uint64_t ctrl);
static int	powernow_k8_decode_pst(struct powernow_cpu_state *, uint8_t *);

CFATTACH_DECL_NEW(powernow, sizeof(struct powernow_softc),
    powernow_match, powernow_attach, powernow_detach, NULL);

static int
powernow_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;
	uint32_t family, regs[4];

	if (strcmp(cfaa->name, "frequency") != 0)
		return 0;

	if (cpu_vendor != CPUVENDOR_AMD)
		return 0;

	family = CPUID_TO_BASEFAMILY(ci->ci_signature);

	if (family != 0x06 && family != 0x0f)
		return 0;
	else {
		x86_cpuid(0x80000000, regs);

		if (regs[0] < 0x80000007)
			return 0;

		x86_cpuid(0x80000007, regs);

		if ((regs[3] & AMD_PN_FID_VID) == AMD_PN_FID_VID)
			return 5;
	}

	return 0;
}

static void
powernow_attach(device_t parent, device_t self, void *aux)
{
	struct powernow_softc *sc = device_private(self);
	struct cpufeature_attach_args *cfaa = aux;
	struct cpu_info *ci = cfaa->ci;
	uint32_t family;
	int rv;

	sc->sc_ci = ci;
	sc->sc_dev = self;

	sc->sc_flags = 0;
	sc->sc_log = NULL;
	sc->sc_tech = NULL;
	sc->sc_state = NULL;
	sc->sc_freqs = NULL;

	family = CPUID_TO_BASEFAMILY(ci->ci_signature);

	switch (family) {

#ifdef __i386__
	case 0x06:
		rv = powernow_k7_init(self);
		break;
#endif
	case 0x0f:
		rv = powernow_k8_init(self);
		break;

	default:
		rv = ENODEV;
	}

	if (rv != 0) {
		aprint_normal(": failed to initialize (err %d)\n", rv);
		return;
	}

	rv = powernow_sysctl(self);

	if (rv != 0) {
		aprint_normal(": failed to initialize sysctl (err %d)\n", rv);
		return;
	}

	KASSERT(sc->sc_tech != NULL);

	aprint_naive("\n");
	aprint_normal(": AMD %s\n", sc->sc_tech);

	(void)pmf_device_register(self, NULL, NULL);
}

static int
powernow_detach(device_t self, int flags)
{
	struct powernow_softc *sc = device_private(self);

	if (sc->sc_log != NULL)
		sysctl_teardown(&sc->sc_log);

	if (sc->sc_state != NULL)
		kmem_free(sc->sc_state, sizeof(*sc->sc_state));

	if (sc->sc_freqs != NULL)
		kmem_free(sc->sc_freqs, sc->sc_freqs_len);

	pmf_device_deregister(self);

	return 0;
}

static int
powernow_sysctl(device_t self)
{
	const struct sysctlnode *freqnode, *node, *pnownode;
	struct powernow_softc *sc = device_private(self);
	int rv;

	/*
	 * Setup the sysctl sub-tree machdep.powernow.
	 */
	rv = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &node, &pnownode,
	    0, CTLTYPE_NODE, "powernow", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &pnownode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    powernow_sysctl_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	sc->sc_node_target = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_INT, "current", NULL,
	    powernow_sysctl_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	sc->sc_node_current = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, sc->sc_freqs, sc->sc_freqs_len, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	return 0;

fail:
	sysctl_teardown(&sc->sc_log);
	sc->sc_log = NULL;

	return rv;
}

static int
powernow_sysctl_helper(SYSCTLFN_ARGS)
{
	struct powernow_softc *sc;
	struct sysctlnode node;
	int fq, oldfq, error;
	uint32_t family;
	int rv;

	fq = oldfq = 0;

	node = *rnode;
	sc = node.sysctl_data;

	if (sc->sc_state == NULL)
		return EOPNOTSUPP;

	node.sysctl_data = &fq;

	if (rnode->sysctl_num == sc->sc_node_target)
		fq = oldfq = sc->sc_freqs_cur;
	else if (rnode->sysctl_num == sc->sc_node_current)
		fq = sc->sc_freqs_cur;
	else
		return EOPNOTSUPP;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	family = CPUID_TO_BASEFAMILY(sc->sc_ci->ci_signature);

	if (rnode->sysctl_num == sc->sc_node_target && fq != oldfq) {

		switch (family) {

#ifdef __i386__
		case 0x06:
			rv = powernow_k7_setperf(sc->sc_dev, fq);
			break;
#endif
		case 0x0f:
			rv = powernow_k8_setperf(sc->sc_dev, fq);
			break;

		default:
			return ENODEV;
		}

		if (rv != 0)
			return EINVAL;

		sc->sc_freqs_cur = fq;
	}

	return 0;
}

#ifdef __i386__

static int
powernow_k7_init(device_t self)
{
	struct powernow_softc *sc = device_private(self);
	uint32_t currentfid, maxfid, mhz, startvid;
	uint64_t status;
	int i, rv;
	char tmp[6];

	sc->sc_state = kmem_alloc(sizeof(*sc->sc_state), KM_SLEEP);

	if (sc->sc_state == NULL)
		return ENOMEM;

	if (sc->sc_ci->ci_signature == AMD_ERRATA_A0_CPUSIG)
		sc->sc_flags |= PN7_FLAG_ERRATA_A0;

	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	maxfid = PN7_STA_MFID(status);
	startvid = PN7_STA_SVID(status);
	currentfid = PN7_STA_CFID(status);

	mhz = sc->sc_ci->ci_data.cpu_cc_freq / 1000000;
	sc->sc_state->fsb = mhz / (powernow_k7_fidtomult[currentfid] / 10);

	if (powernow_k7_states(self, maxfid, startvid) == 0) {
		rv = ENXIO;
		goto fail;
	}

	if (sc->sc_state->n_states == 0) {
		rv = ENODEV;
		goto fail;
	}

	sc->sc_freqs_len = sc->sc_state->n_states * (sizeof("9999 ") - 1) + 1;
	sc->sc_freqs = kmem_zalloc(sc->sc_freqs_len, KM_SLEEP);

	if (sc->sc_freqs == NULL) {
		rv = ENOMEM;
		goto fail;
	}

	for (i = 0; i < sc->sc_state->n_states; i++) {

		/* Skip duplicated matches. */
		if (i > 0 && (sc->sc_state->state_table[i].freq ==
		    sc->sc_state->state_table[i - 1].freq))
			continue;

		DPRINTF(("%s: cstate->state_table.freq=%d\n",
			__func__, sc->sc_state->state_table[i].freq));
		DPRINTF(("%s: fid=%d vid=%d\n", __func__,
			sc->sc_state->state_table[i].fid,
			sc->sc_state->state_table[i].vid));

		snprintf(tmp, sizeof(tmp), "%d%s",
		    sc->sc_state->state_table[i].freq,
		    i < sc->sc_state->n_states - 1 ? " " : "");

		DPRINTF(("%s: tmp=%s\n", __func__, tmp));

		(void)strlcat(sc->sc_freqs, tmp, sc->sc_freqs_len);
	}

	sc->sc_tech = ((sc->sc_flags & PN7_FLAG_DESKTOP_VRM) != 0) ?
	    "Cool`n'Quiet K7" : "Powernow! K7";

	return 0;

fail:
	kmem_free(sc->sc_state, sizeof(*sc->sc_state));
	sc->sc_state = NULL;

	return rv;
}

static int
powernow_k7_states(device_t self, unsigned int fid, unsigned int vid)
{
	struct powernow_softc *sc = device_private(self);
	struct powernow_cpu_state *cstate;
	struct powernow_psb_s *psb;
	struct powernow_pst_s *pst;
	uint32_t cpusig, maxpst;
	bool cpusig_ok;
	uint8_t *p;

	cstate = sc->sc_state;
	cpusig = sc->sc_ci->ci_signature;

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
				sc->sc_flags |= PN7_FLAG_DESKTOP_VRM;

			p += sizeof(struct powernow_psb_s);

			for (maxpst = 0; maxpst < psb->n_pst; maxpst++) {
				pst = (struct powernow_pst_s*) p;

				/*
				 * Some models do not have same CPUID
				 * signature in the PST table. Accept to
				 * report frequencies via a quirk table
				 * and fid/vid match.
				 */
				DPRINTF(("%s: cpusig=0x%x pst->sig:0x%x\n",
				    __func__, cpusig, pst->signature));

				cpusig_ok = powernow_k7_check(cpusig,
				                              pst->signature);

				if ((cpusig_ok != false &&
				    (fid == pst->fid && vid == pst->vid))) {
					DPRINTF(("%s: pst->signature ok\n",
					    __func__));
					if (abs(cstate->fsb - pst->pll) > 5)
						continue;
					cstate->n_states = pst->n_states;
					return (powernow_k7_decode_pst(sc,
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

static int
powernow_k7_setperf(device_t self, unsigned int freq)
{
	struct powernow_softc *sc = device_private(self);
	int cvid, cfid, vid = 0, fid = 0;
	uint64_t status, ctl;
	unsigned int i;

	DPRINTF(("%s: cstate->n_states=%d\n",
		__func__, sc->sc_state->n_states));

	for (i = 0; i < sc->sc_state->n_states; i++) {

		if (sc->sc_state->state_table[i].freq >= freq) {
			DPRINTF(("%s: freq=%d\n", __func__, freq));
			fid = sc->sc_state->state_table[i].fid;
			vid = sc->sc_state->state_table[i].vid;
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
	ctl |= PN7_CTR_SGTC(sc->sc_state->sgtc);

	if ((sc->sc_flags & PN7_FLAG_ERRATA_A0) != 0)
		x86_disable_intr();

	if (powernow_k7_fidtomult[fid] < powernow_k7_fidtomult[cfid]) {

		wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_FIDC);

		if (vid != cvid)
			wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_VIDC);

	} else {

		wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_VIDC);

		if (fid != cfid)
			wrmsr(MSR_AMDK7_FIDVID_CTL, ctl | PN7_CTR_FIDC);
	}

	if ((sc->sc_flags & PN7_FLAG_ERRATA_A0) != 0)
		x86_enable_intr();

	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);

	cfid = PN7_STA_CFID(status);
	cvid = PN7_STA_CVID(status);

	if (cfid == fid || cvid == vid)
		freq = sc->sc_state->state_table[i].freq;

	return 0;
}

static bool
powernow_k7_check(uint32_t real_cpusig, uint32_t pst_cpusig)
{
	int j;

	if (real_cpusig == pst_cpusig)
		return true;

	for (j = 0; powernow_k7_quirk[j].rcpusig != 0; j++) {

		if ((real_cpusig == powernow_k7_quirk[j].rcpusig) &&
		    (pst_cpusig == powernow_k7_quirk[j].pcpusig))
			return true;
	}

	return false;
}

static int
powernow_k7_decode_pst(struct powernow_softc *sc, uint8_t *p, int npst)
{
	struct powernow_cpu_state *cstate = sc->sc_state;
	struct powernow_state state;
	int i, j, n;

	for (n = 0, i = 0; i < npst; ++i) {

		state.fid = *p++;
		state.vid = *p++;
		state.freq = powernow_k7_fidtomult[state.fid]/10 * cstate->fsb;

		if ((sc->sc_flags & PN7_FLAG_ERRATA_A0) != 0 &&
		    (powernow_k7_fidtomult[state.fid] % 10) == 5)
			continue;

		j = n;

		while (j > 0 && cstate->state_table[j - 1].freq > state.freq) {

			(void)memcpy(&cstate->state_table[j],
			    &cstate->state_table[j - 1],
			    sizeof(struct powernow_state));

			--j;
		}

		(void)memcpy(&cstate->state_table[j], &state,
		    sizeof(struct powernow_state));

		++n;
	}

	/*
	 * Fix powernow_max_states, if errata_a0 gave
	 * us less states than expected.
	 */
	cstate->n_states = n;

	return 1;
}

#endif	/* __i386__ */

static int
powernow_k8_init(device_t self)
{
	struct powernow_softc *sc = device_private(self);
	uint32_t i, maxfid, maxvid;
	uint64_t status;
	int rv;
	char tmp[6];

	sc->sc_state = kmem_alloc(sizeof(*sc->sc_state), KM_SLEEP);

	if (sc->sc_state == NULL)
		return ENOMEM;

	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);
	maxfid = PN8_STA_MFID(status);
	maxvid = PN8_STA_MVID(status);

	if (powernow_k8_states(self, maxfid, maxvid) == 0) {
		rv = ENXIO;
		goto fail;
	}

	if (sc->sc_state->n_states == 0) {
		rv = ENODEV;
		goto fail;
	}

	sc->sc_freqs_len = sc->sc_state->n_states * (sizeof("9999 ") - 1) + 1;
	sc->sc_freqs = kmem_zalloc(sc->sc_freqs_len, KM_SLEEP);

	if (sc->sc_freqs == NULL) {
		rv = ENOMEM;
		goto fail;
	}

	for (i = 0; i < sc->sc_state->n_states; i++) {

		DPRINTF(("%s: cstate->state_table.freq=%d\n",
			__func__, sc->sc_state->state_table[i].freq));

		DPRINTF(("%s: fid=%d vid=%d\n", __func__,
			sc->sc_state->state_table[i].fid,
			sc->sc_state->state_table[i].vid));

		snprintf(tmp, sizeof(tmp), "%d%s",
		    sc->sc_state->state_table[i].freq,
		    i < sc->sc_state->n_states - 1 ? " " : "");

		DPRINTF(("%s: tmp=%s\n", __func__, tmp));

		(void)strlcat(sc->sc_freqs, tmp, sc->sc_freqs_len);
	}

	/*
	 * If start FID is different to max FID, then it is a mobile
	 * processor. If not, it is a low powered desktop processor.
	 */
	sc->sc_tech = (PN8_STA_SFID(status) != PN8_STA_MFID(status)) ?
	    "PowerNow!" : "Cool`n'Quiet";

	return 0;

fail:
	kmem_free(sc->sc_state, sizeof(*sc->sc_state));
	sc->sc_state = NULL;

	return rv;
}

static int
powernow_k8_states(device_t self, unsigned int fid, unsigned int vid)
{
	struct powernow_softc *sc = device_private(self);
	struct powernow_cpu_state *cstate;
	struct powernow_psb_s *psb;
	struct powernow_pst_s *pst;
	uint32_t cpusig;
	uint8_t *p;
	int i;

	cstate = sc->sc_state;
	cpusig = sc->sc_ci->ci_signature;

	DPRINTF(("%s: before the for loop\n", __func__));

	for (p = (uint8_t *)ISA_HOLE_VADDR(BIOS_START);
	     p < (uint8_t *)ISA_HOLE_VADDR(BIOS_START + BIOS_LEN); p += 16) {

		if (memcmp(p, "AMDK7PNOW!", 10) != 0)
			continue;

		DPRINTF(("%s: inside the for loop\n", __func__));

		psb = (struct powernow_psb_s *)p;

		if (psb->version != 0x14) {
			DPRINTF(("%s: psb->version != 0x14\n", __func__));
			return 0;
		}

		cstate->vst = psb->ttime;
		cstate->rvo = PN8_PSB_TO_RVO(psb->reserved);
		cstate->irt = PN8_PSB_TO_IRT(psb->reserved);
		cstate->mvs = PN8_PSB_TO_MVS(psb->reserved);
		cstate->low = PN8_PSB_TO_BATT(psb->reserved);

		p += sizeof(struct powernow_psb_s);

		for(i = 0; i < psb->n_pst; ++i) {

			pst = (struct powernow_pst_s *)p;

			cstate->pll = pst->pll;
			cstate->n_states = pst->n_states;

			if (cpusig == pst->signature &&
			    pst->fid == fid && pst->vid == vid) {

				DPRINTF(("%s: cpusig = sig\n", __func__));

				return (powernow_k8_decode_pst(cstate,
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
powernow_k8_setperf(device_t self, unsigned int freq)
{
	struct powernow_softc *sc = device_private(self);
	int cfid, cvid, fid = 0, vid = 0;
	uint64_t status;
	unsigned int i;
	uint32_t val;
	int rvo;

	/*
	 * We dont do a "pending wait" here, given
	 * that we need to ensure that the change
	 * pending bit is not stuck.
	 */
	status = rdmsr(MSR_AMDK7_FIDVID_STATUS);

	if (PN8_STA_PENDING(status))
		return 1;

	cfid = PN8_STA_CFID(status);
	cvid = PN8_STA_CVID(status);

	DPRINTF(("%s: sc->sc_state->n_states=%d\n",
		__func__, sc->sc_state->n_states));

	for (i = 0; i < sc->sc_state->n_states; i++) {

		if (sc->sc_state->state_table[i].freq >= freq) {
			DPRINTF(("%s: freq=%d\n", __func__, freq));
			fid = sc->sc_state->state_table[i].fid;
			vid = sc->sc_state->state_table[i].vid;
			DPRINTF(("%s: fid=%d vid=%d\n", __func__, fid, vid));
			break;
		}

	}

	if (fid == cfid && vid == cvid)
		return 0;

	/*
	 * Phase 1: raise the core voltage to the
	 * requested VID if frequency is going up.
	 */
	while (cvid > vid) {
		val = cvid - (1 << sc->sc_state->mvs);
		status = powernow_k8_fidvid(cfid, (val > 0) ? val : 0, 1ULL);
		cvid = PN8_STA_CVID(status);
		COUNT_OFF_VST(sc->sc_state->vst);
	}

	/*
	 * Then raise to voltage + RVO (if required).
	 */
	for (rvo = sc->sc_state->rvo; rvo > 0 && cvid > 0; --rvo) {
		/*
		 * XXX:	It is not clear from the specification if we have
		 *	to do that in 0.25 step or in MVS. Therefore do it
		 *	as it is done under Linux.
		 */
		status = powernow_k8_fidvid(cfid, cvid - 1, 1ULL);
		cvid = PN8_STA_CVID(status);
		COUNT_OFF_VST(sc->sc_state->vst);
	}

	/*
	 * Phase 2: change to requested core frequency.
	 */
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

			status = powernow_k8_fidvid(val, cvid,
			    (uint64_t)sc->sc_state->pll * 1000 / 5);

			cfid = PN8_STA_CFID(status);
			COUNT_OFF_IRT(sc->sc_state->irt);
			vco_cfid = FID_TO_VCO_FID(cfid);
		}


		status = powernow_k8_fidvid(fid, cvid,
		    (uint64_t)sc->sc_state->pll * 1000 / 5);

		cfid = PN8_STA_CFID(status);
		COUNT_OFF_IRT(sc->sc_state->irt);
	}

	/*
	 * Phase 3: change to requested voltage.
	 */
	if (cvid != vid) {
		status = powernow_k8_fidvid(cfid, vid, 1ULL);
		cvid = PN8_STA_CVID(status);
		COUNT_OFF_VST(sc->sc_state->vst);
	}

	if (cfid == fid || cvid == vid)
		freq = sc->sc_state->state_table[i].freq;

	return 0;
}

static uint64_t
powernow_k8_fidvid(u_int fid, uint64_t vid, uint64_t ctrl)
{
	struct msr_rw_info msr;
	uint64_t status, xc;

	msr.msr_read = false;
	msr.msr_value = (ctrl << 32) | (1ULL << 16) | (vid << 8) | fid;
	msr.msr_type = MSR_AMDK7_FIDVID_CTL;

	xc = xc_broadcast(0, (xcfunc_t)x86_msr_xcall, &msr, NULL);
	xc_wait(xc);

	do {
		status = rdmsr(MSR_AMDK7_FIDVID_STATUS);

	} while (PN8_STA_PENDING(status));

	return status;
}

/*
 * Given a set of pair of FID/VID, and a number of performance
 * states, compute a state table via an insertion sort.
 */
static int
powernow_k8_decode_pst(struct powernow_cpu_state *cstate, uint8_t *p)
{
	struct powernow_state state;
	int i, j, n;

	for (n = 0, i = 0; i < cstate->n_states; i++) {

		state.fid = *p++;
		state.vid = *p++;

		/*
		 * The minimum supported frequency per the data sheet is
		 * 800 MHz. The maximum supported frequency is 5000 MHz.
		 */
		state.freq = 800 + state.fid * 100;
		j = n;

		while (j > 0 && cstate->state_table[j - 1].freq > state.freq) {

			(void)memcpy(&cstate->state_table[j],
			    &cstate->state_table[j - 1],
			    sizeof(struct powernow_state));

			--j;
		}

		(void)memcpy(&cstate->state_table[j], &state,
		    sizeof(struct powernow_state));

		n++;
	}

	return 1;
}

MODULE(MODULE_CLASS_DRIVER, powernow, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
powernow_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_powernow,
		    cfattach_ioconf_powernow, cfdata_ioconf_powernow);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_powernow,
		    cfattach_ioconf_powernow, cfdata_ioconf_powernow);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
