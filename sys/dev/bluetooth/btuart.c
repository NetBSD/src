/*	$NetBSD: btuart.c,v 1.1.6.4 2007/03/24 14:55:20 yamt Exp $	*/
/*
 * Copyright (c) 2006, 2007 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: btuart.c,v 1.1.6.4 2007/03/24 14:55:20 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/syslimits.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <netbt/bluetooth.h>
#include <netbt/hci.h>

#include <dev/bluetooth/btuart.h>
#include <dev/firmload.h>

#include "ioconf.h"

#define BTUART_DEBUG
#ifdef BTUART_DEBUG
int btuart_debug = 1;
#endif

struct btuart_softc;
struct bth4hci {
	int type;
	int init_baud;
#define FLOW_CTL	1
	int flags;
	int (*init)(struct btuart_softc *);
};

struct btuart_softc {
	struct device sc_dev;

	struct tty *sc_tp;
	struct hci_unit sc_unit;		/* Bluetooth HCI Unit */

	struct bth4hci sc_bth4hci;
	int sc_baud;

	int sc_state;				/* receive state */
#define BTUART_RECV_PKT_TYPE	0			/* packet type */
#define BTUART_RECV_ACL_HDR	1			/* acl header */
#define BTUART_RECV_SCO_HDR	2			/* sco header */
#define BTUART_RECV_EVENT_HDR	3			/* event header */
#define BTUART_RECV_ACL_DATA	4			/* acl packet data */
#define BTUART_RECV_SCO_DATA	5			/* sco packet data */
#define BTUART_RECV_EVENT_DATA	6			/* event packet data */
	int sc_want;				/* how much we want */
	struct mbuf *sc_rxp;			/* incoming packet */
	struct mbuf *sc_txp;			/* outgoing packet */

	void (*sc_input_acl)(struct hci_unit *, struct mbuf *);
	void (*sc_input_sco)(struct hci_unit *, struct mbuf *);
	void (*sc_input_event)(struct hci_unit *, struct mbuf *);
};

void btuartattach(int);
static int btuart_match(struct device *, struct cfdata *, void *);
static void btuart_attach(struct device *, struct device *, void *);
static int btuart_detach(struct device *, int);

static int bth4_waitresp(struct btuart_softc *, struct mbuf **, uint16_t);
static int bth4_firmload(struct btuart_softc *, char *,
			 int (*)(struct btuart_softc *, int, char *));
static int init_ericsson(struct btuart_softc *);
static int init_digi(struct btuart_softc *);
static int init_texas(struct btuart_softc *);
static int init_csr(struct btuart_softc *);
static int init_swave(struct btuart_softc *);
static int init_st(struct btuart_softc *);
static int firmload_stlc2500(struct btuart_softc *, int, char *);
static int init_stlc2500(struct btuart_softc *);
static int init_bcm2035(struct btuart_softc *);
static int bth4init(struct btuart_softc *);
static void bth4init_input(struct hci_unit *, struct mbuf *);

static int bth4open(dev_t, struct tty *);
static int bth4close(struct tty *, int);
static int bth4ioctl(struct tty *, u_long, void *, int, struct lwp *);
static int bth4input(int, struct tty *);
static int bth4start(struct tty *);

static int bth4_enable(struct hci_unit *);
static void bth4_disable(struct hci_unit *);
static void bth4_start(struct hci_unit *);

/*
 * It doesn't need to be exported, as only btuartattach() uses it,
 * but there's no "official" way to make it static.
 */
CFATTACH_DECL(btuart, sizeof(struct btuart_softc),
    btuart_match, btuart_attach, btuart_detach, NULL);

static struct linesw bth4_disc = {
	.l_name = "btuart",
	.l_open = bth4open,
	.l_close = bth4close,
	.l_read = ttyerrio,
	.l_write = ttyerrio,
	.l_ioctl = bth4ioctl,
	.l_rint = bth4input,
	.l_start = bth4start,
	.l_modem = ttymodem,
	.l_poll = ttyerrpoll
};

static struct bth4hci bth4hci[] = {
	{ BTUART_HCITYPE_ANY,		     B0, FLOW_CTL, NULL },
	{ BTUART_HCITYPE_ERICSSON,	 B57600, FLOW_CTL, init_ericsson },
	{ BTUART_HCITYPE_DIGI,		  B9600, FLOW_CTL, init_digi },
	{ BTUART_HCITYPE_TEXAS,		B115200, FLOW_CTL, init_texas },
	/* CSR Casira serial adapter or BrainBoxes serial dongle (BL642) */
	{ BTUART_HCITYPE_CSR,		B115200, FLOW_CTL, init_csr },
	/* Silicon Wave kits */
	{ BTUART_HCITYPE_SWAVE,		B115200, FLOW_CTL, init_swave },
	/* ST Microelectronics minikits based on STLC2410/STLC2415 */
	{ BTUART_HCITYPE_ST,		 B57600, FLOW_CTL, init_st },
	/* ST Microelectronics minikits based on STLC2500 */
	{ BTUART_HCITYPE_STLC2500,	B115200, FLOW_CTL, init_stlc2500 },
	/* AmbiCom BT2000C Bluetooth PC/CF Card */
	{ BTUART_HCITYPE_BT2000C,	 B57600, FLOW_CTL, init_csr },
	/* Broadcom BCM2035 */
	{ BTUART_HCITYPE_BCM2035,	B115200,        0, init_bcm2035 },

	{ -1,				     B0,        0, NULL }
};


/* ARGSUSED */
void
btuartattach(int num __unused)
{
	int error;

	error = ttyldisc_attach(&bth4_disc);
	if (error) {
		aprint_error("%s: unable to register line discipline, "
		    "error = %d\n", btuart_cd.cd_name, error);
		return;
	}

	error = config_cfattach_attach(btuart_cd.cd_name, &btuart_ca);
	if (error) {
		aprint_error("%s: unable to register cfattach, error = %d\n",
		    btuart_cd.cd_name, error);
		config_cfdriver_detach(&btuart_cd);
		(void) ttyldisc_detach(&bth4_disc);
	}
}

/*
 * Autoconf match routine.
 *
 * XXX: unused: config_attach_pseudo(9) does not call ca_match.
 */
/* ARGSUSED */
static int
btuart_match(struct device *self __unused,
    struct cfdata *cfdata __unused, void *arg __unused)
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
btuart_attach(struct device *parent __unused,
    struct device *self, void *aux __unused)
{
	struct btuart_softc *sc = device_private(self);
	int i;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_input_acl = bth4init_input;
	sc->sc_input_sco = bth4init_input;
	sc->sc_input_event = bth4init_input;

	/* Copy default type */
	for (i = 0; bth4hci[i].type != BTUART_HCITYPE_ANY; i++);
	memcpy(&sc->sc_bth4hci, &bth4hci[i], sizeof(struct bth4hci));

	/* Attach Bluetooth unit */
	sc->sc_unit.hci_softc = sc;
	sc->sc_unit.hci_devname = sc->sc_dev.dv_xname;
	sc->sc_unit.hci_enable = bth4_enable;
	sc->sc_unit.hci_disable = bth4_disable;
	sc->sc_unit.hci_start_cmd = bth4_start;
	sc->sc_unit.hci_start_acl = bth4_start;
	sc->sc_unit.hci_start_sco = bth4_start;
	sc->sc_unit.hci_ipl = makeiplcookie(IPL_TTY);
	hci_attach(&sc->sc_unit);
}

/*
 * Autoconf detach routine.  Called when we close the line discipline.
 */
static int
btuart_detach(struct device *self, int flags __unused)
{
	struct btuart_softc *sc = device_private(self);

	hci_detach(&sc->sc_unit);

	return 0;
}


static int
bth4_waitresp(struct btuart_softc *sc, struct mbuf **mp, uint16_t opcode)
{
	struct hci_unit *unit = &sc->sc_unit;
	hci_event_hdr_t *e;
	int status = 0, rv;

	*mp = NULL;
	while (1 /* CONSTCOND */) {
		if ((rv =
		    tsleep(&unit->hci_eventq, PCATCH, "bth4init", 0)) != 0)
			return rv;

		MBUFQ_DEQUEUE(&unit->hci_eventq, *mp);
		unit->hci_eventqlen--;
		KASSERT(*mp != NULL);

		e = mtod(*mp, hci_event_hdr_t *);
		if (e->event == HCI_EVENT_COMMAND_COMPL) {
			hci_command_compl_ep *ep;

			ep = (hci_command_compl_ep *)(e + 1);
			if (ep->opcode == opcode) {
				status = *(char *)(ep + 1);
				break;
			}
		} else if (e->event == HCI_EVENT_COMMAND_STATUS) {
			hci_command_status_ep *ep;

			ep = (hci_command_status_ep *)(e + 1);
			if (ep->opcode == opcode) {
				status = ep->status;
				break;
			}
		} else if (e->event == HCI_EVENT_VENDOR)
				break;
	}

	return status;
}

static int
bth4_firmload(struct btuart_softc *sc, char *filename,
	      int (*func_firmload)(struct btuart_softc *, int, char *))
{
	const cfdriver_t cd = device_cfdriver(&sc->sc_dev);
	firmware_handle_t fh = NULL;
	int error, size;
	char *buf;

	if ((error = firmware_open(cd->cd_name, filename, &fh)) != 0) {
		printf("firmware_open failed\n");
		return error;
	}
	size = firmware_get_size(fh);
	if ((buf = firmware_malloc(size)) != NULL) {
		printf("firmware_malloc failed\n");
		firmware_close(fh);
		return ENOMEM;
	}

	if ((error = firmware_read(fh, 0, buf, size)) != 0)
		printf("firmware_read failed\n");
	if (error == 0)
		error = (*func_firmload)(sc, size, buf);

	firmware_close(fh);
	firmware_free(buf, size);

	return error;
}

/*
 * LSI initialize functions.
 */
static int
init_ericsson(struct btuart_softc *sc)
{
	struct mbuf *m;
	struct hci_unit *unit = &sc->sc_unit;
	hci_cmd_hdr_t *p;
	int i, error = 0;
	const uint16_t opcode = htole16(HCI_CMD_ERICSSON_SET_UART_BAUD_RATE);
	static struct {
		int baud;
		uint8_t param;
	} ericsson_baudtbl[] = {
		{ B460800, 0x00 },
		{ B230400, 0x01 },
		{ B115200, 0x02 },
		{  B57600, 0x03 },
		{  B28800, 0x04 },
		{  B14400, 0x05 },
		{   B7200, 0x06 },
#if defined(B3600)
		{   B3600, 0x07 },
#endif
		{   B1800, 0x08 },
#if defined(B900)
		{    B900, 0x09 },
#endif
#if defined(B153600)
		{ B153600, 0x10 },
#endif
		{  B76800, 0x11 },
		{  B38400, 0x12 },
		{  B19200, 0x13 },
		{   B9600, 0x14 },
		{   B4800, 0x15 },
		{   B2400, 0x16 },
		{   B1200, 0x17 },
		{    B600, 0x18 },
		{    B300, 0x19 },
		{ B921600, 0x20 },
		{      B0, 0xff }
	};

printf("sc_baud=%d, init_speed=%d\n", sc->sc_baud, sc->sc_bth4hci.init_baud);
	for (i = 0; ericsson_baudtbl[i].baud != sc->sc_baud; i++)
		if (ericsson_baudtbl[i].baud == B0)
			return EINVAL;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = sizeof(ericsson_baudtbl[0].param);
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);
	m_copyback(m, sizeof(hci_cmd_hdr_t), p->length,
	    &ericsson_baudtbl[i].param);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

#if 0
	error = bth4_waitresp(sc, &m, opcode);
	if (m != NULL) {
		if (error != 0) {
			printf("%s: Ericsson_Set_UART_Baud_Rate failed:"
			    " Status 0x%02x\n", sc->sc_dev.dv_xname, error);
			error = EFAULT;
		}
		m_freem(m);
	}
#else
	/*
	 * XXXX: We cannot correctly receive this response perhaps.  Wait
	 * until the transmission of the data of 5 bytes * 10 bit is completed.
	 *   1000000usec * 10bit * 5byte / baud
	 */
	delay(50000000 / sc->sc_bth4hci.init_baud);
#endif
	return error;
}

static int
init_digi(struct btuart_softc *sc)
{
	struct mbuf *m;
	struct hci_unit *unit = &sc->sc_unit;
	hci_cmd_hdr_t *p;
	uint8_t param;

	/* XXXX */
	switch (sc->sc_baud) {
	case B57600:
		param = 0x08;
		break;

	case B115200:
		param = 0x09;
		break;

	default:
		return EINVAL;
	}

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
#define HCI_CMD_DIGIANSWER_SET_UART_BAUD_RATE	0xfc07		/* XXXX */
	p->opcode = htole16(HCI_CMD_DIGIANSWER_SET_UART_BAUD_RATE);
	p->length = sizeof(param);
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);
	m_copyback(m, sizeof(hci_cmd_hdr_t), p->length, &param);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	/*
	 * XXXX
	 * Wait until the transmission of the data of 5 bytes * 10 bit is
	 * completed.
	 *     1000000usec * 10bit * 5byte / baud
	 */
	delay(50000000 / sc->sc_bth4hci.init_baud);
	return 0;
}

static int
init_texas(struct btuart_softc *sc)
{

	/* XXXX: Should we obtain the version of LMP? */
	return 0;
}

static int
init_csr(struct btuart_softc *sc)
{
	struct mbuf *m;
	struct hci_unit *unit = &sc->sc_unit;
	hci_cmd_hdr_t *p;
	int error;
	const uint16_t opcode = htole16(HCI_CMD_CSR_EXTN);
	struct {
		uint8_t last :1;
		uint8_t first :1;
#define CSR_BCCMD_CHANID_BCCMD		2
#define CSR_BCCMD_CHANID_HQ		3
#define CSR_BCCMD_CHANID_DEVMGRLIB	4
#define CSR_BCCMD_CHANID_L2CAPLIB	8
#define CSR_BCCMD_CHANID_RFCOMMLIB	9
#define CSR_BCCMD_CHANID_SDPLIB		10
#define CSR_BCCMD_CHANID_DFU		12
#define CSR_BCCMD_CHANID_VM		13
#define CSR_BCCMD_CHANID_LMDEBUG	20
		uint8_t chanid :6;

		struct {
#define CSR_BCCMD_MESSAGE_TYPE_GETREQ	0x0000
#define CSR_BCCMD_MESSAGE_TYPE_GETRESP	0x0001
#define CSR_BCCMD_MESSAGE_TYPE_SETREQ	0x0002
			uint16_t type;
			uint16_t length;
			uint16_t seqno;
#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART		0x6802
#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART_STOPB		0x2000
#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART_PARENB		0x4000
#define CSR_BCCMD_MESSAGE_VARID_CONFIG_UART_PARODD		0x8000
			uint16_t varid;
#define CSR_BCCMD_MESSAGE_STATUS_OK			0x0000
#define CSR_BCCMD_MESSAGE_STATUS_NO_SUCH_VARID		0x0001
#define CSR_BCCMD_MESSAGE_STATUS_TOO_BIG		0x0002
#define CSR_BCCMD_MESSAGE_STATUS_NO_VALUE		0x0003
#define CSR_BCCMD_MESSAGE_STATUS_BAD_REQ		0x0004
#define CSR_BCCMD_MESSAGE_STATUS_NO_ACCESS		0x0005
#define CSR_BCCMD_MESSAGE_STATUS_READ_ONLY		0x0006
#define CSR_BCCMD_MESSAGE_STATUS_WRITE_ONLY		0x0007
#define CSR_BCCMD_MESSAGE_STATUS_ERROR			0x0008
#define CSR_BCCMD_MESSAGE_STATUS_PERMISION_DENIED	0x0009
			uint16_t status;
			uint16_t payload[4];
		} message;
	} bccmd;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = sizeof(bccmd);
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

	/* setup BCSP command packet */
	bccmd.last = 1;
	bccmd.first = 1;
	bccmd.chanid = CSR_BCCMD_CHANID_BCCMD;
	bccmd.message.type = htole16(CSR_BCCMD_MESSAGE_TYPE_SETREQ);
	bccmd.message.length = htole16(sizeof(bccmd.message) >> 1);
	bccmd.message.seqno = htole16(0);
	bccmd.message.varid = htole16(CSR_BCCMD_MESSAGE_VARID_CONFIG_UART);
	bccmd.message.status = htole16(CSR_BCCMD_MESSAGE_STATUS_OK);
	memset(bccmd.message.payload, 0, sizeof(bccmd.message.payload));

	/* Value = (baud rate / 244.140625) | no parity | 1 stop bit. */
	bccmd.message.payload[0] = htole16((sc->sc_baud * 64 + 7812) / 15625);

	m_copyback(m, sizeof(hci_cmd_hdr_t), p->length, &bccmd);
	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	error = bth4_waitresp(sc, &m, opcode);
	if (m != NULL) {
		/*
		 * XXXX:
		 * We will have to check the HCI_EVENT_VENDOR packet. For
		 * instance, it might be a different HCI_EVENT_VENDOR packet.
		 */
		if (error != 0) {
			printf("%s: CSR set UART speed failed: Status 0x%02x\n",
			    sc->sc_dev.dv_xname, error);
			error = EFAULT;
		}
		m_freem(m);
	}

	return error;
}

static int
init_swave(struct btuart_softc *sc)
{
	struct mbuf *m;
	struct hci_unit *unit = &sc->sc_unit;
	hci_cmd_hdr_t *p;
	hci_event_hdr_t *e;
	int i, error;
#define HCI_CMD_SWAVE_SET_UART_BAUD_RATE	0xfc0b		/* XXXX */
	const uint16_t opcode = htole16(HCI_CMD_SWAVE_SET_UART_BAUD_RATE);
	char param[6], *resp;
	static struct {			/* XXXX */
		int baud;
		uint8_t param;
	} swave_baudtbl[] = {
		{  B19200, 0x03 },
		{  B38400, 0x02 },
		{  B57600, 0x01 },
		{ B115200, 0x00 },
		{      B0, 0xff }
	};

	for (i = 0; swave_baudtbl[i].baud != sc->sc_baud; i++)
		if (swave_baudtbl[i].baud == B0)
			return EINVAL;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	/* first send 'param access set' command. */
	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = sizeof(param);
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

	/* XXXX */
	param[0] = 0x01;		/* param sub command */
	param[1] = 0x11;		/* HCI Tranport Params */
	param[2] = 0x03;		/* length of the parameter following */
	param[3] = 0x01;		/* HCI Transport flow control enable */
	param[4] = 0x01;		/* HCI Transport Type = UART */
	param[5] = swave_baudtbl[i].param;
	m_copyback(m, sizeof(hci_cmd_hdr_t), p->length, &param);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	while(1 /* CONSTCOND */) {
		error = bth4_waitresp(sc, &m, opcode);
		if (error != 0) {
			if (m != NULL)
				m_freem(m);
			printf("%s: swave set baud rate command failed:"
			    " error 0x%02x\n", sc->sc_dev.dv_xname, error);
			return error;
		}
		if (m != NULL) {
			e = mtod(m, hci_event_hdr_t *);
			resp = (char *)(e + 1);
			if (e->length == 7 && *resp == 0xb &&
			    memcmp(resp + 1, param, sizeof(param)) == 0)
				break;
			m_freem(m);
		}
	}

	/* send 'reset' command consecutively. */
	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
	p->opcode = htole16(HCI_CMD_RESET);
	p->length = 0;
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

	/*
	 * XXXX
	 * Wait until the transmission of the data of 4 bytes * 10 bit is
	 * completed.
	 *     1000000usec * 10bit * 4byte / baud
	 */
	delay(40000000 / sc->sc_bth4hci.init_baud);
	return 0;
}

static int
init_st(struct btuart_softc *sc)
{
	struct mbuf *m;
	struct hci_unit *unit = &sc->sc_unit;
	hci_cmd_hdr_t *p;
	int i;
	static struct {			/* XXXX */
		int baud;
		uint8_t param;
	} st_baudtbl[] = {
		{   B9600, 0x09 },
		{  B19200, 0x0b },
		{  B38400, 0x0d },
		{  B57600, 0x0e },
		{ B115200, 0x10 },
		{ B230400, 0x12 },
		{ B460800, 0x13 },
		{ B921600, 0x14 },
		{      B0, 0xff }
	};

	for (i = 0; st_baudtbl[i].baud != sc->sc_baud; i++)
		if (st_baudtbl[i].baud == B0)
			return EINVAL;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
#define HCI_CMD_ST_SET_UART_BAUD_RATE	0xfc46	/* XXXX */
	p->opcode = htole16(HCI_CMD_ST_SET_UART_BAUD_RATE);
	p->length = sizeof(st_baudtbl[0].param);
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);
	m_copyback(m, sizeof(hci_cmd_hdr_t), p->length, &st_baudtbl[i].param);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	/*
	 * XXXX
	 * Wait until the transmission of the data of 5 bytes * 10 bit is
	 * completed.
	 *     1000000usec * 10bit * 5byte / baud
	 */
	delay(50000000 / sc->sc_bth4hci.init_baud);
	return 0;
}

static int
firmload_stlc2500(struct btuart_softc *sc, int size, char *buf)
{
	struct hci_unit *unit = &sc->sc_unit;
	struct mbuf *m;
	hci_cmd_hdr_t *p;
	int error, offset, n;
	uint16_t opcode = htole16(0xfc2e);	/* XXXX */
	uint8_t seq;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;
	seq = 0;
	offset = 0;
	error = 0;
	while (offset < size) {
		n = size - offset < 254 ? size - offset : 254;

		p = mtod(m, hci_cmd_hdr_t *);
		p->type = HCI_CMD_PKT;
		p->opcode = opcode;
		p->length = n;
		m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);
		*(char *)(p + 1) = seq;
		m_copyback(m,
		    sizeof(hci_cmd_hdr_t) + 1, p->length, buf + offset);

		MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
		bth4_start(unit);

		error = bth4_waitresp(sc, &m, opcode);
		if (m != NULL) {
			if (error != 0) {
				printf("%s: stlc2500 firmware load failed:"
				    " Status 0x%02x\n",
				    sc->sc_dev.dv_xname, error);
				error = EFAULT;
				break;
			}
		}

		seq++;
		offset += n;
	}
	m_freem(m);

	return error;
}

static int
init_stlc2500(struct btuart_softc *sc)
{
	struct mbuf *m;
	struct hci_unit *unit = &sc->sc_unit;
	hci_cmd_hdr_t *p;
	hci_event_hdr_t *e;
	hci_read_local_ver_rp *lv;
	int error, revision, i;
	uint16_t opcode;
	char filename[NAME_MAX], param[8];
	static const char filenametmpl[] = "STLC2500_R%d_%02d%s";
	const char *suffix[] = { ".ptc", ".ssf", NULL };

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	p = mtod(m, hci_cmd_hdr_t *);
	opcode = htole16(HCI_CMD_READ_LOCAL_VER);
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = 0;
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	error = bth4_waitresp(sc, &m, opcode);
	if (m != NULL) {
		if (error != 0) {
			printf("%s: HCI_Read_Local_Version_Information failed:"
			    " Status 0x%02x\n", sc->sc_dev.dv_xname, error);
			error = EFAULT;
			m_freem(m);
		}
	}
	if (error != 0)
		return error;

	e = mtod(m, hci_event_hdr_t *);
	lv = (hci_read_local_ver_rp *)(e + 1);
	revision = le16toh(lv->hci_revision);
	opcode = htole16(HCI_CMD_RESET);
	for (i = 0; suffix[i] != NULL; i++) {
		/* send firmware */
		snprintf(filename, sizeof(filename), filenametmpl,
		    (uint8_t)(revision >> 8), (uint8_t)revision, suffix[i]);
		bth4_firmload(sc, filename, firmload_stlc2500);

		p = mtod(m, hci_cmd_hdr_t *);
		p->type = HCI_CMD_PKT;
		p->opcode = opcode;
		p->length = 0;
		m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

		MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
		bth4_start(unit);

		error = bth4_waitresp(sc, &m, opcode);
		if (m != NULL) {
			if (error != 0) {
				printf("%s: HCI_Reset (%d) failed:"
				    " Status 0x%02x\n",
				    sc->sc_dev.dv_xname, i, error);
				error = EFAULT;
				m_freem(m);
			}
		}
		if (error != 0)
			return error;
	}

	/* XXXX: We will obtain the character string.  But I don't know... */
	p = mtod(m, hci_cmd_hdr_t *);
	opcode = htole16(0xfc0f);		/* XXXXX ?? */
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = 0;
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	error = bth4_waitresp(sc, &m, opcode);
	if (m != NULL) {
		if (error != 0) {
			printf("%s: failed: opcode 0xfc0f Status 0x%02x\n",
			    sc->sc_dev.dv_xname, error);
			error = EFAULT;
			m_freem(m);
		}
	}
	if (error != 0)
		return error;
	/*
	 * XXXX:
	 * We do not know the beginning point of this character string.
	 * Because it doesn't know the event of this packet.
	 *
	 * printf("%s: %s\n", sc->sc_dev.dv_xname, ???);
	 */

	p = mtod(m, hci_cmd_hdr_t *);
	opcode = htole16(0xfc22);		/* XXXXX ?? */
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = sizeof(param);
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

	/* XXXX */
	param[0] = 0xfe;
	param[1] = 0x06;
	param[2] = 0xba;
	param[3] = 0xab;
	param[4] = 0x00;
	param[5] = 0xe1;
	param[6] = 0x80;
	param[7] = 0x00;
	m_copyback(m, sizeof(hci_cmd_hdr_t), p->length, param);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	error = bth4_waitresp(sc, &m, opcode);
	if (m != NULL) {
		if (error != 0) {
			printf("%s: failed: opcode 0xfc0f Status 0x%02x\n",
			    sc->sc_dev.dv_xname, error);
			error = EFAULT;
			m_freem(m);
		}
	}
	if (error != 0)
		return error;

	opcode = htole16(HCI_CMD_RESET);
	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = 0;
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	error = bth4_waitresp(sc, &m, opcode);
	if (m != NULL) {
		if (error != 0) {
			printf("%s: HCI_Reset failed: Status 0x%02x\n",
			    sc->sc_dev.dv_xname, error);
			error = EFAULT;
			m_freem(m);
		}
	}

	return error;
}

static int
init_bcm2035(struct btuart_softc *sc)
{
	struct mbuf *m;
	struct hci_unit *unit = &sc->sc_unit;
	hci_cmd_hdr_t *p;
	int i, error;
#define HCI_CMD_BCM2035_SET_UART_BAUD_RATE	0xfc18	/* XXXX */
	const uint16_t opcode = htole16(HCI_CMD_BCM2035_SET_UART_BAUD_RATE);
	static struct {			/* XXXX */
		int baud;
		uint16_t param;
	} bcm2035_baudtbl[] = {
		{  B57600, 0xe600 },
		{ B230400, 0xfa22 },
		{ B460800, 0xfd11 },
		{ B921600, 0xff65 },
		{      B0, 0xffff }
	};

	for (i = 0; bcm2035_baudtbl[i].baud != sc->sc_baud; i++)
		if (bcm2035_baudtbl[i].baud == -1)
			return EINVAL;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	/*
	 * XXXX: Should we send some commands?
	 *         HCI_CMD_RESET and HCI_CMD_READ_LOCAL_VER and
	 *         HCI_CMD_READ_LOCAL_COMMANDS
	 */

	p = mtod(m, hci_cmd_hdr_t *);
	p->type = HCI_CMD_PKT;
	p->opcode = opcode;
	p->length = sizeof(bcm2035_baudtbl[0].param);
	m->m_pkthdr.len = m->m_len = sizeof(hci_cmd_hdr_t);
	m_copyback(m, sizeof(hci_cmd_hdr_t), p->length,
	    &bcm2035_baudtbl[i].param);

	MBUFQ_ENQUEUE(&unit->hci_cmdq, m);
	bth4_start(unit);

	error = bth4_waitresp(sc, &m, opcode);
	if (m != NULL) {
		if (error != 0) {
			printf("%s: bcm2035 set baud rate failed:"
			    " Status 0x%02x\n", sc->sc_dev.dv_xname, error);
			error = EFAULT;
		}
		m_freem(m);
	}

	return error;
}

static int
bth4init(struct btuart_softc *sc)
{
	struct tty *tp = sc->sc_tp;
	struct termios t;
	int error = 0, s;

	sc->sc_baud = tp->t_ospeed;
	t.c_cflag = tp->t_cflag;
	t.c_ispeed = 0;
	t.c_ospeed = tp->t_ospeed;
	if ((tp->t_cflag & CRTSCTS) && !(sc->sc_bth4hci.flags & FLOW_CTL))
		t.c_cflag &= ~CRTSCTS;
	if (sc->sc_bth4hci.init_baud != 0 &&
	    tp->t_ospeed != sc->sc_bth4hci.init_baud)
		t.c_ospeed = sc->sc_bth4hci.init_baud;
	if (t.c_ospeed != tp->t_ospeed || t.c_cflag != tp->t_cflag)
		error = (*tp->t_param)(tp, &t);

	if (error == 0 && sc->sc_bth4hci.init != NULL)
		error = (*sc->sc_bth4hci.init)(sc);

	s = splserial();
	sc->sc_input_acl = hci_input_acl;
	sc->sc_input_sco = hci_input_sco;
	sc->sc_input_event = hci_input_event;
	splx(s);

	if (sc->sc_bth4hci.init_baud != 0 &&
	    sc->sc_bth4hci.init_baud != sc->sc_baud) {
		t.c_ospeed = sc->sc_baud;
		t.c_cflag = tp->t_cflag;
		error = (*tp->t_param)(tp, &t);
	}

	return error;
}

static void
bth4init_input(struct hci_unit *unit, struct mbuf *m)
{
	int i;
	uint8_t *rptr =  mtod(m, uint8_t *);
	const char *pktstr = NULL;

	switch (*rptr) {
	case HCI_ACL_DATA_PKT:
		pktstr = "acl data";
		break;

	case HCI_SCO_DATA_PKT:
		pktstr = "sco data";
		break;

	case HCI_EVENT_PKT:
		break;

	default:
		pktstr = "unknown";
		break;
	}
	if (pktstr != NULL)
		printf("%s: %s packet was received in initialization phase\n",
		    unit->hci_devname, pktstr);
	if (
#ifdef BTUART_DEBUG
	    btuart_debug ||
#endif
	    pktstr != NULL) {
		printf("%s: %s:", __FUNCTION__, unit->hci_devname);
		for (i = 0; i < m->m_len; i++)
			printf(" %02x", *(rptr + i));
		printf("\n");
	}

	if (*rptr == HCI_EVENT_PKT)
		if (unit->hci_eventqlen <= hci_eventq_max) {
			unit->hci_eventqlen++;
			MBUFQ_ENQUEUE(&unit->hci_eventq, m);
			m = NULL;
			wakeup(&unit->hci_eventq);
		}

	if (m != NULL)
		m_freem(m);
}


/*
 * Line discipline functions.
 */
/* ARGSUSED */
static int
bth4open(dev_t device __unused, struct tty *tp)
{
	struct btuart_softc *sc;
	struct cfdata *cfdata;
	struct lwp *l = curlwp;		/* XXX */
	int error, unit, s;
	static char name[] = "btuart";

	if ((error = kauth_authorize_device_tty(l->l_cred,
	    KAUTH_GENERIC_ISSUSER, tp)) != 0)
		return error;

	s = spltty();

	if (tp->t_linesw == &bth4_disc) {
		sc = (struct btuart_softc *)tp->t_sc;
		if (sc != NULL) {
			splx(s);
			return EBUSY;
		}
	}

	KASSERT(tp->t_oproc != NULL);

	cfdata = malloc(sizeof(struct cfdata), M_DEVBUF, M_WAITOK);
	for (unit = 0; unit < btuart_cd.cd_ndevs; unit++)
		if (btuart_cd.cd_devs[unit] == NULL)
			break;
	cfdata->cf_name = name;
	cfdata->cf_atname = name;
	cfdata->cf_unit = unit;
	cfdata->cf_fstate = FSTATE_STAR;

	printf("%s%d at tty major %d minor %d",
	    name, unit, major(tp->t_dev), minor(tp->t_dev));
	sc = (struct btuart_softc *)config_attach_pseudo(cfdata);
	if (sc == NULL) {
		splx(s);
		return EIO;
	}
	tp->t_sc = sc;
	sc->sc_tp = tp;

	ttyflush(tp, FREAD | FWRITE);

	splx(s);

	return 0;
}

/* ARGSUSED */
static int
bth4close(struct tty *tp, int flag __unused)
{
	struct btuart_softc *sc;
	struct cfdata *cfdata;
	int s, baud;

	sc = tp->t_sc;

	/* reset to initial speed */
	if (sc->sc_bth4hci.init != NULL) {
		baud = sc->sc_baud;
		sc->sc_baud = sc->sc_bth4hci.init_baud;
		sc->sc_bth4hci.init_baud = baud;
		s = splserial();
		sc->sc_input_acl = bth4init_input;
		sc->sc_input_sco = bth4init_input;
		sc->sc_input_event = bth4init_input;
		splx(s);
		if ((*sc->sc_bth4hci.init)(sc) != 0)
			printf("%s: reset speed fail\n", sc->sc_dev.dv_xname);
	}

	s = spltty();
	ttyflush(tp, FREAD | FWRITE);
	ttyldisc_release(tp->t_linesw);
	tp->t_linesw = ttyldisc_default();
	if (sc != NULL) {
		tp->t_sc = NULL;
		if (sc->sc_tp == tp) {
			cfdata = sc->sc_dev.dv_cfdata;
			config_detach(&sc->sc_dev, 0);
			free(cfdata, M_DEVBUF);
		}

	}
	splx(s);
	return 0;
}

/* ARGSUSED */
static int
bth4ioctl(struct tty *tp, u_long cmd, void *data,
    int flag __unused, struct lwp *l __unused)
{
	struct btuart_softc *sc = (struct btuart_softc *)tp->t_sc;
	int error, i;

	if (sc == NULL || tp != sc->sc_tp)
		return EPASSTHROUGH;

	error = 0;
	switch (cmd) {
	case BTUART_HCITYPE:
		for (i = 0; bth4hci[i].type != -1; i++)
			if (bth4hci[i].type == *(uint32_t *)data)
				break;
		if (bth4hci[i].type != -1)
			memcpy(&sc->sc_bth4hci, &bth4hci[i],
			    sizeof(struct bth4hci));
		else
			error = EINVAL;
		break;

	case BTUART_INITSPEED:
		sc->sc_bth4hci.init_baud = *(uint32_t *)data;
		break;

	case BTUART_START:
		error = bth4init(sc);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	return error;
}

static int
bth4input(int c, struct tty *tp)
{
	struct btuart_softc *sc = (struct btuart_softc *)tp->t_sc;
	struct mbuf *m = sc->sc_rxp;
	int space = 0;

	c &= TTY_CHARMASK;

	/* If we already started a packet, find the trailing end of it. */
	if (m) {
		while (m->m_next)
			m = m->m_next;

		space = M_TRAILINGSPACE(m);
	}

	if (space == 0) {
		if (m == NULL) {
			/* new packet */
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				printf("%s: out of memory\n",
				    sc->sc_dev.dv_xname);
				++sc->sc_unit.hci_stats.err_rx;
				return 0;	/* (lost sync) */
			}

			sc->sc_rxp = m;
			m->m_pkthdr.len = m->m_len = 0;
			space = MHLEN;

			sc->sc_state = BTUART_RECV_PKT_TYPE;
			sc->sc_want = 1;
		} else {
			/* extend mbuf */
			MGET(m->m_next, M_DONTWAIT, MT_DATA);
			if (m->m_next == NULL) {
				printf("%s: out of memory\n",
				    sc->sc_dev.dv_xname);
				++sc->sc_unit.hci_stats.err_rx;
				return 0;	/* (lost sync) */
			}

			m = m->m_next;
			m->m_len = 0;
			space = MLEN;

			if (sc->sc_want > MINCLSIZE) {
				MCLGET(m, M_DONTWAIT);
				if (m->m_flags & M_EXT)
					space = MCLBYTES;
			}
		}
	}

	mtod(m, uint8_t *)[m->m_len++] = c;
	sc->sc_rxp->m_pkthdr.len++;
	sc->sc_unit.hci_stats.byte_rx++;

	sc->sc_want--;
	if (sc->sc_want > 0)
		return 0;	/* want more */

	switch (sc->sc_state) {
	case BTUART_RECV_PKT_TYPE:	/* Got packet type */

		switch (c) {
		case HCI_ACL_DATA_PKT:
			sc->sc_state = BTUART_RECV_ACL_HDR;
			sc->sc_want = sizeof(hci_acldata_hdr_t) - 1;
			break;

		case HCI_SCO_DATA_PKT:
			sc->sc_state = BTUART_RECV_SCO_HDR;
			sc->sc_want = sizeof(hci_scodata_hdr_t) - 1;
			break;

		case HCI_EVENT_PKT:
			sc->sc_state = BTUART_RECV_EVENT_HDR;
			sc->sc_want = sizeof(hci_event_hdr_t) - 1;
			break;

		default:
			printf("%s: Unknown packet type=%#x!\n",
			    sc->sc_dev.dv_xname, c);
			sc->sc_unit.hci_stats.err_rx++;
			m_freem(sc->sc_rxp);
			sc->sc_rxp = NULL;
			return 0;	/* (lost sync) */
		}

		break;

	/*
	 * we assume (correctly of course :) that the packet headers all fit
	 * into a single pkthdr mbuf
	 */
	case BTUART_RECV_ACL_HDR:	/* Got ACL Header */
		sc->sc_state = BTUART_RECV_ACL_DATA;
		sc->sc_want = mtod(m, hci_acldata_hdr_t *)->length;
		sc->sc_want = le16toh(sc->sc_want);
		break;

	case BTUART_RECV_SCO_HDR:	/* Got SCO Header */
		sc->sc_state = BTUART_RECV_SCO_DATA;
		sc->sc_want =  mtod(m, hci_scodata_hdr_t *)->length;
		break;

	case BTUART_RECV_EVENT_HDR:	/* Got Event Header */
		sc->sc_state = BTUART_RECV_EVENT_DATA;
		sc->sc_want =  mtod(m, hci_event_hdr_t *)->length;
		break;

	case BTUART_RECV_ACL_DATA:	/* ACL Packet Complete */
		(*sc->sc_input_acl)(&sc->sc_unit, sc->sc_rxp);
		sc->sc_unit.hci_stats.acl_rx++;
		sc->sc_rxp = m = NULL;
		break;

	case BTUART_RECV_SCO_DATA:	/* SCO Packet Complete */
		(*sc->sc_input_sco)(&sc->sc_unit, sc->sc_rxp);
		sc->sc_unit.hci_stats.sco_rx++;
		sc->sc_rxp = m = NULL;
		break;

	case BTUART_RECV_EVENT_DATA:	/* Event Packet Complete */
		sc->sc_unit.hci_stats.evt_rx++;
		(*sc->sc_input_event)(&sc->sc_unit, sc->sc_rxp);
		sc->sc_rxp = m = NULL;
		break;

	default:
		panic("%s: invalid state %d!\n",
		    sc->sc_dev.dv_xname, sc->sc_state);
	}

	return 0;
}

static int
bth4start(struct tty *tp)
{
	struct btuart_softc *sc = (struct btuart_softc *)tp->t_sc;
	struct mbuf *m;
	int count, rlen;
	uint8_t *rptr;

	m = sc->sc_txp;
	if (m == NULL) {
		sc->sc_unit.hci_flags &= ~BTF_XMIT;
		bth4_start(&sc->sc_unit);
		return 0;
	}

	count = 0;
	rlen = 0;
	rptr = mtod(m, uint8_t *);

	for(;;) {
		if (rlen >= m->m_len) {
			m = m->m_next;
			if (m == NULL) {
				m = sc->sc_txp;
				sc->sc_txp = NULL;

				if (M_GETCTX(m, void *) == NULL)
					m_freem(m);
				else
					hci_complete_sco(&sc->sc_unit, m);

				break;
			}

			rlen = 0;
			rptr = mtod(m, uint8_t *);
			continue;
		}

		if (putc(*rptr++, &tp->t_outq) < 0) {
			m_adj(m, rlen);
			break;
		}
		rlen++;
		count++;
	}

	sc->sc_unit.hci_stats.byte_tx += count;

	if (tp->t_outq.c_cc != 0)
		(*tp->t_oproc)(tp);

	return 0;
}


/*
 * HCI UART (H4) functions.
 */
static int
bth4_enable(struct hci_unit *unit)
{

	if (unit->hci_flags & BTF_RUNNING)
		return 0;

	unit->hci_flags |= BTF_RUNNING;
	unit->hci_flags &= ~BTF_XMIT;

	return 0;
}

static void
bth4_disable(struct hci_unit *unit)
{
	struct btuart_softc *sc = unit->hci_softc;

	if ((unit->hci_flags & BTF_RUNNING) == 0)
		return;

	if (sc->sc_rxp) {
		m_freem(sc->sc_rxp);
		sc->sc_rxp = NULL;
	}

	if (sc->sc_txp) {
		m_freem(sc->sc_txp);
		sc->sc_txp = NULL;
	}

	unit->hci_flags &= ~BTF_RUNNING;
}

static void
bth4_start(struct hci_unit *unit)
{
	struct btuart_softc *sc = unit->hci_softc;
	struct mbuf *m;

	KASSERT((unit->hci_flags & BTF_XMIT) == 0);
	KASSERT(sc->sc_txp == NULL);

	if (MBUFQ_FIRST(&unit->hci_cmdq)) {
		MBUFQ_DEQUEUE(&unit->hci_cmdq, m);
		unit->hci_stats.cmd_tx++;
		M_SETCTX(m, NULL);
		goto start;
	}

	if (MBUFQ_FIRST(&unit->hci_scotxq)) {
		MBUFQ_DEQUEUE(&unit->hci_scotxq, m);
		unit->hci_stats.sco_tx++;
		goto start;
	}

	if (MBUFQ_FIRST(&unit->hci_acltxq)) {
		MBUFQ_DEQUEUE(&unit->hci_acltxq, m);
		unit->hci_stats.acl_tx++;
		M_SETCTX(m, NULL);
		goto start;
	}

	/* Nothing to send */
	return;

start:
	sc->sc_txp = m;
	unit->hci_flags |= BTF_XMIT;
	bth4start(sc->sc_tp);
}
