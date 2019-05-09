/*	$NetBSD: ucomvar.h,v 1.23 2019/05/09 02:43:35 mrg Exp $	*/

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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


/* just for ucom_attach_args, not in the config namespace */
#define UCOM_UNK_PORTNO (-1)

struct	ucom_softc;

/*
 * USB detach requires ensuring that outstanding operations and
 * open devices are properly closed before detach can return.
 *
 * ucom parents rely upon ucom(4) itself doing any safety here.
 * The standard method is:
 *
 * 1. device softc has a "bool sc_dying" member, that may be set
 *    in attach or other run-time for general failure, and set
 *    early in the detach callback
 *
 * 2. if sc_dying is set, most functions should perform as close
 *    to zero operations as possible
 *
 * 3. detach callback sets sc_dying to true and then cleans up
 *    any local state and calls config_detach() on each child
 */

/*
 * The first argument to the ucom callbacks is the passed in ucaa_arg
 * member of the attach args, typically the parent softc pointer.
 *
 * All of these are optional.
 */
struct ucom_methods {
	/*
	 * arg2: port number
	 * arg3: pointer to lsr (always non NULL)
	 * arg4: pointer to msr (always non NULL)
	 */
	void (*ucom_get_status)(void *, int, u_char *, u_char *);
	/*
	 * arg2: port number
	 * arg3: value to turn on or off (DTR, RTS, BREAK)
	 * arg4: onoff
	 */
	void (*ucom_set)(void *, int, int, int);
#define UCOM_SET_DTR 1
#define UCOM_SET_RTS 2
#define UCOM_SET_BREAK 3
	/*
	 * arg2: port number
	 * arg3: termios structure to set parameters from
	 */
	int (*ucom_param)(void *, int, struct termios *);
	/*
	 * arg2: port number
	 * arg3: ioctl command
	 * arg4: ioctl data
	 * arg5: ioctl flags
	 * arg6: process calling ioctl
	 */
	int (*ucom_ioctl)(void *, int, u_long, void *, int, proc_t *);
	/* arg2: port number */
	int (*ucom_open)(void *, int);
	/* arg2: port number */
	void (*ucom_close)(void *, int);
	/*
	 * arg2: port number
	 * arg3: pointer to buffer pointer 
	 * arg4: pointer to buffer count
	 *
	 * Note: The 'ptr' (3nd arg) and 'count' (4rd arg) pointers can be
	 * adjusted as follows:
	 *
	 *  ptr:	If consuming characters from the start of the buffer,
	 *		advance '*ptr' to skip the data consumed.
	 *
	 *  count:	If consuming characters at the end of the buffer,
	 *		decrement '*count' by the number of characters
	 *		consumed.
	 *
	 * If consuming all characters, set '*count' to zero.
	 */
	void (*ucom_read)(void *, int, u_char **, uint32_t *);
	/*
	 * arg2: port number
	 * arg3: pointer to source buffer
	 * arg4: pointer to destination buffer
	 * arg5: pointer to buffer count
	 */
	void (*ucom_write)(void *, int, u_char *, u_char *, uint32_t *);
};

/* modem control register */
#define	UMCR_RTS	0x02	/* Request To Send */
#define	UMCR_DTR	0x01	/* Data Terminal Ready */

/* line status register */
#define	ULSR_RCV_FIFO	0x80
#define	ULSR_TSRE	0x40	/* Transmitter empty: byte sent */
#define	ULSR_TXRDY	0x20	/* Transmitter buffer empty */
#define	ULSR_BI		0x10	/* Break detected */
#define	ULSR_FE		0x08	/* Framing error: bad stop bit */
#define	ULSR_PE		0x04	/* Parity error */
#define	ULSR_OE		0x02	/* Overrun, lost incoming byte */
#define	ULSR_RXRDY	0x01	/* Byte ready in Receive Buffer */
#define	ULSR_RCV_MASK	0x1f	/* Mask for incoming data or error */

/* modem status register */
/* All deltas are from the last read of the MSR. */
#define	UMSR_DCD	0x80	/* Current Data Carrier Detect */
#define	UMSR_RI		0x40	/* Current Ring Indicator */
#define	UMSR_DSR	0x20	/* Current Data Set Ready */
#define	UMSR_CTS	0x10	/* Current Clear to Send */
#define	UMSR_DDCD	0x08	/* DCD has changed state */
#define	UMSR_TERI	0x04	/* RI has toggled low to high */
#define	UMSR_DDSR	0x02	/* DSR has changed state */
#define	UMSR_DCTS	0x01	/* CTS has changed state */

struct ucom_attach_args {
	int ucaa_portno;
	int ucaa_bulkin;
	int ucaa_bulkout;
	u_int ucaa_ibufsize;
	u_int ucaa_ibufsizepad;
	u_int ucaa_obufsize;
	u_int ucaa_opkthdrlen;
	const char *ucaa_info;	/* attach message */
	struct usbd_device *ucaa_device;
	struct usbd_interface *ucaa_iface;
	const struct ucom_methods *ucaa_methods;
	void *ucaa_arg;
};

int ucomprint(void *, const char *);
int ucomsubmatch(device_t t, cfdata_t, const int *, void *);
void ucom_status_change(struct ucom_softc *);
