/*	$NetBSD: i4b_capi_l4if.c,v 1.2 2003/09/26 15:17:23 pooka Exp $	*/

/*
 * Copyright (c) 2001-2003 Cubical Solutions Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * capi/capi_l4if.c	The CAPI i4b L4/device interface.
 *
 * $FreeBSD: src/sys/i4b/capi/capi_l4if.c,v 1.4 2002/04/04 21:03:20 jhb Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_capi_l4if.c,v 1.2 2003/09/26 15:17:23 pooka Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/callout.h>
#include <net/if.h>

#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_cause.h>
#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>
#include <netisdn/i4b_l4.h>
#include <netisdn/i4b_capi.h>
#include <netisdn/i4b_capi_msgs.h>

static void n_connect_request(call_desc_t *);
static void n_connect_response(call_desc_t *, int response, int cause);
static void n_disconnect_request(call_desc_t *, int cause);
static void n_alert_request(call_desc_t *);
static void n_mgmt_command(struct isdn_l3_driver *, int cmd, void *parm);
static int  n_download(void *, int, struct isdn_dr_prot *);

static int ncapi = 0;

/*
//  i4b_capi_{ret,set}_linktab
//      i4b driver glue.
//
//  i4b_capi_bch_config
//      Called by i4b driver to flush + {en,dis}able a channel.
//
//  i4b_capi_bch_start_tx
//      Called by i4b driver to transmit a queued mbuf.
//
//  i4b_capi_bch_stat
//      Called by i4b driver to obtain statistics information.
*/

static isdn_link_t *
i4b_capi_ret_linktab(void *token, int channel)
{
    capi_softc_t *sc = token;

    return &sc->sc_bchan[channel].capi_isdn_linktab;
}

static void
i4b_capi_set_link(void *token, int channel,
    const struct isdn_l4_driver_functions *l4_driver, void *l4_inst)
{
    capi_softc_t *sc = token;

    sc->sc_bchan[channel].l4_driver = l4_driver;
    sc->sc_bchan[channel].l4_driver_softc = l4_inst;
}

static void
i4b_capi_bch_config(void *token, int chan, int bprot, int activate)
{
    capi_softc_t *sc = token;

    i4b_Bcleanifq(&sc->sc_bchan[chan].tx_queue);
    sc->sc_bchan[chan].tx_queue.ifq_maxlen = IFQ_MAXLEN;
    sc->sc_bchan[chan].txcount = 0;

    /* The telephony drivers use rx_queue for receive. */
    i4b_Bcleanifq(&sc->sc_bchan[chan].rx_queue);
    sc->sc_bchan[chan].rx_queue.ifq_maxlen = IFQ_MAXLEN;
    sc->sc_bchan[chan].rxcount = 0;

    /* HDLC frames are put to in_mbuf */
    i4b_Bfreembuf(sc->sc_bchan[chan].in_mbuf);
    sc->sc_bchan[chan].in_mbuf = NULL;

    /* Because of the difference, we need to remember the protocol. */
    sc->sc_bchan[chan].bprot = bprot;
    sc->sc_bchan[chan].busy = 0;
}

static void
i4b_capi_bch_start_tx(void *token, int chan)
{
    capi_softc_t *sc = token;
    int s;

    s = splnet();

    if (sc->sc_bchan[chan].state != B_CONNECTED) {
	splx(s);
	printf("capi%d: start_tx on unconnected channel\n", sc->sc_unit);
	return;
    }

    if (sc->sc_bchan[chan].busy) {
	splx(s);
	return;
    }

    capi_start_tx(sc, chan);

    splx(s);
}

static void
i4b_capi_bch_stat(void *token, int chan, bchan_statistics_t *bsp)
{
    capi_softc_t *sc = token;
    int s = splnet();

    bsp->outbytes = sc->sc_bchan[chan].txcount;
    bsp->inbytes = sc->sc_bchan[chan].rxcount;

    sc->sc_bchan[chan].txcount = 0;
    sc->sc_bchan[chan].rxcount = 0;

    splx(s);
}

int capi_start_tx(void *token, int chan)
{
    capi_softc_t *sc = token;
    struct mbuf *m_b3;
    int sent = 0;

    IF_DEQUEUE(&sc->sc_bchan[chan].tx_queue, m_b3);
    while (m_b3) {
	struct mbuf *m = m_b3->m_next;

	sc->sc_bchan[chan].txcount += m_b3->m_len;
	capi_data_b3_req(sc, chan, m_b3);
	sent++;

	m_b3 = m;
    }

    if (sc->sc_bchan[chan].l4_driver) {
	capi_bchan_t *bch = &sc->sc_bchan[chan];

	/* Notify i4b driver of activity, and if the queue is drained. */
	if (sent)
	    (*bch->l4_driver->bch_activity)(bch->l4_driver_softc, ACT_TX);

	if (IF_QEMPTY(&bch->tx_queue))
	    (*bch->l4_driver->bch_tx_queue_empty)(bch->l4_driver_softc);
    }

    return sent;
}

static const struct isdn_l4_bchannel_functions
capi_l4_driver = {
    i4b_capi_bch_config,
    i4b_capi_bch_start_tx,
    i4b_capi_bch_stat
};

/*
//  n_mgmt_command
//      i4b L4 management command.
*/

static void
n_mgmt_command(struct isdn_l3_driver *l3, int op, void *arg)
{
    capi_softc_t *sc = l3->l1_token;

#if 0
    printf("capi%d: mgmt command %d\n", sc->sc_unit, op);
#endif

    switch(op) {
    case CMR_DOPEN:
	sc->sc_enabled = 1;
	break;

    case CMR_DCLOSE:
	sc->sc_enabled = 0;
	break;

    case CMR_SETTRACE:
	break;

    default:
	break;
    }
}

/*
//  n_connect_request
//      i4b L4 wants to connect. We assign a B channel to the call,
//      send a CAPI_CONNECT_REQ, and set the channel to B_CONNECT_CONF.
*/

static void
n_connect_request(call_desc_t *cd)
{
    capi_softc_t *sc;
    int bch, s;

    sc = cd->l3drv->l1_token;
    bch = cd->channelid;

    s = splnet();

    if ((bch < 0) || (bch >= sc->sc_nbch))
	for (bch = 0; bch < sc->sc_nbch; bch++)
	    if (sc->sc_bchan[bch].state == B_FREE)
		break;

    if (bch == sc->sc_nbch) {
	splx(s);
	printf("capi%d: no free B channel\n", sc->sc_unit);
	return;
    }

    cd->channelid = bch;

    capi_connect_req(sc, cd);
    splx(s);
}

/*
//  n_connect_response
//      i4b L4 answers a call. We send a CONNECT_RESP with the proper
//      Reject code, and set the channel to B_CONNECT_B3_IND or B_FREE,
//      depending whether we answer or not.
*/

static void
n_connect_response(call_desc_t *cd, int response, int cause)
{
    capi_softc_t *sc;
    int bch, s;

    sc = cd->l3drv->l1_token;
    bch = cd->channelid;

    T400_stop(cd);

    cd->response = response;
    cd->cause_out = cause;

    s = splnet();
    capi_connect_resp(sc, cd);
    splx(s);
}

/*
//  n_disconnect_request
//      i4b L4 wants to disconnect. We send a DISCONNECT_REQ and
//      set the channel to B_DISCONNECT_CONF.
*/

static void
n_disconnect_request(call_desc_t *cd, int cause)
{
    capi_softc_t *sc;
    int bch, s;

    sc = cd->l3drv->l1_token;
    bch = cd->channelid;

    cd->cause_out = cause;

    s = splnet();
    capi_disconnect_req(sc, cd);
    splx(s);
}

/*
//  n_alert_request
//      i4b L4 wants to alert an incoming call. We send ALERT_REQ.
*/

static void
n_alert_request(call_desc_t *cd)
{
    capi_softc_t *sc;
    int s;

    sc = cd->l3drv->l1_token;

    s = splnet();
    capi_alert_req(sc, cd);
    splx(s);
}

/*
//  n_download
//      L4 -> firmware download
*/

static int
n_download(void *token, int numprotos, struct isdn_dr_prot *protocols)
{
    capi_softc_t *sc = token;

    if (sc->load) {
	(*sc->load)(sc, protocols[0].bytecount,
			       protocols[0].microcode);
	return(0);
    }

    return(ENXIO);
}

static const struct isdn_l3_driver_functions
capi_l3_functions = {
    i4b_capi_ret_linktab,
    i4b_capi_set_link,
    n_connect_request,
    n_connect_response,
    n_disconnect_request,
    n_alert_request,
    n_download,
    NULL,
    n_mgmt_command
};

/*
//  capi_ll_attach
//      Called by a link layer driver at boot time.
*/

int
capi_ll_attach(capi_softc_t *sc, const char *devname, const char *cardname)
{
    struct isdn_l3_driver *l3drv;
    int i;

    /* Unit state */

    sc->sc_enabled = 0;
    sc->sc_state = C_DOWN;
    sc->sc_msgid = 0;

    for (i = 0; i < sc->sc_nbch; i++) {
	sc->sc_bchan[i].ncci = INVALID;
	sc->sc_bchan[i].msgid = 0;
	sc->sc_bchan[i].busy = 0;
	sc->sc_bchan[i].state = B_FREE;

	memset(&sc->sc_bchan[i].tx_queue, 0, sizeof(struct ifqueue));
	memset(&sc->sc_bchan[i].rx_queue, 0, sizeof(struct ifqueue));
	sc->sc_bchan[i].tx_queue.ifq_maxlen = IFQ_MAXLEN;
	sc->sc_bchan[i].rx_queue.ifq_maxlen = IFQ_MAXLEN;

	sc->sc_bchan[i].txcount = 0;
	sc->sc_bchan[i].rxcount = 0;

	sc->sc_bchan[i].cdid = CDID_UNUSED;
	sc->sc_bchan[i].bprot = BPROT_NONE;
	sc->sc_bchan[i].in_mbuf = NULL;

	sc->sc_bchan[i].capi_isdn_linktab.l1token = sc;
	sc->sc_bchan[i].capi_isdn_linktab.channel = i;
	sc->sc_bchan[i].capi_isdn_linktab.bchannel_driver = &capi_l4_driver;
	sc->sc_bchan[i].capi_isdn_linktab.tx_queue = &sc->sc_bchan[i].tx_queue;
	sc->sc_bchan[i].capi_isdn_linktab.rx_queue = &sc->sc_bchan[i].rx_queue;
	sc->sc_bchan[i].capi_isdn_linktab.rx_mbuf = &sc->sc_bchan[i].in_mbuf;
    }

    l3drv = isdn_attach_bri(devname, cardname, sc, &capi_l3_functions);

    l3drv->tei = -1;
    l3drv->dl_est = DL_DOWN;
    l3drv->nbch = sc->sc_nbch;

    sc->sc_unit = ncapi++;
    sc->capi_bri = l3drv->bri;

    isdn_bri_ready(l3drv->bri);

    printf("capi%d: card type %d attached\n", sc->sc_unit, sc->card_type);

    return(0);
}


/*
//  capi_ll_detach
*/

int
capi_ll_detach(capi_softc_t *sc)
{

	/* TODO */
	return(0);
}
