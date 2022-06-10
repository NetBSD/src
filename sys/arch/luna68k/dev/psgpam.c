/*	$NetBSD: psgpam.c,v 1.1 2022/06/10 21:42:23 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Yosuke Sugahara. All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: psgpam.c,v 1.1 2022/06/10 21:42:23 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>

#include <sys/cpu.h>
#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <machine/autoconf.h>

#include <luna68k/dev/xpbusvar.h>
#include <luna68k/dev/psgpam_enc.h>
#include <luna68k/dev/xpcmd.h>
#include <luna68k/dev/xplx/xplxdefs.h>

#include <luna68k/luna68k/isr.h>

#include "ioconf.h"

/*
 * Debug level:
 * 0: No debug logs
 * 1: action changes like open/close/set_format...
 * 2: + normal operations like read/write/ioctl...
 * 3: + TRACEs except interrupt
 * 4: + TRACEs including interrupt
 */
/* Note AUDIO_DEBUG should be sync'ed with src/sys/dev/audio/audio.c */
/* #define AUDIO_DEBUG	1 */

#if defined(AUDIO_DEBUG)
#define DPRINTF(n, fmt...)	do {					\
	if (psgpamdebug >= (n)) {					\
		if (cpu_intr_p()) {					\
			audio_mlog_printf(fmt);				\
		} else {						\
			audio_mlog_flush();				\
			printf(fmt);					\
		}							\
	}								\
} while (0)

/* XXX Parasitic to audio.c... */
extern void audio_mlog_flush(void);
extern void audio_mlog_printf(const char *, ...);

static int	psgpamdebug = AUDIO_DEBUG;
#else
#define DPRINTF(n, fmt...)	__nothing
#endif

struct psgpam_softc {
	device_t sc_dev;
	vaddr_t sc_shm_base;
	vsize_t sc_shm_size;

	void (*sc_intr)(void *);
	void *sc_arg;

	kmutex_t sc_intr_lock;
	kmutex_t sc_thread_lock;

	int      sc_isopen;

	int      sc_started;
	int      sc_outcount;
	int      sc_xp_state;
	uint16_t sc_xp_addr;	/* XP buffer addr */

	int      sc_xp_enc;
	int      sc_xp_rept;
	int      sc_xp_cycle_clk;
	int      sc_xp_rept_clk;
	int      sc_xp_rept_max;

	u_int    sc_sample_rate;
	int      sc_stride;
	int      sc_dynamic;

	uint8_t *sc_start_ptr;
	uint8_t *sc_end_ptr;
	int      sc_blksize;
	int      sc_blkcount;
	int      sc_cur_blk_id;

	struct psgpam_codecvar sc_psgpam_codecvar;
};

static int  psgpam_match(device_t, cfdata_t, void *);
static void psgpam_attach(device_t, device_t, void *);

/* MI audio layer interface */
static int  psgpam_open(void *, int);
static void psgpam_close(void *);
static int  psgpam_query_format(void *, audio_format_query_t *);
static int  psgpam_set_format(void *, int,
    const audio_params_t *, const audio_params_t *,
    audio_filter_reg_t *, audio_filter_reg_t *);
static int  psgpam_trigger_output(void *, void *, void *, int,
    void (*)(void *), void *, const audio_params_t *);
static int  psgpam_halt_output(void *);
static int  psgpam_getdev(void *, struct audio_device *);
static int  psgpam_set_port(void *, mixer_ctrl_t *);
static int  psgpam_get_port(void *, mixer_ctrl_t *);
static int  psgpam_query_devinfo(void *, mixer_devinfo_t *);
static int  psgpam_get_props(void *);
static void psgpam_get_locks(void *, kmutex_t **, kmutex_t **);
static int  psgpam_round_blocksize(void *, int, int, const audio_params_t *);
static size_t psgpam_round_buffersize(void *, int, size_t);

static int  psgpam_intr(void *);

#if defined(AUDIO_DEBUG)
static int  psgpam_sysctl_debug(SYSCTLFN_PROTO);
#endif
static int  psgpam_sysctl_enc(SYSCTLFN_PROTO);
static int  psgpam_sysctl_dynamic(SYSCTLFN_PROTO);

CFATTACH_DECL_NEW(psgpam, sizeof(struct psgpam_softc),
    psgpam_match, psgpam_attach, NULL, NULL);

static int psgpam_matched;

static const struct audio_hw_if psgpam_hw_if = {
	.open			= psgpam_open,
	.close			= psgpam_close,
	.query_format		= psgpam_query_format,
	.set_format		= psgpam_set_format,
	.trigger_output		= psgpam_trigger_output,
	.halt_output		= psgpam_halt_output,
	.getdev			= psgpam_getdev,
	.set_port		= psgpam_set_port,
	.get_port		= psgpam_get_port,
	.query_devinfo		= psgpam_query_devinfo,
	.get_props		= psgpam_get_props,
	.get_locks		= psgpam_get_locks,
	.round_blocksize        = psgpam_round_blocksize,
	.round_buffersize	= psgpam_round_buffersize,
};

static struct audio_device psgpam_device = {
	"PSG PAM",
	"0.2",
	"",
};

static struct audio_format psgpam_format = {
	.mode		= AUMODE_PLAY,
	.encoding	= AUDIO_ENCODING_NONE,
	.validbits	= 0,		/* filled by query_format */
	.precision	= 0,		/* filled by query_format */
	.channels	= 1,
	.channel_mask	= AUFMT_MONAURAL,
	.frequency_type	= 0,		/* filled by query_format */
	.frequency	= { 0 },	/* filled by query_format */
};

static int
psgpam_match(device_t parent, cfdata_t cf, void *aux)
{
	struct xpbus_attach_args *xa = aux;

	if (psgpam_matched)
		return 0;

	if (strcmp(xa->xa_name, psgpam_cd.cd_name))
		return 0;

	psgpam_matched = 1;
	return 1;
}

static void
psgpam_attach(device_t parent, device_t self, void *aux)
{
	struct psgpam_softc *sc;
	const struct sysctlnode *node;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_normal(": HD647180X I/O processor as PSG PAM\n");

	sc->sc_shm_base = XP_SHM_BASE;
	sc->sc_shm_size = XP_SHM_SIZE;

	sc->sc_xp_enc = PAM_ENC_PAM2A;
	sc->sc_sample_rate = 8000;
	sc->sc_stride = 2;
	sc->sc_dynamic = 1;

	mutex_init(&sc->sc_thread_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	isrlink_autovec(psgpam_intr, sc, 5, ISRPRI_TTYNOBUF);

	sysctl_createv(NULL, 0, NULL, &node,
		0,
		CTLTYPE_NODE, device_xname(sc->sc_dev),
		SYSCTL_DESCR("psgpam"),
		NULL, 0,
		NULL, 0,
		CTL_HW,
		CTL_CREATE, CTL_EOL);
	if (node != NULL) {
#if defined(AUDIO_DEBUG)
		sysctl_createv(NULL, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "debug",
			SYSCTL_DESCR("PSGPAM debug"),
			psgpam_sysctl_debug, 0, (void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);
#endif
		sysctl_createv(NULL, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "enc",
			SYSCTL_DESCR("PSGPAM encoding"),
			psgpam_sysctl_enc, 0, (void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);
		sysctl_createv(NULL, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "dynamic",
			SYSCTL_DESCR("PSGPAM dynamic offset"),
			psgpam_sysctl_dynamic, 0, (void *)sc, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);
	}

	audio_attach_mi(&psgpam_hw_if, sc, sc->sc_dev);
}

/* private functions */

static void
psgpam_xp_query(struct psgpam_softc *sc)
{
	u_int a;
	int r;

	if (!sc->sc_isopen) {
		a = xp_acquire(DEVID_PAM, 0);
		if (a == 0) {
			sc->sc_xp_cycle_clk = 65535;
			sc->sc_xp_rept_clk = 255;
			sc->sc_xp_rept_max = 0;
			DPRINTF(1, "XPLX BUSY!\n");
			return;
		}
		xp_ensure_firmware();
	}

	xp_writemem8(PAM_ENC, sc->sc_xp_enc);
	r = xp_cmd(DEVID_PAM, PAM_CMD_QUERY);
	if (r != XPLX_R_OK) {
		sc->sc_xp_cycle_clk = 65535;
		sc->sc_xp_rept_clk = 255;
		sc->sc_xp_rept_max = 0;
		DPRINTF(1, "XPLX QUERY FAIL: %d\n", r);
	} else {
		sc->sc_xp_cycle_clk = xp_readmem16le(PAM_CYCLE_CLK);
		sc->sc_xp_rept_clk = xp_readmem8(PAM_REPT_CLK);
		sc->sc_xp_rept_max = xp_readmem8(PAM_REPT_MAX);
		DPRINTF(1, "xp cycle_clk=%d rept_clk=%d rept_max=%d\n",
		    sc->sc_xp_cycle_clk,
		    sc->sc_xp_rept_clk,
		    sc->sc_xp_rept_max);
	}
	if (!sc->sc_isopen) {
		xp_release(DEVID_PAM);
	}
}

static void
psgpam_xp_start(struct psgpam_softc *sc)
{

	DPRINTF(3, "XP PAM starting..");
	if (xp_readmem8(PAM_RUN) != 0) {
		DPRINTF(1, "XP PAM already started???\n");
	}

	psgpam_xp_query(sc);

	sc->sc_xp_rept = (XP_CPU_FREQ / sc->sc_sample_rate
	    - sc->sc_xp_cycle_clk) / sc->sc_xp_rept_clk;
	if (sc->sc_xp_rept < 0)
		sc->sc_xp_rept = 0;
	if (sc->sc_xp_rept > sc->sc_xp_rept_max)
		sc->sc_xp_rept = sc->sc_xp_rept_max;
	xp_writemem8(PAM_REPT, sc->sc_xp_rept);
	DPRINTF(3, "ENC=%d REPT=%d\n", sc->sc_xp_enc, sc->sc_xp_rept);

	xp_intr5_enable();
	xp_cmd_nowait(DEVID_PAM, PAM_CMD_START);

	DPRINTF(3, "XP PAM started\n");
}

/* MI MD API */

static int
psgpam_open(void *hdl, int flags)
{
	struct psgpam_softc *sc;
	u_int a;

	DPRINTF(1, "%s: flags=0x%x\n", __func__, flags);
	sc = hdl;

	a = xp_acquire(DEVID_PAM, 0);
	if (a == 0)
		return EBUSY;

	/* firmware transfer */
	xp_ensure_firmware();

	sc->sc_xp_state = 0;
	sc->sc_started = 0;
	sc->sc_outcount = 0;
	sc->sc_isopen = 1;

	memset(xp_shmptr(PAM_BUF), XP_ATN_RESET, PAM_BUF_LEN);

	return 0;
}

static void
psgpam_close(void *hdl)
{
	struct psgpam_softc *sc;

	sc = hdl;

	xp_intr5_disable();

	xp_release(DEVID_PAM);

	sc->sc_isopen = 0;

	DPRINTF(1, "%s\n", __func__);
}

static int
psgpam_query_format(void *hdl, audio_format_query_t *afp)
{
	struct psgpam_softc *sc;
	u_int freq;
	uint8_t rept_max;
	int clk;
	int i, n;

#define XP_FREQ_MAXCOUNT	40
	int f[XP_FREQ_MAXCOUNT];

	if (afp->index != 0)
		return EINVAL;

	sc = hdl;

	psgpam_xp_query(sc);
	switch (sc->sc_xp_enc) {
	case PAM_ENC_PAM2A:
	case PAM_ENC_PAM2B:
		psgpam_format.validbits = 16;
		psgpam_format.precision = 16;
		break;
	case PAM_ENC_PAM3A:
	case PAM_ENC_PAM3B:
		psgpam_format.validbits = 32;
		psgpam_format.precision = 32;
		break;
	}

	/* convert xp's max to AUFMT's max */
	rept_max = sc->sc_xp_rept_max + 1;

	if (rept_max <= AUFMT_MAX_FREQUENCIES) {
		/* all choice */
		for (i = 0; i < rept_max; i++) {
			clk = sc->sc_xp_cycle_clk + i * sc->sc_xp_rept_clk;
			freq = XP_CPU_FREQ / clk;
			psgpam_format.frequency[i] = freq;
		}
		n = rept_max;
	} else {
		if (rept_max > XP_FREQ_MAXCOUNT)
			rept_max = XP_FREQ_MAXCOUNT;

		for (i = 0; i < rept_max; i++) {
			clk = sc->sc_xp_cycle_clk + i * sc->sc_xp_rept_clk;
			freq = XP_CPU_FREQ / clk;
			if (freq < 4000) break;
			f[i] = freq;
		}
		for (; i < XP_FREQ_MAXCOUNT; i++)
			f[i] = 0;

		/*
		 * keep: first, last
		 * remove: any unusable freq
		 */
		for (i = 1; i < rept_max - 1; i++) {
			if (( 4000 <= f[i] && f[i] <  6000 &&
			     f[i - 1] <  6000 && f[i + 1] >  4000) ||
			    ( 6000 <= f[i]    && f[i] < 8000 &&
			     f[i - 1] <  8000 && f[i + 1] >  6000) ||
			    ( 8000 <= f[i] && f[i] < 12000 &&
			     f[i - 1] < 12000 && f[i + 1] >  8000) ||
			    (12000 <= f[i] && f[i] < 16000 &&
			     f[i - 1] < 16000 && f[i + 1] > 12000)) {
				f[i] = 0;
			}
		}
		n = 0;
		for (i = 0; i < rept_max; i++) {
			if (f[i] != 0) {
				psgpam_format.frequency[n] = f[i];
				n++;
				if (n == AUFMT_MAX_FREQUENCIES)
					break;
			}
		}
	}

	psgpam_format.frequency_type = n;

	afp->fmt = psgpam_format;
	return 0;
}

static int
psgpam_set_format(void *hdl, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	/* called before open */

	struct psgpam_softc *sc;

	sc = hdl;
	DPRINTF(1, "%s: mode=%d %s/%dbit/%dch/%dHz\n", __func__,
	    setmode, audio_encoding_name(play->encoding),
	    play->precision, play->channels, play->sample_rate);

	sc->sc_sample_rate = play->sample_rate;

	/* set filter */
	switch (sc->sc_xp_enc) {
	case PAM_ENC_PAM2A:
		if (sc->sc_dynamic) {
			pfil->codec = psgpam_aint_to_pam2a_d;
		} else {
			pfil->codec = psgpam_aint_to_pam2a;
		}
		sc->sc_stride = 2;
		break;
	case PAM_ENC_PAM2B:
		if (sc->sc_dynamic) {
			pfil->codec = psgpam_aint_to_pam2b_d;
		} else {
			pfil->codec = psgpam_aint_to_pam2b;
		}
		sc->sc_stride = 2;
		break;
	case PAM_ENC_PAM3A:
		if (sc->sc_dynamic) {
			pfil->codec = psgpam_aint_to_pam3a_d;
		} else {
			pfil->codec = psgpam_aint_to_pam3a;
		}
		sc->sc_stride = 4;
		break;
	case PAM_ENC_PAM3B:
		if (sc->sc_dynamic) {
			pfil->codec = psgpam_aint_to_pam3b_d;
		} else {
			pfil->codec = psgpam_aint_to_pam3b;
		}
		sc->sc_stride = 4;
		break;
	}
	psgpam_init_context(&sc->sc_psgpam_codecvar, sc->sc_sample_rate);
	pfil->context = &sc->sc_psgpam_codecvar;

	return 0;
}

/* marking block */
static void
psgpam_mark_blk(struct psgpam_softc *sc, int blk_id)
{
	int markoffset;
	uint marker;

	markoffset = sc->sc_blksize * (blk_id + 1);

	if (blk_id == sc->sc_blkcount - 1) {
		marker = XP_ATN_RELOAD;
	} else {
		marker = XP_ATN_STAT;
	}

	/* marking */
	uint8_t *start = sc->sc_start_ptr;
	if (sc->sc_stride == 2) {
		uint16_t *markptr = (uint16_t*)(start + markoffset);
		markptr -= 1;
		*markptr |= marker;
	} else {
		/* stride == 4 */
		uint32_t *markptr = (uint32_t*)(start + markoffset);
		markptr -= 1;
		*markptr |= marker;
	}
}

static int
psgpam_trigger_output(void *hdl, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *param)
{
	void *dp;
	struct psgpam_softc *sc;

	sc = hdl;

	DPRINTF(2, "%s start=%p end=%p blksize=%d\n", __func__,
		start, end, blksize);

	sc->sc_outcount++;

	sc->sc_intr = intr;
	sc->sc_arg = intrarg;
	sc->sc_blksize = blksize;

	sc->sc_start_ptr = start;
	sc->sc_end_ptr = end;
	sc->sc_blkcount = (sc->sc_end_ptr - sc->sc_start_ptr) / sc->sc_blksize;
	sc->sc_xp_addr = PAM_BUF;

	psgpam_mark_blk(sc, 0);
	psgpam_mark_blk(sc, 1);

	/* transfer */
	dp = xp_shmptr(sc->sc_xp_addr);
	memcpy(dp, start, blksize * 2);

	/* (preincrement variable in intr) */
	sc->sc_cur_blk_id = 1;
	sc->sc_xp_addr += blksize;

	/* play start */
	if (sc->sc_started == 0) {
		/* set flag first */
		sc->sc_started = 1;
		psgpam_xp_start(sc);
	}

	return 0;
}

static int
psgpam_halt_output(void *hdl)
{
	struct psgpam_softc *sc = hdl;

	DPRINTF(2, "%s\n", __func__);

	xp_intr5_disable();

	memset(xp_shmptr(PAM_BUF), XP_ATN_RESET, PAM_BUF_LEN);

	sc->sc_started = 0;
	sc->sc_xp_state = 0;

	return 0;
}

static int
psgpam_getdev(void *hdl, struct audio_device *ret)
{

	*ret = psgpam_device;
	return 0;
}

static int
psgpam_set_port(void *hdl, mixer_ctrl_t *mc)
{

	DPRINTF(2, "%s\n", __func__);
	return 0;
}

static int
psgpam_get_port(void *hdl, mixer_ctrl_t *mc)
{

	DPRINTF(2, "%s\n", __func__);
	return 0;
}

static int
psgpam_query_devinfo(void *hdl, mixer_devinfo_t *di)
{

	DPRINTF(2, "%s %d\n", __func__, di->index);
	switch (di->index) {
	default:
		return EINVAL;
	}
	return 0;
}

static int
psgpam_get_props(void *hdl)
{

	return AUDIO_PROP_PLAYBACK;
}

static void
psgpam_get_locks(void *hdl, kmutex_t **intr, kmutex_t **thread)
{
	struct psgpam_softc *sc = hdl;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_thread_lock;
}

static int
psgpam_round_blocksize(void *hdl, int bs, int mode,
    const audio_params_t *param)
{

#if 0
	if (bs < 16384) {
		return (16384 / bs) * bs;
	} else {
		return 16384;
	}
#else
	return bs;
#endif
}

static size_t
psgpam_round_buffersize(void *hdl, int direction, size_t bufsize)
{

	if (bufsize > 28 * 1024) {
		bufsize = 28 * 1024;
	}
	return bufsize;
}

static int
psgpam_intr(void *hdl)
{
	struct psgpam_softc *sc = hdl;

	xp_intr5_acknowledge();
	DPRINTF(4, "psgpam intr\n");

	sc->sc_cur_blk_id++;
	sc->sc_xp_addr += sc->sc_blksize;
	if (sc->sc_cur_blk_id == sc->sc_blkcount) {
		sc->sc_cur_blk_id = 0;
		sc->sc_xp_addr = PAM_BUF;
	}
	psgpam_mark_blk(sc, sc->sc_cur_blk_id);
	memcpy(xp_shmptr(sc->sc_xp_addr),
	    sc->sc_start_ptr + sc->sc_cur_blk_id * sc->sc_blksize,
	    sc->sc_blksize);

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_intr) {
		sc->sc_intr(sc->sc_arg);
	} else {
		DPRINTF(1, "psgpam_intr: spurious interrupt\n");
	}

	mutex_spin_exit(&sc->sc_intr_lock);

	/* handled */
	return 1;
}

#if defined(AUDIO_DEBUG)
/* sysctl */
static int
psgpam_sysctl_debug(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int t, error;

	node = *rnode;

	t = psgpamdebug;
	node.sysctl_data = &t;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		return error;
	}

	if (t < 0)
		return EINVAL;
	if (t > 4)
		return EINVAL;
	psgpamdebug = t;
	return 0;
}
#endif

/* sysctl */
static int
psgpam_sysctl_enc(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct psgpam_softc *sc;
	int t, error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_xp_enc;
	node.sysctl_data = &t;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		return error;
	}

	if (t < PAM_ENC_PAM2A)
		return EINVAL;
	if (t > PAM_ENC_PAM3B)
		return EINVAL;
	sc->sc_xp_enc = t;
	return 0;
}

static int
psgpam_sysctl_dynamic(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct psgpam_softc *sc;
	int t, error;

	node = *rnode;
	sc = node.sysctl_data;

	t = sc->sc_dynamic;
	node.sysctl_data = &t;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		return error;
	}

	sc->sc_dynamic = t;
	return 0;
}
