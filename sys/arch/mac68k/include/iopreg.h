/*	$NetBSD: iopreg.h,v 1.4 2000/07/30 21:48:54 briggs Exp $	*/

/*
 * Copyright (c) 2000 Allen Briggs.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#define IOP1_BASE	0x00004000

#define SCC_IOP		0
#define ISM_IOP		1

#define IOP_CS_BYPASS	0x01
#define IOP_CS_AUTOINC	0x02
#define IOP_CS_RUN	0x04
#define IOP_CS_IRQ	0x08
#define IOP_CS_INT0	0x10
#define IOP_CS_INT1	0x20
#define IOP_CS_HWINT	0x40
#define IOP_CS_DMAINACT	0x80

#define IOP_RESET	(IOP_CS_DMAINACT | IOP_CS_AUTOINC)
#define IOP_BYPASS	\
		(IOP_CS_BYPASS | IOP_CS_AUTOINC | IOP_CS_RUN | IOP_CS_DMAINACT)
#define IOP_INTERRUPT	(IOP_CS_INT0 | IOP_CS_INT1)

#define OSS_INTLEVEL_OFFSET	0x0001A006

typedef struct {
	volatile u_char	ram_hi;
	u_char		pad0;
	volatile u_char	ram_lo;
	u_char		pad1;
	volatile u_char	control_status;
	u_char		pad2[3];
	volatile u_char	data;
	u_char		pad3[23];
	union {
		struct {
			volatile u_char sccb_cmd;
			u_char		pad0;
			volatile u_char scca_cmd;
			u_char		pad1;
			volatile u_char sccb_data;
			u_char		pad2;
			volatile u_char scca_data;
			u_char		pad3;
		} scc;
		struct {
			volatile u_char wdata;
			u_char		pad0;
			/* etc... */
		} iwm;
	} bypass;
} IOPHW;

#define	IOP_MAXCHAN	7
#define	IOP_MAXMSG	8
#define IOP_MSGLEN	32
#define IOP_MSGBUFLEN	(IOP_MSGLEN * IOP_MAXCHAN)

#define IOP_MSG_IDLE		0	/* idle 			*/
#define IOP_MSG_NEW		1	/* new message sent		*/
#define IOP_MSG_RECEIVED	2	/* message received; processing	*/
#define IOP_MSG_COMPLETE	3	/* message processing complete	*/

#define IOP_ADDR_MAX_SEND_CHAN	0x200
#define IOP_ADDR_SEND_STATE	0x201
#define IOP_ADDR_PATCH_CTRL	0x21F
#define IOP_ADDR_SEND_MSG	0x220
#define IOP_ADDR_MAX_RECV_CHAN	0x300
#define IOP_ADDR_RECV_STATE	0x301
#define IOP_ADDR_ALIVE		0x31F
#define IOP_ADDR_RECV_MSG	0x320

typedef struct {
	u_char	pad1[0x200];
	u_char	max_send_chan;		 /* maximum send channel #	*/
	u_char	send_state[IOP_MAXCHAN]; /* send channel states		*/
	u_char	pad2[23];
	u_char	patch_ctrl;		 /* patch control flag		*/
	u_char	send_msg[IOP_MSGBUFLEN]; /* send channel message data	*/
	u_char	max_recv_chan;		 /* max. receive channel #	*/
	u_char	recv_state[IOP_MAXCHAN]; /* receive channel states	*/
	u_char	pad3[23];
	u_char	alive;			 /* IOP alive flag		*/
	u_char	recv_msg[IOP_MSGBUFLEN]; /* receive channel msg data	*/
} IOPK;

struct iop_msg;
struct _s_IOP;

typedef	void	(*iop_msg_handler)(struct _s_IOP *iop, struct iop_msg *);

struct iop_msg {
	SIMPLEQ_ENTRY(iop_msg)	iopm;
	int			channel;
	int			status;
	u_char			msg[IOP_MSGLEN];

	/* The routine that will handle the message */
	iop_msg_handler		handler;
	void			*user_data;
};

#define IOP_MSGSTAT_IDLE		0	/* Message unused (invalid) */
#define IOP_MSGSTAT_QUEUED		1	/* Message queued for send */
#define IOP_MSGSTAT_SENDING		2	/* Message on IOP */
#define IOP_MSGSTAT_SENT		3	/* Message complete */
#define IOP_MSGSTAT_RECEIVING		4	/* Top of receive queue */
#define IOP_MSGSTAT_RECEIVED		5	/* Msg received */
#define IOP_MSGSTAT_UNEXPECTED		6	/* Unexpected msg received */

typedef struct _s_IOP {
	IOPHW			*iop;
	struct pool		pool;
	SIMPLEQ_HEAD(, iop_msg)	sendq[IOP_MAXCHAN];
	SIMPLEQ_HEAD(, iop_msg)	recvq[IOP_MAXCHAN];
	iop_msg_handler		listeners[IOP_MAXCHAN];
	void			*listener_data[IOP_MAXCHAN];
	struct iop_msg		unsolicited_msg;
} IOP;

#define	IOP_LOADADDR(ioph,addr)	(ioph->ram_lo = addr & 0xff, \
				 ioph->ram_hi = (addr >> 8) & 0xff)

void	iop_init __P((int fullinit));
void	iop_upload __P((int iop, u_char *mem, u_long nb, u_long iopbase));
void	iop_download __P((int iop, u_char *mem, u_long nb, u_long iopbase));
int	iop_send_msg __P((int iopn, int chan, u_char *msg, int msglen,
			 iop_msg_handler handler, void *udata));
int	iop_queue_receipt __P((int iopn, int chan, iop_msg_handler handler,
				void *user_data));
int	iop_register_listener __P((int iopn, int chan, iop_msg_handler handler,
				void *user_data));

/* ADB support */
#define IOP_CHAN_ADB	2

#define IOP_ADB_FL_EXPLICIT	0x80	/* Non-zero if explicit command */
#define IOP_ADB_FL_AUTOPOLL	0x40	/* Auto/SRQ polling enabled     */
#define IOP_ADB_FL_SRQ		0x04	/* SRQ detected                 */
#define IOP_ADB_FL_TIMEOUT	0x02	/* Non-zero if timeout          */

/*
 * The structure of an ADB packet to/from the IOP is:
 *	Flag byte (values above)
 *	Count of bytes in data
 *	Command byte
 *	Data (optional)
 */
