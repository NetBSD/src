/*	$NetBSD: uvm_pdpolicy.h,v 1.9 2022/08/20 23:26:02 riastradh Exp $	*/

/*-
 * Copyright (c)2005, 2006 YAMAMOTO Takashi,
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
 */

#ifndef _UVM_PDPOLICY_H_
#define _UVM_PDPOLICY_H_

#include <sys/mutex.h>
#include <sys/stdint.h>

#include <uvm/uvm_page.h>

struct krwlock;
struct uvm_cpu;
struct vm_anon;
struct vm_page;

/*
 * these API is for uvm internal use only.
 * don't use them directly from outside of /sys/uvm.
 */

void uvmpdpol_idle(struct uvm_cpu *);
void uvmpdpol_init(void);
void uvmpdpol_init_cpu(struct uvm_cpu *);
void uvmpdpol_reinit(void);
void uvmpdpol_estimatepageable(int *, int *);
bool uvmpdpol_needsscan_p(void);

void uvmpdpol_pageactivate(struct vm_page *);
void uvmpdpol_pagedeactivate(struct vm_page *);
void uvmpdpol_pagedequeue(struct vm_page *);
void uvmpdpol_pageenqueue(struct vm_page *);
bool uvmpdpol_pageactivate_p(struct vm_page *);
bool uvmpdpol_pageisqueued_p(struct vm_page *);
void uvmpdpol_pagerealize(struct vm_page *);
void uvmpdpol_anfree(struct vm_anon *);

void uvmpdpol_tune(void);
void uvmpdpol_scaninit(void);
void uvmpdpol_scanfini(void);
struct vm_page *uvmpdpol_selectvictim(struct krwlock **);
void uvmpdpol_balancequeue(int);

void uvmpdpol_sysctlsetup(void);

/*
 * uvmpdpol_set_intent: set an intended state for the page, taking care not
 * to overwrite any of the other flags.
 */

static inline void
uvmpdpol_set_intent(struct vm_page *pg, uint32_t i)
{

	KASSERT(mutex_owned(&pg->interlock));
	pg->pqflags = PQ_INTENT_SET | (pg->pqflags & ~PQ_INTENT_MASK) | i;
}

#endif /* !_UVM_PDPOLICY_H_ */
