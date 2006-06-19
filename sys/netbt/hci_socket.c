/*	$NetBSD: hci_socket.c,v 1.1 2006/06/19 15:44:45 gdamore Exp $	*/

/*-
 * Copyright (c) 2005 Iain Hibbert.
 * Copyright (c) 2006 Itronix Inc.
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
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hci_socket.c,v 1.1 2006/06/19 15:44:45 gdamore Exp $");

#include "opt_bluetooth.h"
#ifdef BLUETOOTH_DEBUG
#define PRUREQUESTS
#define PRCOREQUESTS
#endif

#include <sys/param.h>
#include <sys/domain.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>

#include <netbt/bluetooth.h>
#include <netbt/hci.h>

/*******************************************************************************
 *
 * HCI SOCK_RAW Sockets - for control of Bluetooth Devices
 *
 */

/*
 * the raw HCI protocol control block
 */
struct hci_pcb {
	struct socket		*hp_socket;	/* socket */
	unsigned int		hp_flags;	/* flags */
	bdaddr_t		hp_laddr;	/* local address */
	bdaddr_t		hp_raddr;	/* remote address */
	struct hci_filter	hp_efilter;	/* user event filter */
	struct hci_filter	hp_pfilter;	/* user packet filter */
	LIST_ENTRY(hci_pcb)	hp_next;	/* next HCI pcb */
};

/* hp_flags */
#define HCI_PRIVILEGED		(1<<0)	/* no security filter for root */
#define HCI_DIRECTION		(1<<1)	/* direction control messages */
#define HCI_PROMISCUOUS		(1<<2)	/* listen to all units */

LIST_HEAD(hci_pcb_list, hci_pcb) hci_pcb = LIST_HEAD_INITIALIZER(hci_pcb);

/* sysctl defaults */
int hci_sendspace = HCI_CMD_PKT_SIZE;
int hci_recvspace = 4096;

/*
 * Security filter routines
 *	The security mask is an bit array.
 *	If the bit is set, the opcode/event is permitted.
 *	I only allocate the security mask if needed, and give it back
 *      if requested as its quite a chunk. This could no doubt be made
 *	somewhat bit more memory efficient given that most OGF/OCF's are
 *	not used, but this way is quick.
 */
#define HCI_OPCODE_MASK_SIZE	0x800

uint32_t *hci_event_mask = NULL;
uint32_t *hci_opcode_mask = NULL;

static __inline int
hci_security_check_opcode(uint16_t opcode)
{

	return (hci_opcode_mask[opcode >> 5] & (1 << (opcode & 0x1f)));
}

static __inline int
hci_security_check_event(uint8_t event)
{

	return (hci_event_mask[event >> 5] & (1 << (event & 0x1f)));
}

static __inline void
hci_security_set_opcode(uint16_t opcode)
{

	hci_opcode_mask[opcode >> 5] |= (1 << (opcode & 0x1f));
}

static __inline void
hci_security_clr_event(uint8_t event)
{

	hci_event_mask[event >> 5] &= ~(1 << (event & 0x1f));
}

static int
hci_security_init(void)
{

	if (hci_event_mask)
		return 0;

	// XXX could wait?
	hci_event_mask = malloc(HCI_EVENT_MASK_SIZE + HCI_OPCODE_MASK_SIZE,
				M_BLUETOOTH, M_NOWAIT | M_ZERO);
	if (hci_event_mask == NULL) {
		DPRINTF("no memory\n");
		return ENOMEM;
	}

	hci_opcode_mask = hci_event_mask + HCI_EVENT_MASK_SIZE;

	/* Events */
	/* enable all but a few critical events */
	memset(hci_event_mask, 0xff,
		HCI_EVENT_MASK_SIZE * sizeof(*hci_event_mask));
	hci_security_clr_event(HCI_EVENT_RETURN_LINK_KEYS);
	hci_security_clr_event(HCI_EVENT_LINK_KEY_NOTIFICATION);
	hci_security_clr_event(HCI_EVENT_VENDOR);

	/* Commands - Link control */
	hci_security_set_opcode(HCI_CMD_INQUIRY);
	hci_security_set_opcode(HCI_CMD_REMOTE_NAME_REQ);
	hci_security_set_opcode(HCI_CMD_READ_REMOTE_FEATURES);
	hci_security_set_opcode(HCI_CMD_READ_REMOTE_EXTENDED_FEATURES);
	hci_security_set_opcode(HCI_CMD_READ_REMOTE_VER_INFO);
	hci_security_set_opcode(HCI_CMD_READ_CLOCK_OFFSET);
	hci_security_set_opcode(HCI_CMD_READ_LMP_HANDLE);

	/* Commands - Link policy */
	hci_security_set_opcode(HCI_CMD_ROLE_DISCOVERY);
	hci_security_set_opcode(HCI_CMD_READ_LINK_POLICY_SETTINGS);
	hci_security_set_opcode(HCI_CMD_READ_DEFAULT_LINK_POLICY_SETTINGS);

	/* Commands - Host controller and baseband */
	hci_security_set_opcode(HCI_CMD_READ_PIN_TYPE);
	hci_security_set_opcode(HCI_CMD_READ_LOCAL_NAME);
	hci_security_set_opcode(HCI_CMD_READ_CON_ACCEPT_TIMEOUT);
	hci_security_set_opcode(HCI_CMD_READ_PAGE_TIMEOUT);
	hci_security_set_opcode(HCI_CMD_READ_SCAN_ENABLE);
	hci_security_set_opcode(HCI_CMD_READ_PAGE_SCAN_ACTIVITY);
	hci_security_set_opcode(HCI_CMD_READ_INQUIRY_SCAN_ACTIVITY);
	hci_security_set_opcode(HCI_CMD_READ_AUTH_ENABLE);
	hci_security_set_opcode(HCI_CMD_READ_ENCRYPTION_MODE);
	hci_security_set_opcode(HCI_CMD_READ_UNIT_CLASS);
	hci_security_set_opcode(HCI_CMD_READ_VOICE_SETTING);
	hci_security_set_opcode(HCI_CMD_READ_AUTO_FLUSH_TIMEOUT);
	hci_security_set_opcode(HCI_CMD_READ_NUM_BROADCAST_RETRANS);
	hci_security_set_opcode(HCI_CMD_READ_HOLD_MODE_ACTIVITY);
	hci_security_set_opcode(HCI_CMD_READ_XMIT_LEVEL);
	hci_security_set_opcode(HCI_CMD_READ_SCO_FLOW_CONTROL);
	hci_security_set_opcode(HCI_CMD_READ_LINK_SUPERVISION_TIMEOUT);
	hci_security_set_opcode(HCI_CMD_READ_NUM_SUPPORTED_IAC);
	hci_security_set_opcode(HCI_CMD_READ_IAC_LAP);
	hci_security_set_opcode(HCI_CMD_READ_PAGE_SCAN_PERIOD);
	hci_security_set_opcode(HCI_CMD_READ_PAGE_SCAN);
	hci_security_set_opcode(HCI_CMD_READ_INQUIRY_SCAN_TYPE);
	hci_security_set_opcode(HCI_CMD_READ_INQUIRY_MODE);
	hci_security_set_opcode(HCI_CMD_READ_PAGE_SCAN_TYPE);
	hci_security_set_opcode(HCI_CMD_READ_AFH_ASSESSMENT);

	/* Commands - Informational */
	hci_security_set_opcode(HCI_CMD_READ_LOCAL_VER);
	hci_security_set_opcode(HCI_CMD_READ_LOCAL_COMMANDS);
	hci_security_set_opcode(HCI_CMD_READ_LOCAL_FEATURES);
	hci_security_set_opcode(HCI_CMD_READ_LOCAL_EXTENDED_FEATURES);
	hci_security_set_opcode(HCI_CMD_READ_BUFFER_SIZE);
	hci_security_set_opcode(HCI_CMD_READ_COUNTRY_CODE);
	hci_security_set_opcode(HCI_CMD_READ_BDADDR);

	/* Commands - Status */
	hci_security_set_opcode(HCI_CMD_READ_FAILED_CONTACT_CNTR);
	hci_security_set_opcode(HCI_CMD_READ_LINK_QUALITY);
	hci_security_set_opcode(HCI_CMD_READ_RSSI);
	hci_security_set_opcode(HCI_CMD_READ_AFH_CHANNEL_MAP);
	hci_security_set_opcode(HCI_CMD_READ_CLOCK);

	/* Commands - Testing */
	hci_security_set_opcode(HCI_CMD_READ_LOOPBACK_MODE);

	return 0;
}

/*
 * When command packet reaches the device, we can drop
 * it from the socket buffer (called from hci_output_acl)
 */
void
hci_drop(void *arg)
{
	struct socket *so = arg;

	sbdroprecord(&so->so_snd);
	sowwakeup(so);
}

/*
 * HCI socket is going away and has some pending packets. We let them
 * go by design, but remove the context pointer as it will be invalid
 * and we no longer need to be notified.
 */
static void
hci_cmdwait_flush(struct socket *so)
{
	struct hci_unit *unit;
	struct socket *ctx;
	struct mbuf *m;

	DPRINTF("flushing %p\n", so);

	SIMPLEQ_FOREACH(unit, &hci_unit_list, hci_next) {
		m = MBUFQ_FIRST(&unit->hci_cmdwait);
		while (m != NULL) {
			ctx = M_GETCTX(m, struct socket *);
			if (ctx == so)
				M_SETCTX(m, NULL);

			m = MBUFQ_NEXT(m);
		}
	}
}

/*
 * HCI send packet
 *     This came from userland, so check it out.
 */
static int
hci_send(struct hci_pcb *pcb, struct mbuf *m, bdaddr_t *addr)
{
	struct hci_unit *unit;
	struct mbuf *m0;
	hci_cmd_hdr_t hdr;
	int err;

	KASSERT(m);
	KASSERT(addr);

	/* wants at least a header to start with */
	if (m->m_pkthdr.len < sizeof(hdr)) {
		err = EMSGSIZE;
		goto bad;
	}
	m_copydata(m, 0, sizeof(hdr), &hdr);

	/* only allows CMD packets to be sent */
	if (hdr.type != HCI_CMD_PKT) {
		err = EINVAL;
		goto bad;
	}

	/* validates packet length */
	if (m->m_pkthdr.len != sizeof(hdr) + hdr.length) {
		err = EMSGSIZE;
		goto bad;
	}

	/* security checks for unprivileged users */
	if ((pcb->hp_flags & HCI_PRIVILEGED) == 0
	    && (hci_security_check_opcode(le16toh(hdr.opcode)) == 0)) {
		err = EPERM;
		goto bad;
	}

	/* finds destination */
	unit = hci_unit_lookup(addr);
	if (unit == NULL) {
		err = ENETDOWN;
		goto bad;
	}

	/* makess a copy for precious to keep */
	m0 = m_copypacket(m, M_DONTWAIT);
	if (m0 == NULL) {
		err = ENOMEM;
		goto bad;
	}
	sbappendrecord(&pcb->hp_socket->so_snd, m0);
	M_SETCTX(m, pcb->hp_socket);	/* enable drop callback */

	DPRINTFN(2, "(%s) opcode (%03x|%04x)\n", unit->hci_devname,
		HCI_OGF(le16toh(hdr.opcode)), HCI_OCF(le16toh(hdr.opcode)));

	/* Sendss it */
	if (unit->hci_num_cmd_pkts == 0)
		MBUFQ_ENQUEUE(&unit->hci_cmdwait, m);
	else
		hci_output_cmd(unit, m);

	return 0;

bad:
	DPRINTF("packet (%d bytes) not sent (error %d)\n",
			m->m_pkthdr.len, err);
	if (m) m_freem(m);
	return err;
}

/*
 * User Request.
 * up is socket
 * m is either
 *	optional mbuf chain containing message
 *	ioctl command (PRU_CONTROL)
 * nam is either
 *	optional mbuf chain containing an address
 *	ioctl data (PRU_CONTROL)
 *      optionally, protocol number (PRU_ATTACH)
 * ctl is optional mbuf chain containing socket options
 * l is pointer to process requesting action (if any)
 *
 * we are responsible for disposing of m and ctl if
 * they are mbuf chains
 */
int
hci_usrreq(struct socket *up, int req, struct mbuf *m,
		struct mbuf *nam, struct mbuf *ctl, struct lwp *l)
{
	struct hci_pcb *pcb = (struct hci_pcb *)up->so_pcb;
	struct sockaddr_bt *sa;
	struct proc *p;
	int err = 0, flags;

	DPRINTFN(2, "%s\n", prurequests[req]);

	p = (l == NULL) ? NULL : l->l_proc;

	switch(req) {
	case PRU_CONTROL:
		return hci_ioctl((unsigned long)m, (void *)nam, p);

	case PRU_PURGEIF:
		return EOPNOTSUPP;

	case PRU_ATTACH:
		if (pcb)
			return EINVAL;

		flags = 0;
		if (p != NULL && kauth_authorize_generic(p->p_cred,
					    KAUTH_GENERIC_ISSUSER,
					    &p->p_acflag)) {
			err = hci_security_init();
			if (err)
				return err;
		} else {
			flags = HCI_PRIVILEGED;
		}

		err = soreserve(up, hci_sendspace, hci_recvspace);
		if (err)
			return err;

		pcb = malloc(sizeof(struct hci_pcb), M_PCB, M_NOWAIT | M_ZERO);
		if (pcb == NULL)
			return ENOMEM;

		up->so_pcb = pcb;
		pcb->hp_socket = up;
		pcb->hp_flags = flags;

		/*
		 * Set default user filter. By default, socket only passes
		 * Command_Complete and Command_Status Events.
		 */
		hci_filter_set(HCI_EVENT_COMMAND_COMPL, &pcb->hp_efilter);
		hci_filter_set(HCI_EVENT_COMMAND_STATUS, &pcb->hp_efilter);
		hci_filter_set(HCI_EVENT_PKT, &pcb->hp_pfilter);

		LIST_INSERT_HEAD(&hci_pcb, pcb, hp_next);

		return 0;
	}

	/* anything after here *requires* a pcb */
	if (pcb == NULL) {
		err = EINVAL;
		goto release;
	}

	switch(req) {
	case PRU_DISCONNECT:
		bdaddr_copy(&pcb->hp_raddr, BDADDR_ANY);

		/* XXX we cannot call soisdisconnected() here, as it sets
		 * SS_CANTRCVMORE and SS_CANTSENDMORE. The problem being,
		 * that soisconnected() does not clear these and if you
		 * try to reconnect this socket (which is permitted) you
		 * get a broken pipe when you try to write any data.
		 */
		up->so_state &= ~SS_ISCONNECTED;
		break;

	case PRU_ABORT:
		soisdisconnected(up);
		/* fall through to */
	case PRU_DETACH:
		if (up->so_snd.sb_mb != NULL)
			hci_cmdwait_flush(up);

		up->so_pcb = NULL;
		LIST_REMOVE(pcb, hp_next);
		free(pcb, M_PCB);
		return 0;

	case PRU_BIND:
		KASSERT(nam);
		sa = mtod(nam, struct sockaddr_bt *);

		if (sa->bt_len != sizeof(struct sockaddr_bt))
			return EINVAL;

		if (sa->bt_family != AF_BLUETOOTH)
			return EAFNOSUPPORT;

		bdaddr_copy(&pcb->hp_laddr, &sa->bt_bdaddr);

		if (bdaddr_any(&sa->bt_bdaddr))
			pcb->hp_flags |= HCI_PROMISCUOUS;
		else
			pcb->hp_flags &= ~HCI_PROMISCUOUS;

		return 0;

	case PRU_CONNECT:
		KASSERT(nam);
		sa = mtod(nam, struct sockaddr_bt *);

		if (sa->bt_len != sizeof(struct sockaddr_bt))
			return EINVAL;

		if (sa->bt_family != AF_BLUETOOTH)
			return EAFNOSUPPORT;

		if (hci_unit_lookup(&sa->bt_bdaddr) == NULL)
			return EADDRNOTAVAIL;

		bdaddr_copy(&pcb->hp_raddr, &sa->bt_bdaddr);
		soisconnected(up);
		return 0;

	case PRU_PEERADDR:
		KASSERT(nam);
		sa = mtod(nam, struct sockaddr_bt *);

		memset(sa, 0, sizeof(struct sockaddr_bt));
		nam->m_len =
		sa->bt_len = sizeof(struct sockaddr_bt);
		sa->bt_family = AF_BLUETOOTH;
		bdaddr_copy(&sa->bt_bdaddr, &pcb->hp_raddr);
		return 0;

	case PRU_SOCKADDR:
		KASSERT(nam);
		sa = mtod(nam, struct sockaddr_bt *);

		memset(sa, 0, sizeof(struct sockaddr_bt));
		nam->m_len =
		sa->bt_len = sizeof(struct sockaddr_bt);
		sa->bt_family = AF_BLUETOOTH;
		bdaddr_copy(&sa->bt_bdaddr, &pcb->hp_laddr);
		return 0;

	case PRU_SHUTDOWN:
		socantsendmore(up);
		break;

	case PRU_SEND:
		sa = NULL;
		if (nam) {
			sa = mtod(nam, struct sockaddr_bt *);

			if (sa->bt_len != sizeof(struct sockaddr_bt)) {
				err = EINVAL;
				goto release;
			}

			if (sa->bt_family != AF_BLUETOOTH) {
				err = EAFNOSUPPORT;
				goto release;
			}
		}

		if (ctl) /* have no use for this */
			m_freem(ctl);

		return hci_send(pcb, m, (sa ? &sa->bt_bdaddr : &pcb->hp_raddr));

	case PRU_SENSE:
		return 0;		/* (no sense - Doh!) */

	case PRU_RCVD:
	case PRU_RCVOOB:
		return EOPNOTSUPP;	/* (no release) */

	case PRU_ACCEPT:
	case PRU_CONNECT2:
	case PRU_LISTEN:
	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		err = EOPNOTSUPP;
		break;

	default:
		UNKNOWN(req);
		err = EOPNOTSUPP;
		break;
	}

release:
	if (m) m_freem(m);
	if (ctl) m_freem(ctl);
	return err;
}

/*
 * System is short on memory.
 */
void
hci_drain(void)
{

	/*
	 * We can give the security masks back, if there
	 * are no unprivileged sockets in operation..
	 */
	if (hci_event_mask) {
		struct hci_pcb *pcb;

		LIST_FOREACH(pcb, &hci_pcb, hp_next) {
			if ((pcb->hp_flags & HCI_PRIVILEGED) == 0)
				break;
		}

		if (pcb == NULL) {
			free(hci_event_mask, M_BLUETOOTH);
			hci_event_mask = NULL;
			hci_opcode_mask = NULL;
		}
	}
}

/*
 * get/set socket options
 */
int
hci_ctloutput(int req, struct socket *so, int level,
		int optname, struct mbuf **opt)
{
	struct hci_pcb *pcb = (struct hci_pcb *)so->so_pcb;
	struct mbuf *m;
	int err = 0;

	DPRINTFN(2, "req %s\n", prcorequests[req]);

	if (pcb == NULL)
		return EINVAL;

	if (level != BTPROTO_HCI)
		return 0;

	switch(req) {
	case PRCO_GETOPT:
		m = m_get(M_WAIT, MT_SOOPTS);
		switch (optname) {
		case SO_HCI_EVT_FILTER:
			m->m_len = sizeof(struct hci_filter);
			memcpy(mtod(m, void *), &pcb->hp_efilter, m->m_len);
			break;

		case SO_HCI_PKT_FILTER:
			m->m_len = sizeof(struct hci_filter);
			memcpy(mtod(m, void *), &pcb->hp_pfilter, m->m_len);
			break;

		case SO_HCI_DIRECTION:
			m->m_len = sizeof(int);
			if (pcb->hp_flags & HCI_DIRECTION)
				*mtod(m, int *) = 1;
			else
				*mtod(m, int *) = 0;
			break;

		default:
			err = EINVAL;
			m_freem(m);
			m = NULL;
			break;
		}
		*opt = m;
		break;

	case PRCO_SETOPT:
		m = *opt;
		if (m) switch (optname) {
		case SO_HCI_EVT_FILTER:	/* set event filter */
			m->m_len = min(m->m_len, sizeof(struct hci_filter));
			memcpy(&pcb->hp_efilter, mtod(m, void *), m->m_len);
			break;

		case SO_HCI_PKT_FILTER:	/* set packet filter */
			m->m_len = min(m->m_len, sizeof(struct hci_filter));
			memcpy(&pcb->hp_pfilter, mtod(m, void *), m->m_len);
			break;

		case SO_HCI_DIRECTION:	/* request direction ctl messages */
			if (*mtod(m, int *))
				pcb->hp_flags |= HCI_DIRECTION;
			else
				pcb->hp_flags &= ~HCI_DIRECTION;
			break;

		default:
			err = EINVAL;
			break;
		}
		m_freem(m);
		break;

	default:
		err = EINVAL;
		break;
	}

	return err;
}

/*
 * HCI mbuf tap routine
 *
 * copy packets to any raw HCI sockets that wish (and are
 * permitted) to see them
 */
void
hci_mtap(struct mbuf *m, struct hci_unit *unit)
{
	struct hci_pcb *pcb;
	struct mbuf *m0, *ctlmsg, **ctl;
	struct sockaddr_bt sa;
	uint8_t type;
	uint8_t event;
	uint16_t opcode;

	KASSERT(m->m_len >= sizeof(type));

	type = *mtod(m, uint8_t *);

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(struct sockaddr_bt);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, &unit->hci_bdaddr);

	LIST_FOREACH(pcb, &hci_pcb, hp_next) {
		/*
		 * filter according to source address
		 */
		if ((pcb->hp_flags & HCI_PROMISCUOUS) == 0
		    && bdaddr_same(&pcb->hp_laddr, &sa.bt_bdaddr) == 0)
			continue;

		/*
		 * filter according to packet type filter
		 */
		if (hci_filter_test(type, &pcb->hp_pfilter) == 0)
			continue;

		/*
		 * filter according to event/security filters
		 */
		switch(type) {
		case HCI_EVENT_PKT:
			KASSERT(m->m_len >= sizeof(hci_event_hdr_t));

			event = mtod(m, hci_event_hdr_t *)->event;

			if (hci_filter_test(event, &pcb->hp_efilter) == 0)
				continue;

			if ((pcb->hp_flags & HCI_PRIVILEGED) == 0
			    && hci_security_check_event(event) == 0)
				continue;
			break;

		case HCI_CMD_PKT:
			KASSERT(m->m_len >= sizeof(hci_cmd_hdr_t));

			opcode = le16toh(mtod(m, hci_cmd_hdr_t *)->opcode);

			if ((pcb->hp_flags & HCI_PRIVILEGED) == 0
			    && hci_security_check_opcode(opcode) == 0)
				continue;
			break;

		case HCI_ACL_DATA_PKT:
		case HCI_SCO_DATA_PKT:
		default:
			if ((pcb->hp_flags & HCI_PRIVILEGED) == 0)
				continue;

			break;
		}

		/*
		 * create control messages
		 */
		ctlmsg = NULL;
		ctl = &ctlmsg;
		if (pcb->hp_flags & HCI_DIRECTION) {
			int dir = m->m_flags & M_LINK0 ? 1 : 0;

			*ctl = sbcreatecontrol((caddr_t)&dir, sizeof(dir),
			    SCM_HCI_DIRECTION, BTPROTO_HCI);

			if (*ctl != NULL)
				ctl = &((*ctl)->m_next);
		}

		/*
		 * copy to socket
		 */
		m0 = m_copypacket(m, M_DONTWAIT);
		if (m0 && sbappendaddr(&pcb->hp_socket->so_rcv,
				(struct sockaddr *)&sa, m0, ctlmsg)) {
			sorwakeup(pcb->hp_socket);
		} else {
			m_freem(ctlmsg);
			m_freem(m0);
		}
	}
}
