/*
 * Copyright (c) 1997,2001 Martin Husemann <martin@duskware.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 */

#ifndef DIEHLISDNVAR
#define DIEHLISDNVAR
#if defined(KERNEL) || defined(_KERNEL)

#define	DAIC_MAX_PORT		4
#define	DAIC_ISA_MEMSIZE	2048
#define	DAIC_ISA_QUADSIZE	(DAIC_MAX_PORT*DAIC_ISA_MEMSIZE)
#define	DAIC_MAX_ACTIVE		(2*DAIC_MAX_PORT)	/* simlutaneous connections on one instance */

/* A queue of pending outgoing calls */
struct outcallentry {
	TAILQ_ENTRY(outcallentry) queue;
	unsigned int cdid;	/* call descriptor id for layer 4 */
	u_int8_t dchan_id;	/* task id for microcode */
	u_int8_t rc;		/* return code */
};

/* One active connection */
struct daic_connection {
	u_int cdid;			/* layer 4 call descriptor id */
	struct ifqueue tx_queue;	/* outgoing data */
	struct ifqueue rx_queue;	/* incoming data */
	isdn_link_t isdn_linktab;	/* description of ourself */
	drvr_link_t *drvr_linktab;	/* application driver using us */
	u_int8_t dchan_inst;		/* d-channel instance */
	u_int8_t dchan_rc;		/* return code for dchannel requests */
	u_int8_t bchan_inst;		/* b-channel instance */
	u_int8_t bchan_rc;		/* return code for bchannel requests */
};

/* superclass of all softc structs for attachments, you should
 * always be able to cast an attachments struct device *self to this. */
struct daic {
	struct device sc_dev;
	bus_space_tag_t sc_iot;		/* bus identifier */
	bus_space_handle_t sc_ioh;	/* mem handle */
	int sc_cardtype;		/* variant of card */
#define	DAIC_TYPE_S	0
#define	DAIC_TYPE_SX	1
#define	DAIC_TYPE_SCOM	2
#define	DAIC_TYPE_QUAD	3

	int sc_ctrl[DAIC_MAX_PORT];	/* upper layer controller number */

	int sc_state[DAIC_MAX_PORT];	/* state of board, see below */
#define	DAIC_STATE_COLD		0	/* nothing happened to the board */
#define	DAIC_STATE_DOWNLOAD	1	/* the card is waiting for protocol code */
#define DAIC_STATE_TESTING	2	/* waiting for first interrupt from card */
#define	DAIC_STATE_RUNNING	3	/* the protocol is running */
#define	DAIC_STATE_DIAGNOSTIC	4	/* temporary disabled for diagnostics */

	int sc_assign[DAIC_MAX_PORT];	/* allocating new protocol instance */
#define	DAIC_ASSIGN_PENDING	1	/* we are awaiting a new ID */
#define	DAIC_ASSIGN_SLEEPING	2	/* somebody sleeping, wakeup after we got the ID */
#define	DAIC_ASSIGN_GLOBAL	4	/* result is a global d-channel id */
#define	DAIC_ASSIGN_NOGLOBAL	8	/* need to assign a global d-channel id */

	u_int8_t sc_global_dchan[DAIC_MAX_PORT];	/* handle of global dchannel instance */
	u_int8_t sc_request_res[DAIC_MAX_PORT];		/* result of requests */
	u_int8_t sc_assign_res[DAIC_MAX_PORT];		/* result of assigns */

	/* one record for each b-channel we could handle concurrently */
	struct daic_connection sc_con[DAIC_MAX_ACTIVE];
	/* a tailq of ougoing calls no yet assigned to any b-channel */
	TAILQ_HEAD(outcallhead, outcallentry) sc_outcalls[DAIC_MAX_PORT];
};

/*
 * functions exported from MI part
 */
extern int daic_probe __P((bus_space_tag_t bus, bus_space_handle_t io));
extern void daic_attach __P((struct device *self, struct daic *sc));
extern int daic_intr __P((struct daic *));

#endif
#endif
