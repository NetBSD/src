/*
 * Copyright (c) 1997, 1999 Hellmuth Michaelis. All rights reserved.
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
 *---------------------------------------------------------------------------
 *
 *	i4b_l3l4.h - layer 3 / layer 4 interface
 *	------------------------------------------
 *
 *	$Id: i4b_l3l4.h,v 1.5 2002/03/17 20:54:05 martin Exp $
 *
 * $FreeBSD$
 *
 *	last edit-date: [Fri Jun  2 14:29:35 2000]
 *
 *---------------------------------------------------------------------------*/

#ifndef _I4B_L3L4_H_
#define _I4B_L3L4_H_

#define T303VAL	(hz*4)			/* 4 seconds timeout		*/
#define T305VAL	(hz*30)			/* 30 seconds timeout		*/
#define T308VAL	(hz*4)			/* 4 seconds timeout		*/
#define T309VAL	(hz*90)			/* 90 seconds timeout		*/
#define T310VAL	(hz*60)			/* 30-120 seconds timeout	*/
#define T313VAL	(hz*4)			/* 4 seconds timeout		*/
#define T400DEF	(hz*10)			/* 10 seconds timeout		*/

#define N_CALL_DESC (MAX_CONTROLLERS*2)	/* no of call descriptors */

extern int nctrl;		/* number of controllers detected in system */

typedef struct bchan_statistics {
	int outbytes;
	int inbytes;
} bchan_statistics_t;

/*---------------------------------------------------------------------------*
 * table of things the driver needs to know about the b channel
 * it is connected to for data transfer
 *---------------------------------------------------------------------------*/
typedef struct i4b_isdn_bchan_linktab {
	void* l1token;
	int channel;
	const struct isdn_l4_bchannel_functions *bchannel_driver;
	struct ifqueue *tx_queue;
	struct ifqueue *rx_queue;	/* data xfer for NON-HDLC traffic   */
	struct mbuf **rx_mbuf;		/* data xfer for HDLC based traffic */
} isdn_link_t;

struct isdn_l4_driver_functions;

/* global linktab functions for controller types (aka hardware drivers) */
struct ctrl_type_desc {
	isdn_link_t* (*get_linktab)(void*, int channel);
	void (*set_l4_driver)(void*, int channel, const struct isdn_l4_driver_functions *l4_driver, void *l4_driver_softc);
};
extern struct ctrl_type_desc ctrl_types[];

/*---------------------------------------------------------------------------*
 *	this structure describes one call/connection on one B-channel
 *	and all its parameters
 *---------------------------------------------------------------------------*/
typedef struct
{
	u_int	cdid;			/* call descriptor id		*/
	int	bri;			/* isdn controller number	*/
	int	cr;			/* call reference value		*/

	int	crflag;			/* call reference flag		*/
#define CRF_ORIG	0		/* originating side		*/
#define CRF_DEST	1		/* destinating side		*/

	int	channelid;		/* channel id value		*/
	int	channelexcl;		/* channel exclusive		*/

	int	bprot;			/* B channel protocol BPROT_XXX */

	int	bchan_driver_index;	/* driver to use for B channel	*/
	int	bchan_driver_unit;	/* unit for above driver number	*/
	
	cause_t	cause_in;		/* cause value from NT	*/
	cause_t	cause_out;		/* cause value to NT	*/

	int	call_state;		/* from incoming SETUP	*/
	
	u_char	dst_telno[TELNO_MAX];	/* destination number	*/
	u_char	src_telno[TELNO_MAX];	/* source number	*/

	int	scr_ind;		/* screening ind for incoming call */
	int	prs_ind;		/* presentation ind for incoming call */
	
	int	Q931state;		/* Q.931 state for call	*/
	int	event;			/* event to be processed */

	int	response;		/* setup response type	*/

	int	T303;			/* SETUP sent response timeout	*/
	int	T303_first_to;		/* first timeout flag		*/

	int	T305;			/* DISC without PROG IND	*/

	int	T308;			/* RELEASE sent response timeout*/
	int	T308_first_to;		/* first timeout flag		*/

	int	T309;			/* data link disconnect timeout	*/

	int	T310;			/* CALL PROC received		*/

	int	T313;			/* CONNECT sent timeout		*/ 

	int	T400;			/* L4 timeout */

	isdn_link_t	*ilt;		/* isdn B channel driver/state	*/
	const struct isdn_l4_driver_functions *l4_driver;		/* layer 4 driver		*/
	void	*l4_driver_softc;					/* layer 4 driver instance	*/

	int	dir;			/* outgoing or incoming call	*/
#define DIR_OUTGOING	0
#define DIR_INCOMING	1

	int	timeout_active;		/* idle timeout() active flag	*/

	struct	callout	idle_timeout_handle;
	struct	callout	T303_callout;
	struct	callout	T305_callout;
	struct	callout	T308_callout;
	struct	callout	T309_callout;
	struct	callout	T310_callout;
	struct	callout	T313_callout;
	struct	callout	T400_callout;
	int	callouts_inited;		/* must init before use */

	int	idletime_state;		/* wait for idle_time begin	*/
#define IST_IDLE	0	/* shorthold mode disabled 	*/
#define IST_NONCHK	1	/* in non-checked window	*/
#define IST_CHECK	2	/* in idle check window		*/
#define IST_SAFE	3	/* in safety zone		*/

	time_t	idletimechk_start;	/* check idletime window start	*/
	time_t	connect_time;		/* time connect was made	*/
	time_t	last_active_time;	/* last time with activity	*/

					/* for incoming connections:	*/
	time_t	max_idle_time;		/* max time without activity	*/

					/* for outgoing connections:	*/	
	msg_shorthold_t shorthold_data;	/* shorthold data to use */

	int	aocd_flag;		/* AOCD used for unitlength calc*/
	time_t	last_aocd_time;		/* last time AOCD received	*/
	int	units;			/* number of AOCD charging units*/
	int	units_type;		/* units type: AOCD, AOCE	*/
	int	cunits;			/* calculated units		*/

	int	isdntxdelay;		/* isdn tx delay after connect	*/

	u_char	display[DISPLAY_MAX];	/* display information element	*/
	char	datetime[DATETIME_MAX];	/* date/time information element*/	
} call_desc_t;

extern call_desc_t call_desc[N_CALL_DESC];

/*
 * Set of functions layer 4 drivers calls to manipulate the B channel
 * they are using.
 */
struct isdn_l4_bchannel_functions {
	void (*bch_config)(void*, int channel, int bprot, int updown);
	void (*bch_tx_start)(void*, int channel);
	void (*bch_stat)(void*, int channel, bchan_statistics_t *bsp);	
};

/*
 * Functions a layer 4 application driver exports
 */
struct isdn_l4_driver_functions {
	/*
	 * Functions for use by the B channel driver
	 */
	void (*bch_rx_data_ready)(void *softc);
	void (*bch_tx_queue_empty)(void *softc);
	void (*bch_activity)(void *softc, int rxtx);
#define ACT_RX 0
#define ACT_TX 1
	void (*line_connected)(void *softc, void *cde);
	void (*line_disconnected)(void *softc, void *cde);
	void (*dial_response)(void *softc, int stat, cause_t cause);
	void (*updown_ind)(void *softc, int updown);
	/*
	 * Functions used by the ISDN management system
	 */
	void* (*get_softc)(int unit);
	void (*set_linktab)(void *softc, isdn_link_t *ilt);
	/*
	 * Optional accounting function
	 */
	time_t (*get_idletime)(void* softc);
};

/* global registry of layer 4 drivers */
int isdn_l4_driver_attach(const char *name, int units, const struct isdn_l4_driver_functions *driver);
int isdn_l4_driver_detatch(const char *name);
int isdn_l4_find_driverid(const char *name);
const struct isdn_l4_driver_functions *isdn_l4_find_driver(const char *name, int unit);
const struct isdn_l4_driver_functions *isdn_l4_get_driver(int driver_id, int unit);

/* forward decl. */
struct isdn_diagnostic_request;
struct isdn_dr_prot;

/*---------------------------------------------------------------------------*
 *	this structure "describes" one controller
 *---------------------------------------------------------------------------*/
typedef struct
{
	void*	l1_token;		/* softc of hardware driver	*/
	int	bri;
	int	ctrl_type;		/* controller type   (CTRL_XXX)	*/
	int	card_type;		/* card manufacturer (CARD_XXX) */

	int	protocol;		/* D-channel protocol type */

	int	dl_est;			/* layer 2 established	*/
#define DL_DOWN	0
#define DL_UP	1	

	int	bch_state[2];		/* states of the b channels */
#define BCH_ST_FREE	0	/* free to be used, idle */
#define BCH_ST_RSVD	1	/* reserved, may become free or used */
#define BCH_ST_USED	2	/* in use for data transfer */

	int	tei;			/* current tei or -1 if invalid */

	/* pointers to functions to be called from L4 */
	
	void	(*N_CONNECT_REQUEST)	(unsigned int);	
	void	(*N_CONNECT_RESPONSE)	(unsigned int, int, int);
	void	(*N_DISCONNECT_REQUEST)	(unsigned int, int);
	void	(*N_ALERT_REQUEST)	(unsigned int);
#if 0
	int     (*N_DOWNLOAD)		(void*, int numprotos, struct isdn_dr_prot *protocols);
	int     (*N_DIAGNOSTICS)	(void*, struct isdn_diagnostic_request*);
#endif
	void	(*N_MGMT_COMMAND)	(int bri, int cmd, void *);
} ctrl_desc_t;

extern ctrl_desc_t ctrl_desc[MAX_CONTROLLERS];

#endif /* _I4B_Q931_H_ */
