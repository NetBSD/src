/* $NetBSD: i4b_l1l2.h,v 1.6.2.1 2002/05/30 13:52:36 gehenna Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@netbsd.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _I4B_L1L2_H_
#define _I4B_L1L2_H_

/*
 * This file defines the D-channel interface between layer 1 (physical 
 * card hardware) and layer 2 of a passive ISDN card (i.e. a card that 
 * does not run it's own ISDN stack in firmware).
 *
 * Since each layer 1 driver knows in advance it will need to attach
 * to this layer 2, the whole layer 2 softc struct is typically included
 * in the layer 1 softc.
 *
 * A back-pointer to the layer 1 softc is included (as void*) in this
 * structure.
 */

typedef void * isdn_layer1token;

/*
 * Each driver attaching to layer 2 via this interface provides a pointer
 * to a struct of function pointers used to communicate with layer 1, while
 * layer 1 calls layer 2 functions directly (see below).
 *
 * Layer 1 functions called from layer 2:
 */
struct isdn_layer1_bri_driver {
	/* Request to transmit data. */
	int (*ph_data_req)(isdn_layer1token, struct mbuf *, int);

	/* Request to activate layer 1. */
	int (*ph_activate_req)(isdn_layer1token);

	/* Request to execute an internal command. */
	int (*mph_command_req)(isdn_layer1token, int, void *);
};

/*
 * Layer 2 functions called by layer 1:
 */

/* Process a rx'd frame */
int isdn_layer2_data_ind(struct l2_softc * t, struct isdn_l3_driver *, struct mbuf *m);

/* Pass a layer 1 activation/deactivation to layer 2. */
int isdn_layer2_activate_ind(struct l2_softc *, struct isdn_l3_driver *, int);

/* Pass trace data to layer 2. */
struct i4b_trace_hdr;	/* from i4b_trace.h */
int isdn_layer2_trace_ind(struct l2_softc *, struct isdn_l3_driver *, struct i4b_trace_hdr *, size_t, unsigned char *);

/* Pass status informations to layer 2. */
int isdn_layer2_status_ind(struct l2_softc *, struct isdn_l3_driver *, int, int);
	
#endif /* !_I4B_L1L2_H_ */

