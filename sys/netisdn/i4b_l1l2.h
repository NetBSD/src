/* $NetBSD: i4b_l1l2.h,v 1.2 2001/03/24 12:40:31 martin Exp $ */

/*
 * Copyright (c) 2001 Martin Husemann. All rights reserved.
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
 */

#ifndef _I4B_L1L2_H_
#define _I4B_L1L2_H_

/*
 * This file defines the interface between layer 1 (physical card hardware)
 * and layer 2 of a passive ISDN card (i.e. a card that does not run it's
 * own ISDN stack in firmware).
 *
 * The layer 2 is represented in all functions by an opaque layer2token type,
 * internally representing the layer 2 struct softc (which is unknown to
 * layer 1).
 * Similarily layer 1 is represented by a layer1token type, typically
 * representing its struct device derived softc pointer. If someone ever
 * creates a multi-bri passive isdn card, it would need to be something
 * else (i.e. pointer to a struct containing the softc pointer and a
 * interface index), so we do not define it as 'struct device *' here.
 */

typedef void * isdn_layer2token;
typedef void * isdn_layer1token;

/*
 * Each driver attaching to layer 2 via this interface provides a pointer
 * to a struct of function pointers used to communicate with layer 1, while
 * layer 1 calls layer 2 functions directly (see below).
 *
 * Layer 1 functions called from layer 2:
 */
struct isdn_layer1_bri_driver {
	/* Activate card and enable interrupts or disable it. */
	int (*enable)(isdn_layer1token, int);

	/* Request to transmit data. */
	int (*ph_data_req)(isdn_layer1token, struct mbuf *, int);

	/* Request to activate layer 1. */
	int (*ph_activate_req)(isdn_layer1token);

	/* Request to execute an internal command. */
	int (*mph_command_req)(isdn_layer1token, int, void *);

	/* switch on/off trace */
	void (*n_mgmt_command)(isdn_layer1token, int cmd, void *);
};

/*
 * Support functions called from upper layers
 */
isdn_layer2token isdn_find_l2_by_bri(int);

/*
 * Layer 2 functions called by layer 1:
 */

/* Attach a new BRI and return it's layer 2 token. */
isdn_layer2token isdn_attach_layer1_bri(isdn_layer1token,
					const char *, const char *,
					const struct isdn_layer1_bri_driver *);

/* Detach a BRI. */
int isdn_detach_layer1_bri(isdn_layer2token);

/* Pass data up to layer 2. */
int isdn_layer2_data_ind(isdn_layer2token, struct mbuf *);

/* Pass a layer 1 activation/deactivation to layer 2. */
int isdn_layer2_activate_ind(isdn_layer2token, int);

/* Pass trace data to layer 2. */
struct i4b_trace_hdr;	/* from i4b_trace.h */
int isdn_layer2_trace_ind(isdn_layer2token, struct i4b_trace_hdr *, size_t, unsigned char *);

/* Pass status informations to layer 2. */
int isdn_layer2_status_ind(isdn_layer2token, int, int);
	
#endif /* !_I4B_L1L2_H_ */

