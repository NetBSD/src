/*	$NetBSD: evtchn.h,v 1.13.24.1 2008/01/09 01:50:05 matt Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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

#ifndef _XEN_EVENTS_H_
#define _XEN_EVENTS_H_

#define NR_PIRQS	256

extern struct evtsource *evtsource[];

void events_default_setup(void);
void init_events(void);
unsigned int evtchn_do_event(int, struct intrframe *);
void call_evtchn_do_event(int, struct intrframe *);
int event_set_handler(int, int (*func)(void *), void *, int, const char *);
int event_remove_handler(int, int (*func)(void *), void *);

extern int debug_port;
extern int xen_debug_handler(void *);

int bind_virq_to_evtch(int);
int bind_pirq_to_evtch(int);
void unbind_pirq_from_evtch(int);
void unbind_virq_from_evtch(int);

struct pintrhand {
	int pirq;
	int evtch;
	int (*func)(void *);
	void *arg;
};

struct pintrhand *pirq_establish(int, int, int (*)(void *), void *, int,
     const char *);

#endif /*  _XEN_EVENTS_H_ */
