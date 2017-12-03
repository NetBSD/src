/*	$NetBSD: bth5.c,v 1.5.2.2 2017/12/03 11:36:59 jdolecek Exp $	*/
/*
 * Copyright (c) 2017 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
 *
 * Copyright (c) 2007 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bth5.c,v 1.5.2.2 2017/12/03 11:36:59 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/syslimits.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <netbt/bluetooth.h>
#include <netbt/hci.h>

#include <dev/bluetooth/bth5.h>

#include "ioconf.h"

#ifdef BTH5_DEBUG
#ifdef DPRINTF
#undef DPRINTF
#endif
#ifdef DPRINTFN
#undef DPRINTFN
#endif

#define DPRINTF(x)	printf x
#define DPRINTFN(n, x)	do { if (bth5_debug > (n)) printf x; } while (0)
int bth5_debug = 3;
#else
#undef DPRINTF
#undef DPRINTFN

#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

struct bth5_softc {
	device_t sc_dev;

	struct tty *sc_tp;
	struct hci_unit *sc_unit;		/* Bluetooth HCI Unit */
	struct bt_stats sc_stats;

	int sc_flags;

	/* output queues */
	MBUFQ_HEAD()	sc_cmdq;
	MBUFQ_HEAD()	sc_aclq;
	MBUFQ_HEAD()	sc_scoq;

	int sc_baud;
	int sc_init_baud;

	/* variables of SLIP Layer */
	struct mbuf *sc_txp;			/* outgoing packet */
	struct mbuf *sc_rxp;			/* incoming packet */
	int sc_slip_txrsv;			/* reserved byte data */
	int sc_slip_rxexp;			/* expected byte data */
	void (*sc_transmit_callback)(struct bth5_softc *, struct mbuf *);

	/* variables of Packet Integrity Layer */
	int sc_pi_txcrc;			/* use CRC, if true */

	/* variables of MUX Layer */
	bool sc_mux_send_ack;			/* flag for send_ack */
	bool sc_mux_choke;			/* Choke signal */
	struct timeval sc_mux_lastrx;		/* Last Rx Pkt Time */

	/* variables of Sequencing Layer */
	MBUFQ_HEAD() sc_seqq;			/* Sequencing Layer queue */
	MBUFQ_HEAD() sc_seq_retryq;		/* retry queue */
	uint32_t sc_seq_txseq;
	uint32_t sc_seq_expected_rxseq;
	uint32_t sc_seq_total_rxpkts;
	uint32_t sc_seq_winack;
	uint32_t sc_seq_winspace;
	uint32_t sc_seq_retries;
	callout_t sc_seq_timer;
	uint32_t sc_seq_timeout;
	uint32_t sc_seq_winsize;
	uint32_t sc_seq_retry_limit;
	bool	 sc_oof_flow_control;

	/* variables of Datagram Queue Layer */
	MBUFQ_HEAD() sc_dgq;			/* Datagram Queue Layer queue */

	/* variables of BTH5 Link Establishment Protocol */
	bool sc_le_muzzled;
	bth5_le_state_t sc_le_state;
	callout_t sc_le_timer;

	struct sysctllog *sc_log;		/* sysctl log */
};

/* sc_flags */
#define	BTH5_XMIT	(1 << 0)	/* transmit active */
#define	BTH5_ENABLED	(1 << 1)	/* is enabled */

static int bthfive_match(device_t, cfdata_t, void *);
static void bthfive_attach(device_t, device_t, void *);
static int bthfive_detach(device_t, int);

/* tty functions */
static int bth5open(dev_t, struct tty *);
static int bth5close(struct tty *, int);
static int bth5ioctl(struct tty *, u_long, void *, int, struct lwp *);

static int bth5_slip_transmit(struct tty *);
static int bth5_slip_receive(int, struct tty *);

static void bth5_pktintegrity_transmit(struct bth5_softc *);
static void bth5_pktintegrity_receive(struct bth5_softc *, struct mbuf *);
static void bth5_crc_update(uint16_t *, uint8_t);
static uint16_t bth5_crc_reverse(uint16_t);

static void bth5_mux_transmit(struct bth5_softc *sc);
static void bth5_mux_receive(struct bth5_softc *sc, struct mbuf *m);
static __inline void bth5_send_ack_command(struct bth5_softc *sc);
static __inline struct mbuf *bth5_create_ackpkt(void);
static __inline void bth5_set_choke(struct bth5_softc *, bool);

static void bth5_sequencing_receive(struct bth5_softc *, struct mbuf *);
static bool bth5_tx_reliable_pkt(struct bth5_softc *, struct mbuf *, u_int);
static __inline u_int bth5_get_txack(struct bth5_softc *);
static void bth5_signal_rxack(struct bth5_softc *, uint32_t);
static void bth5_reliabletx_callback(struct bth5_softc *, struct mbuf *);
static void bth5_timer_timeout(void *);
static void bth5_sequencing_reset(struct bth5_softc *);

static void bth5_datagramq_receive(struct bth5_softc *, struct mbuf *);
static bool bth5_tx_unreliable_pkt(struct bth5_softc *, struct mbuf *, u_int);
static void bth5_unreliabletx_callback(struct bth5_softc *, struct mbuf *);

static int bth5_start_le(struct bth5_softc *);
static void bth5_terminate_le(struct bth5_softc *);
static void bth5_input_le(struct bth5_softc *, struct mbuf *);
static void bth5_le_timeout(void *);

static void bth5_start(struct bth5_softc *);

/* bluetooth hci functions */
static int bth5_enable(device_t);
static void bth5_disable(device_t);
static void bth5_output_cmd(device_t, struct mbuf *);
static void bth5_output_acl(device_t, struct mbuf *);
static void bth5_output_sco(device_t, struct mbuf *);
static void bth5_stats(device_t, struct bt_stats *, int);

#ifdef BTH5_DEBUG
static void bth5_packet_print(struct mbuf *m);
#endif


/*
 * It doesn't need to be exported, as only bth5attach() uses it,
 * but there's no "official" way to make it static.
 */
CFATTACH_DECL_NEW(bthfive, sizeof(struct bth5_softc),
    bthfive_match, bthfive_attach, bthfive_detach, NULL);

static struct linesw bth5_disc = {
	.l_name = "bth5",
	.l_open = bth5open,
	.l_close = bth5close,
	.l_read = ttyerrio,
	.l_write = ttyerrio,
	.l_ioctl = bth5ioctl,
	.l_rint = bth5_slip_receive,
	.l_start = bth5_slip_transmit,
	.l_modem = ttymodem,
	.l_poll = ttyerrpoll
};

static const struct hci_if bth5_hci = {
	.enable = bth5_enable,
	.disable = bth5_disable,
	.output_cmd = bth5_output_cmd,
	.output_acl = bth5_output_acl,
	.output_sco = bth5_output_sco,
	.get_stats = bth5_stats,
	.ipl = IPL_TTY,
};

/* ARGSUSED */
void
bthfiveattach(int num __unused)
{
	int error;

	error = ttyldisc_attach(&bth5_disc);
	if (error) {
		aprint_error("%s: unable to register line discipline, "
		    "error = %d\n", bthfive_cd.cd_name, error);
		return;
	}

	error = config_cfattach_attach(bthfive_cd.cd_name, &bthfive_ca);
	if (error) {
		aprint_error("%s: unable to register cfattach, error = %d\n",
		    bthfive_cd.cd_name, error);
		config_cfdriver_detach(&bthfive_cd);
		(void) ttyldisc_detach(&bth5_disc);
	}
}

/*
 * Autoconf match routine.
 *
 * XXX: unused: config_attach_pseudo(9) does not call ca_match.
 */
/* ARGSUSED */
static int
bthfive_match(device_t self __unused, cfdata_t cfdata __unused,
	   void *arg __unused)
{

	/* pseudo-device; always present */
	return 1;
}

/*
 * Autoconf attach routine.  Called by config_attach_pseudo(9) when we
 * open the line discipline.
 */
/* ARGSUSED */
static void
bthfive_attach(device_t parent __unused, device_t self, void *aux __unused)
{
	struct bth5_softc *sc = device_private(self);
	const struct sysctlnode *node;
	int rc, bth5_node_num;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_dev = self;
	callout_init(&sc->sc_seq_timer, 0);
	callout_setfunc(&sc->sc_seq_timer, bth5_timer_timeout, sc);
	callout_init(&sc->sc_le_timer, 0);
	callout_setfunc(&sc->sc_le_timer, bth5_le_timeout, sc);
	sc->sc_seq_timeout = BTH5_SEQ_TX_TIMEOUT;
	sc->sc_seq_winsize = BTH5_SEQ_TX_WINSIZE;
	sc->sc_seq_retry_limit = BTH5_SEQ_TX_RETRY_LIMIT;
	MBUFQ_INIT(&sc->sc_seqq);
	MBUFQ_INIT(&sc->sc_seq_retryq);
	MBUFQ_INIT(&sc->sc_dgq);
	MBUFQ_INIT(&sc->sc_cmdq);
	MBUFQ_INIT(&sc->sc_aclq);
	MBUFQ_INIT(&sc->sc_scoq);

	/* Attach Bluetooth unit */
	sc->sc_unit = hci_attach_pcb(&bth5_hci, self, 0);

	if ((rc = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    0, CTLTYPE_NODE, device_xname(self),
	    SYSCTL_DESCR("bth5 controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}
	bth5_node_num = node->sysctl_num;
	if ((rc = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL,
	    "muzzled", SYSCTL_DESCR("muzzled for Link-establishment Layer"),
	    NULL, 0, &sc->sc_le_muzzled,
	    0, CTL_HW, bth5_node_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}
	if ((rc = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "txcrc", SYSCTL_DESCR("txcrc for Packet Integrity Layer"),
	    NULL, 0, &sc->sc_pi_txcrc,
	    0, CTL_HW, bth5_node_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}
	if ((rc = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "timeout", SYSCTL_DESCR("timeout for Sequencing Layer"),
	    NULL, 0, &sc->sc_seq_timeout,
	    0, CTL_HW, bth5_node_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}
	if ((rc = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "winsize", SYSCTL_DESCR("winsize for Sequencing Layer"),
	    NULL, 0, &sc->sc_seq_winsize,
	    0, CTL_HW, bth5_node_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}
	if ((rc = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "retry_limit", SYSCTL_DESCR("retry limit for Sequencing Layer"),
	    NULL, 0, &sc->sc_seq_retry_limit,
	    0, CTL_HW, bth5_node_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}
	return;

err:
	aprint_error_dev(self, "sysctl_createv failed (rc = %d)\n", rc);
}

/*
 * Autoconf detach routine.  Called when we close the line discipline.
 */
/* ARGSUSED */
static int
bthfive_detach(device_t self, int flags __unused)
{
	struct bth5_softc *sc = device_private(self);

	if (sc->sc_unit != NULL) {
		hci_detach_pcb(sc->sc_unit);
		sc->sc_unit = NULL;
	}

	callout_halt(&sc->sc_seq_timer, NULL);
	callout_destroy(&sc->sc_seq_timer);

	callout_halt(&sc->sc_le_timer, NULL);
	callout_destroy(&sc->sc_le_timer);

	return 0;
}


/*
 * Line discipline functions.
 */
/* ARGSUSED */
static int
bth5open(dev_t device __unused, struct tty *tp)
{
	struct bth5_softc *sc;
	device_t dev;
	cfdata_t cfdata;
	struct lwp *l = curlwp;		/* XXX */
	int error, unit, s;
	static char name[] = "bthfive";

	error = kauth_authorize_device(l->l_cred, KAUTH_DEVICE_BLUETOOTH_BCSP,
	    KAUTH_ARG(KAUTH_REQ_DEVICE_BLUETOOTH_BCSP_ADD), NULL, NULL, NULL);
	if (error)
		return (error);

	s = spltty();

	if (tp->t_linesw == &bth5_disc) {
		sc = tp->t_sc;
		if (sc != NULL) {
			splx(s);
			return EBUSY;
		}
	}

	KASSERT(tp->t_oproc != NULL);

	cfdata = malloc(sizeof(struct cfdata), M_DEVBUF, M_WAITOK);
	for (unit = 0; unit < bthfive_cd.cd_ndevs; unit++)
		if (device_lookup(&bthfive_cd, unit) == NULL)
			break;
	cfdata->cf_name = name;
	cfdata->cf_atname = name;
	cfdata->cf_unit = unit;
	cfdata->cf_fstate = FSTATE_STAR;

	aprint_normal("%s%d at tty major %llu minor %llu",
	    name, unit, (unsigned long long)major(tp->t_dev),
	    (unsigned long long)minor(tp->t_dev));
	dev = config_attach_pseudo(cfdata);
	if (dev == NULL) {
		splx(s);
		return EIO;
	}
	sc = device_private(dev);

	mutex_spin_enter(&tty_lock);
	tp->t_sc = sc;
	sc->sc_tp = tp;
	ttyflush(tp, FREAD | FWRITE);
	mutex_spin_exit(&tty_lock);

	splx(s);

	sc->sc_slip_txrsv = BTH5_SLIP_PKTSTART;
	bth5_sequencing_reset(sc);

	/* start link-establishment */
	bth5_start_le(sc);

	return 0;
}

/* ARGSUSED */
static int
bth5close(struct tty *tp, int flag __unused)
{
	struct bth5_softc *sc = tp->t_sc;
	cfdata_t cfdata;
	int s;

	/* terminate link-establishment */
	bth5_terminate_le(sc);

	s = spltty();

	MBUFQ_DRAIN(&sc->sc_dgq);
	bth5_sequencing_reset(sc);

	mutex_spin_enter(&tty_lock);
	ttyflush(tp, FREAD | FWRITE);
	mutex_spin_exit(&tty_lock);	/* XXX */
	ttyldisc_release(tp->t_linesw);
	tp->t_linesw = ttyldisc_default();
	if (sc != NULL) {
		tp->t_sc = NULL;
		if (sc->sc_tp == tp) {
			cfdata = device_cfdata(sc->sc_dev);
			config_detach(sc->sc_dev, 0);
			free(cfdata, M_DEVBUF);
		}

	}
	splx(s);
	return 0;
}

/* ARGSUSED */
static int
bth5ioctl(struct tty *tp, u_long cmd, void *data, int flag __unused,
	  struct lwp *l __unused)
{
	struct bth5_softc *sc = tp->t_sc;
	int error;

	if (sc == NULL || tp != sc->sc_tp)
		return EPASSTHROUGH;

	error = 0;
	switch (cmd) {
	default:
		error = EPASSTHROUGH;
		break;
	}

	return error;
}


/*
 * UART Driver Layer is supported by com-driver.
 */

/*
 * BTH5 SLIP Layer functions:
 *   Supports to transmit/receive a byte stream.
 *   SLIP protocol described in Internet standard RFC 1055.
 */
static int
bth5_slip_transmit(struct tty *tp)
{
	struct bth5_softc *sc = tp->t_sc;
	struct mbuf *m;
	int count, rlen;
	uint8_t *rptr;
	int s;

	m = sc->sc_txp;
	if (m == NULL) {
		s = spltty();
		sc->sc_flags &= ~BTH5_XMIT;
		splx(s);
		bth5_mux_transmit(sc);
		return 0;
	}

	count = 0;
	rlen = 0;
	rptr = mtod(m, uint8_t *);

	if (sc->sc_slip_txrsv != 0) {
#ifdef BTH5_DEBUG
		if (sc->sc_slip_txrsv == BTH5_SLIP_PKTSTART)
			DPRINTFN(4, ("%s: slip transmit start\n",
			    device_xname(sc->sc_dev)));
		else
			DPRINTFN(4, ("0x%02x ", sc->sc_slip_txrsv));
#endif

		if (putc(sc->sc_slip_txrsv, &tp->t_outq) < 0)
			return 0;
		count++;

		if (sc->sc_slip_txrsv == BTH5_SLIP_ESCAPE_PKTEND ||
		    sc->sc_slip_txrsv == BTH5_SLIP_ESCAPE_ESCAPE) {
			rlen++;
			rptr++;
		}
		if (sc->sc_oof_flow_control == true) {
			if (sc->sc_slip_txrsv == BTH5_SLIP_ESCAPE_XON ||
			    sc->sc_slip_txrsv == BTH5_SLIP_ESCAPE_XOFF) {
				rlen++;
				rptr++;
			}
		}

		sc->sc_slip_txrsv = 0;
	}

	for(;;) {
		if (rlen >= m->m_len) {
			m = m->m_next;
			if (m == NULL) {
				if (putc(BTH5_SLIP_PKTEND, &tp->t_outq) < 0)
					break;

				DPRINTFN(4, ("\n%s: slip transmit end\n",
				    device_xname(sc->sc_dev)));

				m = sc->sc_txp;
				sc->sc_txp = NULL;
				sc->sc_slip_txrsv = BTH5_SLIP_PKTSTART;

				sc->sc_transmit_callback(sc, m);
				m = NULL;
				break;
			}

			rlen = 0;
			rptr = mtod(m, uint8_t *);
			continue;
		}

		if (*rptr == BTH5_SLIP_PKTEND) {
			if (putc(BTH5_SLIP_ESCAPE, &tp->t_outq) < 0)
				break;
			count++;
			DPRINTFN(4, (" esc "));

			if (putc(BTH5_SLIP_ESCAPE_PKTEND, &tp->t_outq) < 0) {
				sc->sc_slip_txrsv = BTH5_SLIP_ESCAPE_PKTEND;
				break;
			}
			DPRINTFN(4, ("0x%02x ", BTH5_SLIP_ESCAPE_PKTEND));
			rptr++;
		} else if (sc->sc_oof_flow_control == true && *rptr ==
							 BTH5_SLIP_XON) {
			if (putc(BTH5_SLIP_ESCAPE, &tp->t_outq) < 0)
				break;
			count++;
			DPRINTFN(4, (" esc "));

			if (putc(BTH5_SLIP_ESCAPE_XON, &tp->t_outq) < 0) {
				sc->sc_slip_txrsv = BTH5_SLIP_ESCAPE_XON;
				break;
			}
			DPRINTFN(4, ("0x%02x ", BTH5_SLIP_ESCAPE_XON));
			rptr++;
		} else if (sc->sc_oof_flow_control == true && *rptr ==
							 BTH5_SLIP_XOFF) {
			if (putc(BTH5_SLIP_ESCAPE, &tp->t_outq) < 0)
				break;
			count++;
			DPRINTFN(4, (" esc "));

			if (putc(BTH5_SLIP_ESCAPE_XOFF, &tp->t_outq) < 0) {
				sc->sc_slip_txrsv = BTH5_SLIP_ESCAPE_XOFF;
				break;
			}
			DPRINTFN(4, ("0x%02x ", BTH5_SLIP_ESCAPE_XOFF));
			rptr++;
		} else if (*rptr == BTH5_SLIP_ESCAPE) {
			if (putc(BTH5_SLIP_ESCAPE, &tp->t_outq) < 0)
				break;
			count++;
			DPRINTFN(4, (" esc "));

			if (putc(BTH5_SLIP_ESCAPE_ESCAPE, &tp->t_outq) < 0) {
				sc->sc_slip_txrsv = BTH5_SLIP_ESCAPE_ESCAPE;
				break;
			}
			DPRINTFN(4, ("0x%02x ", BTH5_SLIP_ESCAPE_ESCAPE));
			rptr++;
		} else {
			if (putc(*rptr++, &tp->t_outq) < 0)
				break;
			DPRINTFN(4, ("0x%02x ", *(rptr - 1)));
		}
		rlen++;
		count++;
	}
	if (m != NULL)
		m_adj(m, rlen);

	sc->sc_stats.byte_tx += count;

	if (tp->t_outq.c_cc != 0)
		(*tp->t_oproc)(tp);

	return 0;
}

static int
bth5_slip_receive(int c, struct tty *tp)
{
	struct bth5_softc *sc = tp->t_sc;
	struct mbuf *m = sc->sc_rxp;
	int discard = 0;
	const char *errstr;

	c &= TTY_CHARMASK;

	/* If we already started a packet, find the trailing end of it. */
	if (m) {
		while (m->m_next)
			m = m->m_next;

		if (M_TRAILINGSPACE(m) == 0) {
			/* extend mbuf */
			MGET(m->m_next, M_DONTWAIT, MT_DATA);
			if (m->m_next == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "out of memory\n");
				sc->sc_stats.err_rx++;
				return 0;	/* (lost sync) */
			}

			m = m->m_next;
			m->m_len = 0;
		}
	} else
		if (c != BTH5_SLIP_PKTSTART) {
			discard = 1;
			errstr = "not sync";
			goto discarded;
		}

	switch (c) {
	case BTH5_SLIP_PKTSTART /* or _PKTEND */:
		if (m == NULL) {
			/* BTH5_SLIP_PKTSTART */

			DPRINTFN(4, ("%s: slip receive start\n",
			    device_xname(sc->sc_dev)));

			/* new packet */
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "out of memory\n");
				sc->sc_stats.err_rx++;
				return 0;	/* (lost sync) */
			}

			sc->sc_rxp = m;
			m->m_pkthdr.len = m->m_len = 0;
			sc->sc_slip_rxexp = 0;
		} else {
			/* BTH5_SLIP_PKTEND */

			if (m == sc->sc_rxp && m->m_len == 0) {
				DPRINTFN(4, ("%s: resynchronises\n",
				    device_xname(sc->sc_dev)));

				sc->sc_stats.byte_rx++;
				return 0;
			}

			DPRINTFN(4, ("%s%s: slip receive end\n",
			    (m->m_len % 16 != 0) ? "\n" :  "",
			    device_xname(sc->sc_dev)));

			bth5_pktintegrity_receive(sc, sc->sc_rxp);
			sc->sc_rxp = NULL;
			sc->sc_slip_rxexp = BTH5_SLIP_PKTSTART;
		}
		sc->sc_stats.byte_rx++;
		return 0;

	case BTH5_SLIP_ESCAPE:

		DPRINTFN(4, ("  esc"));

		if (sc->sc_slip_rxexp == BTH5_SLIP_ESCAPE) {
			discard = 1;
			errstr = "waiting 0xdc or 0xdb or 0xde of 0xdf"; 
		} else
			sc->sc_slip_rxexp = BTH5_SLIP_ESCAPE;
		break;

	default:
		DPRINTFN(4, (" 0x%02x%s",
		    c, (m->m_len % 16 == 15) ? "\n" :  ""));

		switch (sc->sc_slip_rxexp) {
		case BTH5_SLIP_PKTSTART:
			discard = 1;
			errstr = "waiting 0xc0";
			break;

		case BTH5_SLIP_ESCAPE:
			if (c == BTH5_SLIP_ESCAPE_PKTEND)
				mtod(m, uint8_t *)[m->m_len++] =
				    BTH5_SLIP_PKTEND;
			else if (sc->sc_oof_flow_control == true &&
						c == BTH5_SLIP_ESCAPE_XON)
				mtod(m, uint8_t *)[m->m_len++] =
				    BTH5_SLIP_XON;
			else if (sc->sc_oof_flow_control == true &&
						c == BTH5_SLIP_ESCAPE_XOFF)
				mtod(m, uint8_t *)[m->m_len++] =
				    BTH5_SLIP_XOFF;
			else if (c == BTH5_SLIP_ESCAPE_ESCAPE)
				mtod(m, uint8_t *)[m->m_len++] =
				    BTH5_SLIP_ESCAPE;
			else {
				discard = 1;
				errstr = "unknown escape";
			}
			sc->sc_slip_rxexp = 0;
			break;

		default:
			mtod(m, uint8_t *)[m->m_len++] = c;
		}
		sc->sc_rxp->m_pkthdr.len++;
	}
	if (discard) {
discarded:
#ifdef BTH5_DEBUG
		DPRINTFN(4, ("%s: receives unexpected byte 0x%02x: %s\n",
		    device_xname(sc->sc_dev), c, errstr));
#else
		__USE(errstr);
#endif
	}
	sc->sc_stats.byte_rx++;

	return 0;
}


/*
 * BTH5 Packet Integrity Layer functions:
 *   handling Payload Length, Checksum, CRC.
 */
static void
bth5_pktintegrity_transmit(struct bth5_softc *sc)
{
	struct mbuf *m = sc->sc_txp;
	bth5_hdr_t *hdrp = mtod(m, bth5_hdr_t *);
	int pldlen;

	DPRINTFN(3, ("%s: pi transmit\n", device_xname(sc->sc_dev)));

	pldlen = m->m_pkthdr.len - sizeof(bth5_hdr_t);

	if (sc->sc_pi_txcrc)
		hdrp->flags |= BTH5_FLAGS_CRC_PRESENT;

	BTH5_SET_PLEN(hdrp, pldlen);
	BTH5_SET_CSUM(hdrp);

	if (sc->sc_pi_txcrc) {
		struct mbuf *_m;
		int n = 0;
		uint16_t crc = 0xffff;
		uint8_t *buf;

		for (_m = m; _m != NULL; _m = _m->m_next) {
			buf = mtod(_m, uint8_t *);
			for (n = 0; n < _m->m_len; n++)
				bth5_crc_update(&crc, *(buf + n));
		}
		crc = htobe16(bth5_crc_reverse(crc));
		m_copyback(m, m->m_pkthdr.len, sizeof(crc), &crc);
	}

#ifdef BTH5_DEBUG
	if (bth5_debug == 3)
		bth5_packet_print(m);
#endif

	bth5_slip_transmit(sc->sc_tp);
}

static void
bth5_pktintegrity_receive(struct bth5_softc *sc, struct mbuf *m)
{
	bth5_hdr_t *hdrp = mtod(m, bth5_hdr_t *);
	u_int pldlen;
	int discard = 0;
	uint16_t crc = 0xffff;
	const char *errstr;

	DPRINTFN(3, ("%s: pi receive\n", device_xname(sc->sc_dev)));
#ifdef BTH5_DEBUG
	if (bth5_debug == 4)
		bth5_packet_print(m);
#endif

	KASSERT(m->m_len >= sizeof(bth5_hdr_t));

	pldlen = m->m_pkthdr.len - sizeof(bth5_hdr_t) -
	    ((hdrp->flags & BTH5_FLAGS_CRC_PRESENT) ? sizeof(crc) : 0);
	if (pldlen > 0xfff) {
		discard = 1;
		errstr = "Payload Length";
		goto discarded;
	}
	if (hdrp->csum != BTH5_GET_CSUM(hdrp)) {
		discard = 1;
		errstr = "Checksum";
		goto discarded;
	}
	if (BTH5_GET_PLEN(hdrp) != pldlen) {
		discard = 1;
		errstr = "Payload Length";
		goto discarded;
	}
	if (hdrp->flags & BTH5_FLAGS_CRC_PRESENT) {
		struct mbuf *_m;
		int i, n;
		uint16_t crc0;
		uint8_t *buf;

		i = 0;
		n = 0;
		for (_m = m; _m != NULL; _m = _m->m_next) {
			buf = mtod(m, uint8_t *);
			for (n = 0;
			    n < _m->m_len && i < sizeof(bth5_hdr_t) + pldlen;
			    n++, i++)
				bth5_crc_update(&crc, *(buf + n));
		}

		m_copydata(_m, n, sizeof(crc0), &crc0);
		if (be16toh(crc0) != bth5_crc_reverse(crc)) {
			discard = 1;
			errstr = "CRC";
		} else
			/* Shaves CRC */
			m_adj(m, (int)(0 - sizeof(crc)));
	}

	if (discard) {
discarded:
#ifdef BTH5_DEBUG
		DPRINTFN(3, ("%s: receives unexpected packet: %s\n",
		    device_xname(sc->sc_dev), errstr));
#else
		__USE(errstr);
#endif
		m_freem(m);
	} else
		bth5_mux_receive(sc, m);
}

static const uint16_t crctbl[] = {
	0x0000, 0x1081, 0x2102, 0x3183,
	0x4204, 0x5285, 0x6306, 0x7387,
	0x8408, 0x9489, 0xa50a, 0xb58b,
	0xc60c, 0xd68d, 0xe70e, 0xf78f,
};

static void
bth5_crc_update(uint16_t *crc, uint8_t d)
{
	uint16_t reg = *crc;

	reg = (reg >> 4) ^ crctbl[(reg ^ d) & 0x000f];
	reg = (reg >> 4) ^ crctbl[(reg ^ (d >> 4)) & 0x000f];

	*crc = reg;
}

static uint16_t
bth5_crc_reverse(uint16_t crc)
{
	uint16_t b, rev;

	for (b = 0, rev = 0; b < 16; b++) {
		rev = rev << 1;
		rev |= (crc & 1);
		crc = crc >> 1;
	}

	return rev;
}


/*
 * BTH5 MUX Layer functions
 */
static void
bth5_mux_transmit(struct bth5_softc *sc)
{
	struct mbuf *m;
	bth5_hdr_t *hdrp;
	int s;

	DPRINTFN(2, ("%s: mux transmit: sc_flags=0x%x, choke=%d",
	    device_xname(sc->sc_dev), sc->sc_flags, sc->sc_mux_choke));

	if (sc->sc_mux_choke) {
		struct mbuf *_m = NULL;

		/* In this case, send only Link Establishment packet */
		for (m = MBUFQ_FIRST(&sc->sc_dgq); m != NULL;
		    _m = m, m = MBUFQ_NEXT(m)) {
			hdrp = mtod(m, bth5_hdr_t *);
			if (hdrp->ident == BTH5_CHANNEL_LE) {
				if (m == MBUFQ_FIRST(&sc->sc_dgq))
					MBUFQ_DEQUEUE(&sc->sc_dgq, m);
				else {
					if (m->m_nextpkt == NULL)
						sc->sc_dgq.mq_last =
						    &_m->m_nextpkt;
					_m->m_nextpkt = m->m_nextpkt;
					m->m_nextpkt = NULL;
				}
				goto transmit;
			}
		}
		DPRINTFN(2, ("\n"));
		return;
	}

	/*
	 * The MUX Layer always gives priority to packets from the Datagram
	 * Queue Layer over the Sequencing Layer.
	 */
	if (MBUFQ_FIRST(&sc->sc_dgq)) {
		MBUFQ_DEQUEUE(&sc->sc_dgq, m);
		goto transmit;
	}
	if (MBUFQ_FIRST(&sc->sc_seqq)) {
		MBUFQ_DEQUEUE(&sc->sc_seqq, m);
		hdrp = mtod(m, bth5_hdr_t *);
		hdrp->flags |= BTH5_FLAGS_PROTOCOL_REL;		/* Reliable */
		goto transmit;
	}

	s = spltty();
	if ((sc->sc_flags & BTH5_XMIT) == 0)
		bth5_start(sc);
	splx(s);

	if (sc->sc_mux_send_ack == true) {
		m = bth5_create_ackpkt();
		if (m != NULL)
			goto transmit;
		aprint_error_dev(sc->sc_dev, "out of memory\n");
		sc->sc_stats.err_tx++;
	}

	/* Nothing to send */
	DPRINTFN(2, ("\n"));

	return;

transmit:
	DPRINTFN(2, (", txack=%d, send_ack=%d\n",
	    bth5_get_txack(sc), sc->sc_mux_send_ack));

	hdrp = mtod(m, bth5_hdr_t *);
	hdrp->flags |=
	    (bth5_get_txack(sc) << BTH5_FLAGS_ACK_SHIFT) & BTH5_FLAGS_ACK_MASK;
	if (sc->sc_mux_send_ack == true)
		sc->sc_mux_send_ack = false;

#ifdef BTH5_DEBUG
	if (bth5_debug == 3)
		bth5_packet_print(m);
#endif

	sc->sc_txp = m;
	bth5_pktintegrity_transmit(sc);
}

static void
bth5_mux_receive(struct bth5_softc *sc, struct mbuf *m)
{
	bth5_hdr_t *hdrp = mtod(m, bth5_hdr_t *);
	const u_int rxack = BTH5_FLAGS_ACK(hdrp->flags);

	DPRINTFN(2, ("%s: mux receive: flags=0x%x, ident=%d, rxack=%d\n",
	    device_xname(sc->sc_dev), hdrp->flags, hdrp->ident, rxack));
#ifdef BTH5_DEBUG
	if (bth5_debug == 3)
		bth5_packet_print(m);
#endif

	bth5_signal_rxack(sc, rxack);

	microtime(&sc->sc_mux_lastrx);

	/* if the Ack Packet received then discard */
	if (BTH5_FLAGS_SEQ(hdrp->flags) == 0 &&
	    hdrp->ident == BTH5_IDENT_ACKPKT &&
	    BTH5_GET_PLEN(hdrp) == 0) {
		sc->sc_seq_txseq = BTH5_FLAGS_ACK(hdrp->flags);
		bth5_send_ack_command(sc);
		bth5_mux_transmit(sc);
		m_freem(m);
		return;
	}

	if (hdrp->flags & BTH5_FLAGS_PROTOCOL_REL)
		bth5_sequencing_receive(sc, m);
	else
		bth5_datagramq_receive(sc, m);
}

static __inline void
bth5_send_ack_command(struct bth5_softc *sc)
{

	DPRINTFN(2, ("%s: mux send_ack_command\n", device_xname(sc->sc_dev)));

	sc->sc_mux_send_ack = true;
}

static __inline struct mbuf *
bth5_create_ackpkt(void)
{
	struct mbuf *m;
	bth5_hdr_t *hdrp;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m != NULL) {
		m->m_pkthdr.len = m->m_len = sizeof(bth5_hdr_t);
		hdrp = mtod(m, bth5_hdr_t *);
		/*
		 * An Ack Packet has the following fields:
		 *	Ack Field:			txack (not set yet)
		 *	Seq Field:			0
		 *	Protocol Identifier Field:	0
		 *	Protocol Type Field:		Any value
		 *	Payload Length Field:		0
		 */
		memset(hdrp, 0, sizeof(bth5_hdr_t));
	}
	return m;
}

static __inline void
bth5_set_choke(struct bth5_softc *sc, bool choke)
{

	DPRINTFN(2, ("%s: mux set choke=%d\n", device_xname(sc->sc_dev), choke));

	sc->sc_mux_choke = choke;
}


/*
 * BTH5 Sequencing Layer functions
 */
static void
bth5_sequencing_receive(struct bth5_softc *sc, struct mbuf *m)
{
	bth5_hdr_t hdr;
	uint32_t exp_rxseq, rxack, rxseq;

	exp_rxseq = sc->sc_seq_expected_rxseq & BTH5_FLAGS_SEQ_MASK;
	m_copydata(m, 0, sizeof(bth5_hdr_t), &hdr);
	rxseq = BTH5_FLAGS_SEQ(hdr.flags);
	rxack = BTH5_FLAGS_ACK(hdr.flags);

	DPRINTFN(1, ("%s: seq receive: rxseq=%d, expected %d\n",
	    device_xname(sc->sc_dev), rxseq, exp_rxseq));
#ifdef BTH5_DEBUG
	if (bth5_debug == 2)
		bth5_packet_print(m);
#endif

	/*
	 * We remove the header of BTH5 and add the 'uint8_t type' of
	 * hci_*_hdr_t to the head. 
	 */
	m_adj(m, sizeof(bth5_hdr_t) - sizeof(uint8_t));

	if (rxseq != exp_rxseq) {
		m_freem(m);

		bth5_send_ack_command(sc);
		/* send ack packet, if needly */
		bth5_mux_transmit(sc);

		return;
	}

	switch (hdr.ident) {
	case BTH5_CHANNEL_HCI_CMD:
		*(mtod(m, uint8_t *)) = HCI_CMD_PKT;
		if (!hci_input_event(sc->sc_unit, m))
			sc->sc_stats.err_rx++;

		sc->sc_stats.evt_rx++;
		break;

	case BTH5_CHANNEL_HCI_EVT:
		*(mtod(m, uint8_t *)) = HCI_EVENT_PKT;
		if (!hci_input_event(sc->sc_unit, m))
			sc->sc_stats.err_rx++;

		sc->sc_stats.evt_rx++;
		break;

	case BTH5_CHANNEL_HCI_ACL:
		*(mtod(m, uint8_t *)) = HCI_ACL_DATA_PKT;
		if (!hci_input_acl(sc->sc_unit, m))
			sc->sc_stats.err_rx++;

		sc->sc_stats.acl_rx++;
		break;

	case BTH5_CHANNEL_HCI_SCO:
		*(mtod(m, uint8_t *)) = HCI_SCO_DATA_PKT;
		if (!hci_input_sco(sc->sc_unit, m))
			sc->sc_stats.err_rx++;

		sc->sc_stats.sco_rx++;
		break;

	default:
		aprint_error_dev(sc->sc_dev,
		    "received reliable packet with not support channel %d\n",
		    hdr.ident);
		m_freem(m);
		break;
	}
	bth5_send_ack_command(sc);
	sc->sc_seq_txseq = rxack;
	sc->sc_seq_expected_rxseq = (rxseq + 1) & BTH5_FLAGS_SEQ_MASK;
	sc->sc_seq_total_rxpkts++;

	if (sc->sc_seq_total_rxpkts % sc->sc_seq_winack == 0)
		bth5_mux_transmit(sc);
}

static bool
bth5_tx_reliable_pkt(struct bth5_softc *sc, struct mbuf *m, u_int protocol_id)
{
	bth5_hdr_t *hdrp;
	struct mbuf *_m;
	struct mbuf *_retrans;
	u_int pldlen;
	int s;

	DPRINTFN(1, ("%s: seq transmit:"
	    "protocol_id=%d, winspace=%d, txseq=%d\n", device_xname(sc->sc_dev),
	    protocol_id, sc->sc_seq_winspace, sc->sc_seq_txseq));

	for (pldlen = 0, _m = m; _m != NULL; _m = _m->m_next) {
		if (_m->m_len < 0)
			goto out;
		pldlen += _m->m_len;
	}
	if (pldlen > 0xfff)
		goto out;
	if (protocol_id == BTH5_IDENT_ACKPKT || protocol_id > 15)
		goto out;

	if (sc->sc_seq_winspace == 0)
		goto out;

	M_PREPEND(m, sizeof(bth5_hdr_t), M_DONTWAIT);
	if (m == NULL) {
		aprint_error_dev(sc->sc_dev, "out of memory\n");
		return false;
	}
	KASSERT(m->m_len >= sizeof(bth5_hdr_t));

	hdrp = mtod(m, bth5_hdr_t *);
	memset(hdrp, 0, sizeof(bth5_hdr_t));
	hdrp->flags |= sc->sc_seq_txseq;
	hdrp->ident = protocol_id;

	callout_schedule(&sc->sc_seq_timer, sc->sc_seq_timeout);

	s = splserial();
	MBUFQ_ENQUEUE(&sc->sc_seqq, m);
	splx(s);
	sc->sc_transmit_callback = bth5_reliabletx_callback;

#ifdef BTH5_DEBUG
	if (bth5_debug == 2)
		bth5_packet_print(m);
#endif

	sc->sc_seq_winspace--;
	_retrans = m_copym(m, 0, M_COPYALL, M_WAIT);
	if (_retrans == NULL) {
		aprint_error_dev(sc->sc_dev, "out of memory\n");
		goto out;
	}
	MBUFQ_ENQUEUE(&sc->sc_seq_retryq, _retrans);
	bth5_mux_transmit(sc);
	sc->sc_seq_txseq = (sc->sc_seq_txseq + 1) & BTH5_FLAGS_SEQ_MASK;

	return true;
out:
	m_freem(m);
	return false;
}

static __inline u_int
bth5_get_txack(struct bth5_softc *sc)
{

	return sc->sc_seq_expected_rxseq;
}

static void
bth5_signal_rxack(struct bth5_softc *sc, uint32_t rxack)
{
	bth5_hdr_t *hdrp;
	struct mbuf *m;
	uint32_t seqno = (rxack - 1) & BTH5_FLAGS_SEQ_MASK;
	int s;

	DPRINTFN(1, ("%s: seq signal rxack: rxack=%d\n",
	    device_xname(sc->sc_dev), rxack));

	s = splserial();
	m = MBUFQ_FIRST(&sc->sc_seq_retryq);
	while (m != NULL) {
		hdrp = mtod(m, bth5_hdr_t *);
		if (BTH5_FLAGS_SEQ(hdrp->flags) == seqno) {
			struct mbuf *m0;

			for (m0 = MBUFQ_FIRST(&sc->sc_seq_retryq);
			    m0 != MBUFQ_NEXT(m);
			    m0 = MBUFQ_FIRST(&sc->sc_seq_retryq)) {
				MBUFQ_DEQUEUE(&sc->sc_seq_retryq, m0);
				m_freem(m0);
				sc->sc_seq_winspace++;
				if (sc->sc_seq_winspace > sc->sc_seq_winsize)
					sc->sc_seq_winspace = sc->sc_seq_winsize;
			}
			break;
		}
		m = MBUFQ_NEXT(m);
	}
	splx(s);
	sc->sc_seq_retries = 0;

	if (sc->sc_seq_winspace == sc->sc_seq_winsize)
		callout_stop(&sc->sc_seq_timer);
	else
		callout_schedule(&sc->sc_seq_timer, sc->sc_seq_timeout);
}

static void
bth5_reliabletx_callback(struct bth5_softc *sc, struct mbuf *m)
{

	m_freem(m);
}

static void
bth5_timer_timeout(void *arg)
{
	struct bth5_softc *sc = arg;
	struct mbuf *m, *_m;
	int s, i = 0;

	DPRINTFN(1, ("%s: seq timeout: retries=%d\n",
	    device_xname(sc->sc_dev), sc->sc_seq_retries));

	bth5_send_ack_command(sc);
	bth5_mux_transmit(sc);
	s = splserial();
	for (m = MBUFQ_FIRST(&sc->sc_seq_retryq); m != NULL;
	    m = MBUFQ_NEXT(m)) {
		_m = m_copym(m, 0, M_COPYALL, M_DONTWAIT);
		if (_m == NULL) {
			aprint_error_dev(sc->sc_dev, "out of memory\n");
			return;
		}
		MBUFQ_ENQUEUE(&sc->sc_seqq, _m);
		i++;
	}
	splx(s);

	if (i != 0) {
		if (++sc->sc_seq_retries < sc->sc_seq_retry_limit)
			callout_schedule(&sc->sc_seq_timer, sc->sc_seq_timeout);
		else {
			aprint_error_dev(sc->sc_dev,
			    "reached the retry limit."
			    " restart the link-establishment\n");
			bth5_sequencing_reset(sc);
			bth5_start_le(sc);
			return;
		}
	}
	bth5_mux_transmit(sc);
}

static void
bth5_sequencing_reset(struct bth5_softc *sc)
{
	int s;

	s = splserial();
	MBUFQ_DRAIN(&sc->sc_seqq);
	MBUFQ_DRAIN(&sc->sc_seq_retryq);
	splx(s);


	sc->sc_seq_txseq = 0;
	sc->sc_seq_winspace = sc->sc_seq_winsize;
	sc->sc_seq_retries = 0;
	callout_stop(&sc->sc_seq_timer);

	sc->sc_mux_send_ack = false;

	/* XXXX: expected_rxseq should be set by MUX Layer */
	sc->sc_seq_expected_rxseq = 0;
	sc->sc_seq_total_rxpkts = 0;
}


/*
 * BTH5 Datagram Queue Layer functions
 */
static void
bth5_datagramq_receive(struct bth5_softc *sc, struct mbuf *m)
{
	bth5_hdr_t hdr;

	DPRINTFN(1, ("%s: dgq receive\n", device_xname(sc->sc_dev)));
#ifdef BTH5_DEBUG
	if (bth5_debug == 2)
		bth5_packet_print(m);
#endif

	m_copydata(m, 0, sizeof(bth5_hdr_t), &hdr);

	switch (hdr.ident) {
	case BTH5_CHANNEL_LE:
		m_adj(m, sizeof(bth5_hdr_t));
		bth5_input_le(sc, m);
		break;

	case BTH5_CHANNEL_HCI_SCO:
		/*
		 * We remove the header of BTH5 and add the 'uint8_t type' of
		 * hci_scodata_hdr_t to the head. 
		 */
		m_adj(m, sizeof(bth5_hdr_t) - sizeof(uint8_t));
		*(mtod(m, uint8_t *)) = HCI_SCO_DATA_PKT;
		if (!hci_input_sco(sc->sc_unit, m))
			sc->sc_stats.err_rx++;

		sc->sc_stats.sco_rx++;
		break;

	default:
		aprint_error_dev(sc->sc_dev,
		    "received unreliable packet with not support channel %d\n",
		    hdr.ident);
		m_freem(m);
		break;
	}
}

static bool
bth5_tx_unreliable_pkt(struct bth5_softc *sc, struct mbuf *m, u_int protocol_id)
{
	bth5_hdr_t *hdrp;
	struct mbuf *_m;
	u_int pldlen;
	int s;

	DPRINTFN(1, ("%s: dgq transmit: protocol_id=%d,",
	    device_xname(sc->sc_dev), protocol_id));

	for (pldlen = 0, _m = m; _m != NULL; _m = m->m_next) {
		if (_m->m_len < 0)
			goto out;
		pldlen += _m->m_len;
	}
	DPRINTFN(1, (" pldlen=%d\n", pldlen));
	if (pldlen > 0xfff)
		goto out;
	if (protocol_id == BTH5_IDENT_ACKPKT || protocol_id > 15)
		goto out;

	M_PREPEND(m, sizeof(bth5_hdr_t), M_DONTWAIT);
	if (m == NULL) {
		aprint_error_dev(sc->sc_dev, "out of memory\n");
		return false;
	}
	KASSERT(m->m_len >= sizeof(bth5_hdr_t));

	hdrp = mtod(m, bth5_hdr_t *);
	memset(hdrp, 0, sizeof(bth5_hdr_t));
	hdrp->ident = protocol_id;

	s = splserial();
	MBUFQ_ENQUEUE(&sc->sc_dgq, m);
	splx(s);
	sc->sc_transmit_callback = bth5_unreliabletx_callback;

#ifdef BTH5_DEBUG
	if (bth5_debug == 2)
		bth5_packet_print(m);
#endif

	bth5_mux_transmit(sc);

	return true;
out:
	m_freem(m);
	return false;
}

static void
bth5_unreliabletx_callback(struct bth5_softc *sc, struct mbuf *m)
{

	if (M_GETCTX(m, void *) == NULL)
		m_freem(m);
	else if (!hci_complete_sco(sc->sc_unit, m))
		sc->sc_stats.err_tx++;
}


/*
 * BTUART H5 Link Establishment Protocol functions
 */
static const uint8_t sync[] = BTH5_LE_SYNC;
static const uint8_t syncresp[] = BTH5_LE_SYNCRESP;
static const uint8_t conf[] = BTH5_LE_CONF;
static const uint8_t confresp[] = BTH5_LE_CONFRESP;

static int
bth5_start_le(struct bth5_softc *sc)
{

	DPRINTF(("%s: start link-establish\n", device_xname(sc->sc_dev)));

	bth5_set_choke(sc, true);

	if (!sc->sc_le_muzzled) {
		struct mbuf *m;

		m = m_gethdr(M_WAIT, MT_DATA);
		m->m_pkthdr.len = m->m_len = 0;
		m_copyback(m, 0, sizeof(sync), sync);
		if (!bth5_tx_unreliable_pkt(sc, m, BTH5_CHANNEL_LE)) {
			aprint_error_dev(sc->sc_dev,
			    "le-packet transmit failed\n");
			return EINVAL;
		}
	}
	callout_schedule(&sc->sc_le_timer, BTH5_LE_TSHY_TIMEOUT);

	sc->sc_le_state = le_state_shy;
	return 0;
}

static void
bth5_terminate_le(struct bth5_softc *sc)
{
	struct mbuf *m;

	/* terminate link-establishment */
	callout_stop(&sc->sc_le_timer);
	bth5_set_choke(sc, true);
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		aprint_error_dev(sc->sc_dev, "out of memory\n");
	else {
		/* length of le packets is 4 */
		m->m_pkthdr.len = m->m_len = 0;
		m_copyback(m, 0, sizeof(sync), sync);
		if (!bth5_tx_unreliable_pkt(sc, m, BTH5_CHANNEL_LE))
			aprint_error_dev(sc->sc_dev,
			    "link-establishment terminations failed\n");
	}
}

static void
bth5_input_le(struct bth5_softc *sc, struct mbuf *m)
{
	uint16_t *rcvpkt;
	int i, len;
	uint8_t config[3];
	const uint8_t *rplypkt;
	static struct {
		const char *type;
		const uint8_t *datap;
	} pkt[] = {
		{ "sync",	sync },
		{ "sync-resp",	syncresp },
		{ "conf",	conf },
		{ "conf-resp",	confresp },

		{ NULL, 0 }
	};

	DPRINTFN(0, ("%s: le input: state %d, muzzled %d\n",
	    device_xname(sc->sc_dev), sc->sc_le_state, sc->sc_le_muzzled));
#ifdef BTH5_DEBUG
	if (bth5_debug == 1)
		bth5_packet_print(m);
#endif

	rcvpkt = mtod(m, uint16_t *);
	i = 0;

	/* length of le packets is 2 */
	if (m->m_len >= sizeof(uint16_t))
		for (i = 0; pkt[i].type != NULL; i++)
			if (*(const uint16_t *)pkt[i].datap == *rcvpkt)
				break;
	if (m->m_len < sizeof(uint16_t) || pkt[i].type == NULL) {
		aprint_error_dev(sc->sc_dev, "received unknown packet\n");
		m_freem(m);
		return;
	}

	len = m->m_len;

	rplypkt = NULL;
	switch (sc->sc_le_state) {
	case le_state_shy:
		if (*rcvpkt == *(const uint16_t *)sync) {
			sc->sc_le_muzzled = false;
			rplypkt = syncresp;
		} else if (*rcvpkt == *(const uint16_t *)syncresp) {
			DPRINTF(("%s: state change to curious\n",
			    device_xname(sc->sc_dev)));
			rplypkt = conf;
			callout_schedule(&sc->sc_le_timer,
			    BTH5_LE_TCONF_TIMEOUT);
			sc->sc_le_state = le_state_curious;
		} else
			aprint_error_dev(sc->sc_dev,
			    "received an unknown packet at shy\n");
		break;

	case le_state_curious:
		if (*rcvpkt == *(const uint16_t *)sync)
			rplypkt = syncresp;
		else if (*rcvpkt == *(const uint16_t *)syncresp) {
			rplypkt = conf;
			len = 3;
		}
		else if (*rcvpkt == *(const uint16_t *)conf)
			rplypkt = confresp;
		else if (*rcvpkt == *(const uint16_t *)confresp &&
				m->m_len == 3) {
			DPRINTF(("%s: state change to garrulous:\n",
			    device_xname(sc->sc_dev)));

			memcpy(config, conf, sizeof(uint16_t));
			config[2] = (uint8_t)rcvpkt[1];
			sc->sc_seq_winack = config[2] & BTH5_CONFIG_ACK_MASK;
			if (config[2] & BTH5_CONFIG_FLOW_MASK)
				sc->sc_oof_flow_control = true;
			else
				sc->sc_oof_flow_control = false;

			bth5_sequencing_reset(sc);
			bth5_set_choke(sc, false);
			callout_stop(&sc->sc_le_timer);
			sc->sc_le_state = le_state_garrulous;
		} else
			aprint_error_dev(sc->sc_dev,
			    "received unknown packet at curious\n");
		break;

	case le_state_garrulous:
		if (*rcvpkt == *(const uint16_t *)conf)
			rplypkt = confresp;
		else if (*rcvpkt == *(const uint16_t *)sync) {
			/* XXXXX */
			aprint_error_dev(sc->sc_dev,
			    "received sync! peer to reset?\n");

			bth5_sequencing_reset(sc);
			rplypkt = syncresp;
			sc->sc_le_state = le_state_shy;
		} else
			aprint_error_dev(sc->sc_dev,
			    "received unknown packet at garrulous\n");
		break;
	}

	m_freem(m);

	if (rplypkt != NULL) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL)
			aprint_error_dev(sc->sc_dev, "out of memory\n");
		else {
			/* length of le packets is 2 */
			m->m_pkthdr.len = m->m_len = 0;
			if (rplypkt == (const uint8_t *)&config
			    || rplypkt == confresp || rplypkt == conf)
				m_copyback(m, 0, len, rplypkt);
			else
				m_copyback(m, 0, 2, rplypkt);
			if (!bth5_tx_unreliable_pkt(sc, m, BTH5_CHANNEL_LE))
				aprint_error_dev(sc->sc_dev,
				    "le-packet transmit failed\n");
		}
	}
}

static void
bth5_le_timeout(void *arg)
{
	struct bth5_softc *sc = arg;
	struct mbuf *m;
	int timeout;
	const uint8_t *sndpkt = NULL;

	DPRINTFN(0, ("%s: le timeout: state %d, muzzled %d\n",
	    device_xname(sc->sc_dev), sc->sc_le_state, sc->sc_le_muzzled));

	switch (sc->sc_le_state) {
	case le_state_shy:
		if (!sc->sc_le_muzzled)
			sndpkt = sync;
		timeout = BTH5_LE_TSHY_TIMEOUT;
		break;

	case le_state_curious:
		sndpkt = conf;
		timeout = BTH5_LE_TCONF_TIMEOUT;
		break;

	default:
		aprint_error_dev(sc->sc_dev,
		    "timeout happen at unknown state %d\n", sc->sc_le_state);
		return;
	}

	if (sndpkt != NULL) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL)
			aprint_error_dev(sc->sc_dev, "out of memory\n");
		else {
			/* length of le packets is 4 */
			m->m_pkthdr.len = m->m_len = 0;
			if (sndpkt == conf || sndpkt == confresp)
				m_copyback(m, 0, 3, sndpkt);
			else
				m_copyback(m, 0, 2, sndpkt);
			if (!bth5_tx_unreliable_pkt(sc, m, BTH5_CHANNEL_LE))
				aprint_error_dev(sc->sc_dev,
				    "le-packet transmit failed\n");
		}
	}

	callout_schedule(&sc->sc_le_timer, timeout);
}


/*
 * BTUART H5 Serial Protocol functions.
 */
static int
bth5_enable(device_t self)
{
	struct bth5_softc *sc = device_private(self);
	int s;

	if (sc->sc_flags & BTH5_ENABLED)
		return 0;

	s = spltty();

	sc->sc_flags |= BTH5_ENABLED;
	sc->sc_flags &= ~BTH5_XMIT;

	splx(s);

	return 0;
}

static void
bth5_disable(device_t self)
{
	struct bth5_softc *sc = device_private(self);
	int s;

	if ((sc->sc_flags & BTH5_ENABLED) == 0)
		return;

	s = spltty();

	if (sc->sc_rxp) {
		m_freem(sc->sc_rxp);
		sc->sc_rxp = NULL;
	}

	if (sc->sc_txp) {
		m_freem(sc->sc_txp);
		sc->sc_txp = NULL;
	}

	MBUFQ_DRAIN(&sc->sc_cmdq);
	MBUFQ_DRAIN(&sc->sc_aclq);
	MBUFQ_DRAIN(&sc->sc_scoq);

	sc->sc_flags &= ~BTH5_ENABLED;
	splx(s);
}

static void
bth5_start(struct bth5_softc *sc)
{
	struct mbuf *m;

	KASSERT((sc->sc_flags & BTH5_XMIT) == 0);
	KASSERT(sc->sc_txp == NULL);

	if (MBUFQ_FIRST(&sc->sc_aclq)) {
		MBUFQ_DEQUEUE(&sc->sc_aclq, m);
		sc->sc_stats.acl_tx++;
		sc->sc_flags |= BTH5_XMIT;
		bth5_tx_reliable_pkt(sc, m, BTH5_CHANNEL_HCI_ACL);
	}

	if (MBUFQ_FIRST(&sc->sc_cmdq)) {
		MBUFQ_DEQUEUE(&sc->sc_cmdq, m);
		sc->sc_stats.cmd_tx++;
		sc->sc_flags |= BTH5_XMIT;
		bth5_tx_reliable_pkt(sc, m, BTH5_CHANNEL_HCI_CMD);
	}

	if (MBUFQ_FIRST(&sc->sc_scoq)) {
		MBUFQ_DEQUEUE(&sc->sc_scoq, m);
		sc->sc_stats.sco_tx++;
		/* XXXX: We can transmit with reliable */
		sc->sc_flags |= BTH5_XMIT;
		bth5_tx_unreliable_pkt(sc, m, BTH5_CHANNEL_HCI_SCO);
	}

	return;
}

static void
bth5_output_cmd(device_t self, struct mbuf *m)
{
	struct bth5_softc *sc = device_private(self);
	int s;

	KASSERT(sc->sc_flags & BTH5_ENABLED);

	m_adj(m, sizeof(uint8_t));
	M_SETCTX(m, NULL);

	s = spltty();
	MBUFQ_ENQUEUE(&sc->sc_cmdq, m);
	if ((sc->sc_flags & BTH5_XMIT) == 0)
		bth5_start(sc);

	splx(s);
}

static void
bth5_output_acl(device_t self, struct mbuf *m)
{
	struct bth5_softc *sc = device_private(self);
	int s;

	KASSERT(sc->sc_flags & BTH5_ENABLED);

	m_adj(m, sizeof(uint8_t));
	M_SETCTX(m, NULL);

	s = spltty();
	MBUFQ_ENQUEUE(&sc->sc_aclq, m);
	if ((sc->sc_flags & BTH5_XMIT) == 0)
		bth5_start(sc);

	splx(s);
}

static void
bth5_output_sco(device_t self, struct mbuf *m)
{
	struct bth5_softc *sc = device_private(self);
	int s;

	KASSERT(sc->sc_flags & BTH5_ENABLED);

	m_adj(m, sizeof(uint8_t));

	s = spltty();
	MBUFQ_ENQUEUE(&sc->sc_scoq, m);
	if ((sc->sc_flags & BTH5_XMIT) == 0)
		bth5_start(sc);

	splx(s);
}

static void
bth5_stats(device_t self, struct bt_stats *dest, int flush)
{
	struct bth5_softc *sc = device_private(self);
	int s;

	s = spltty();
	memcpy(dest, &sc->sc_stats, sizeof(struct bt_stats));

	if (flush)
		memset(&sc->sc_stats, 0, sizeof(struct bt_stats));

	splx(s);
}


#ifdef BTH5_DEBUG
static void
bth5_packet_print(struct mbuf *m)
{
	int i;
	uint8_t *p;

	for ( ; m != NULL; m = m->m_next) {
		p = mtod(m, uint8_t *);
		for (i = 0; i < m->m_len; i++) {
			if (i % 16 == 0)
				printf(" ");
			printf(" %02x", *(p + i));
			if (i % 16 == 15)
				printf("\n");
		}
		printf("\n");
	}
}
#endif
