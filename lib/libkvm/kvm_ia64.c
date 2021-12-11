/*	$NetBSD: kvm_ia64.c,v 1.3 2021/12/11 19:24:19 mrg Exp $	*/

/*
 * Copyright (c) 2016 Matthew R. Green
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Empty implementation */

#include <sys/param.h>

#include <limits.h>
#include <kvm.h>

#include <uvm/uvm_extern.h>

#include "kvm_private.h"


void
_kvm_freevtop(kvm_t *kd)
{

}

int
_kvm_initvtop(kvm_t *kd)
{

	_kvm_err(kd, 0, "initvtop not yet implemented!");
	return (0);
}

int
_kvm_kvatop(kvm_t *kd, vaddr_t va, paddr_t *pa)
{

	_kvm_err(kd, 0, "vatop not yet implemented!");
	return -1;
}

off_t
_kvm_pa2off(kvm_t *kd, paddr_t pa)
{

	_kvm_err(kd, 0, "pa2off not yet implemented!");
	return -1;
}

int
_kvm_mdopen(kvm_t *kd)
{

	_kvm_err(kd, 0, "mdopen not yet implemented!");
	return -1;
}
