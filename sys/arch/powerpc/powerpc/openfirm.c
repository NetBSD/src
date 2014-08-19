/*	$NetBSD: openfirm.c,v 1.21.12.2 2014/08/20 00:03:20 tls Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: openfirm.c,v 1.21.12.2 2014/08/20 00:03:20 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/psl.h>

#include <dev/ofw/openfirm.h>

char *OF_buf;

void ofw_stack(void);
void ofbcopy(const void *, void *, size_t);
#ifdef MULTIPROCESSOR
void OF_start_cpu(int, u_int, int);
#endif

int
OF_peer(int phandle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		int sibling;
	} args = {
		"peer",
		1,
		1,
	};

	ofw_stack();
	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.sibling;
}

int
OF_child(int phandle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		int child;
	} args = {
		"child",
		1,
		1,
	};

	ofw_stack();
	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.child;
}

int
OF_parent(int phandle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		int parent;
	} args = {
		"parent",
		1,
		1,
	};

	ofw_stack();
	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.parent;
}

int
OF_instance_to_package(int ihandle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int ihandle;
		int phandle;
	} args = {
		"instance-to-package",
		1,
		1,
	};

	ofw_stack();
	args.ihandle = ihandle;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_getproplen(int handle, const char *prop)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		const char *prop;
		int proplen;
	} args = {
		"getproplen",
		2,
		1,
	};

	ofw_stack();
	args.phandle = handle;
	args.prop = prop;
	if (openfirmware(&args) == -1)
		return -1;
	return args.proplen;
}

int
OF_getprop(int handle, const char *prop, void *buf, int buflen)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		const char *prop;
		void *buf;
		int buflen;
		int size;
	} args = {
		"getprop",
		4,
		1,
	};

	ofw_stack();
	if (buflen > PAGE_SIZE)
		return -1;
	args.phandle = handle;
	args.prop = prop;
	args.buf = OF_buf;
	args.buflen = buflen;
	if (openfirmware(&args) == -1)
		return -1;
	if (args.size > buflen)
		args.size = buflen;
	if (args.size > 0)
		ofbcopy(OF_buf, buf, args.size);
	return args.size;
}

int
OF_setprop(int handle, const char *prop, const void *buf, int buflen)
{
	struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		const char *prop;
		const void *buf;
		int buflen;
		int size;
	} args = {
		"setprop", 
		4,
		1
	};
	ofw_stack();

	if (buflen > NBPG)
		return -1;

	ofbcopy(buf, OF_buf, buflen);
	args.phandle = handle;
	args.prop = prop;
	args.buf = OF_buf;
	args.buflen = buflen;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

int
OF_nextprop(int handle, const char *prop, void *nextprop)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		const char *prop;
		char *buf;
		int flag;
	} args = {
		"nextprop",
		3,
		1,
	};

	ofw_stack();
	args.phandle = handle;
	args.prop = prop;
	args.buf = OF_buf;
	if (openfirmware(&args) == -1)
		return -1;
	strncpy(nextprop, OF_buf, 32);
	return args.flag;
}

int
OF_finddevice(const char *name)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		const char *device;
		int phandle;
	} args = {
		"finddevice",
		1,
		1,
	};

	ofw_stack();
	args.device = name;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_instance_to_path(int ihandle, char *buf, int buflen)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int ihandle;
		char *buf;
		int buflen;
		int length;
	} args = {
		"instance-to-path",
		3,
		1,
	};

	if (buflen > PAGE_SIZE)
		return -1;
	args.ihandle = ihandle;
	args.buf = OF_buf;
	args.buflen = buflen;
	if (openfirmware(&args) < 0)
		return -1;
	if (args.length > buflen)
		args.length = buflen;
	if (args.length > 0)
		ofbcopy(OF_buf, buf, args.length);
	return args.length;
}

int
OF_package_to_path(int phandle, char *buf, int buflen)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		char *buf;
		int buflen;
		int length;
	} args = {
		"package-to-path",
		3,
		1,
	};

	ofw_stack();
	if (buflen > PAGE_SIZE)
		return -1;
	args.phandle = phandle;
	args.buf = OF_buf;
	args.buflen = buflen;
	if (openfirmware(&args) < 0)
		return -1;
	if (args.length > buflen)
		args.length = buflen;
	if (args.length > 0)
		ofbcopy(OF_buf, buf, args.length);
	return args.length;
}

int
OF_call_method(const char *method, int ihandle, int nargs, int nreturns, ...)
{
	va_list ap;
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		const char *method;
		int ihandle;
		int args_n_results[12];
	} args = {
		"call-method",
		2,
		1,
	};
	int *ip, n;

	if (nargs > 6)
		return -1;
	args.nargs = nargs + 2;
	args.nreturns = nreturns + 1;
	args.method = method;
	args.ihandle = ihandle;
	va_start(ap, nreturns);
	for (ip = args.args_n_results + (n = nargs); --n >= 0;)
		*--ip = va_arg(ap, int);
	ofw_stack();
	if (openfirmware(&args) == -1) {
		va_end(ap);
		return -1;
	}
	if (args.args_n_results[nargs]) {
		va_end(ap);
		return args.args_n_results[nargs];
	}
	for (ip = args.args_n_results + nargs + (n = args.nreturns); --n > 0;)
		*va_arg(ap, int *) = *--ip;
	va_end(ap);
	return 0;
}

int
OF_call_method_1(const char *method, int ihandle, int nargs, ...)
{
	va_list ap;
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		const char *method;
		int ihandle;
		int args_n_results[8];
	} args = {
		"call-method",
		2,
		2,
	};
	int *ip, n;

	if (nargs > 6)
		return -1;
	args.nargs = nargs + 2;
	args.method = method;
	args.ihandle = ihandle;
	va_start(ap, nargs);
	for (ip = args.args_n_results + (n = nargs); --n >= 0;)
		*--ip = va_arg(ap, int);
	va_end(ap);
	ofw_stack();
	if (openfirmware(&args) == -1)
		return -1;
	if (args.args_n_results[nargs])
		return -1;
	return args.args_n_results[nargs + 1];
}

int
OF_open(const char *dname)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		const char *dname;
		int handle;
	} args = {
		"open",
		1,
		1,
	};
	int l;

	ofw_stack();
	if ((l = strlen(dname)) >= PAGE_SIZE)
		return -1;
	ofbcopy(dname, OF_buf, l + 1);
	args.dname = OF_buf;
	if (openfirmware(&args) == -1)
		return -1;
	return args.handle;
}

void
OF_close(int handle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int handle;
	} args = {
		"close",
		1,
		0,
	};

	ofw_stack();
	args.handle = handle;
	openfirmware(&args);
}

/*
 * This assumes that character devices don't read in multiples of PAGE_SIZE.
 */
int
OF_read(int handle, void *addr, int len)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int ihandle;
		void *addr;
		int len;
		int actual;
	} args = {
		"read",
		3,
		1,
	};
	int l, act = 0;
	char *p = addr;

	ofw_stack();
	args.ihandle = handle;
	args.addr = OF_buf;
	for (; len > 0; len -= l, p += l) {
		l = min(PAGE_SIZE, len);
		args.len = l;
		if (openfirmware(&args) == -1)
			return -1;
		if (args.actual > 0) {
			ofbcopy(OF_buf, p, args.actual);
			act += args.actual;
		}
		if (args.actual < l) {
			if (act)
				return act;
			else
				return args.actual;
		}
	}
	return act;
}

int
OF_write(int handle, const void *addr, int len)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int ihandle;
		void *addr;
		int len;
		int actual;
	} args = {
		"write",
		3,
		1,
	};
	int l, act = 0;
	const char *p = addr;

	ofw_stack();
	args.ihandle = handle;
	args.addr = OF_buf;
	for (; len > 0; len -= l, p += l) {
		l = min(PAGE_SIZE, len);
		ofbcopy(p, OF_buf, l);
		args.len = l;
		args.actual = l;	/* work around a PIBS bug */
		if (openfirmware(&args) == -1)
			return -1;
		l = args.actual;
		act += l;
	}
	return act;
}

int
OF_seek(int handle, u_quad_t pos)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int handle;
		int poshi;
		int poslo;
		int status;
	} args = {
		"seek",
		3,
		1,
	};

	ofw_stack();
	args.handle = handle;
	args.poshi = (int)(pos >> 32);
	args.poslo = (int)pos;
	if (openfirmware(&args) == -1)
		return -1;
	return args.status;
}

#ifdef MULTIPROCESSOR
void
OF_start_cpu(int phandle, u_int pc, int arg)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		u_int pc;
		int arg;
	} args = {
		"start-cpu",
		3,
		0,
	};
	ofw_stack();
	args.phandle = phandle;
	args.pc = pc;
	args.arg = arg;
	if (openfirmware(&args) == -1)
		panic("WTF?");
}
#endif

void
OF_boot(const char *bootspec)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		char *bootspec;
	} args = {
		"boot",
		1,
		0,
	};
	int l;

	if ((l = strlen(bootspec)) >= PAGE_SIZE)
		panic("OF_boot");
	ofw_stack();
	ofbcopy(bootspec, OF_buf, l + 1);
	args.bootspec = OF_buf;
	openfirmware(&args);
	panic("OF_boot didn't");
}

void
OF_enter(void)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
	} args = {
		"enter",
		0,
		0,
	};

	ofw_stack();
	openfirmware(&args);
}

void
OF_exit(void)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
	} args = {
		"exit",
		0,
		0,
	};

	ofw_stack();
	openfirmware(&args);
	while (1);			/* just in case */
}

void
(*OF_set_callback (void (*newfunc)(void *))) (void *)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		void (*newfunc)(void *);
		void (*oldfunc)(void *);
	} args = {
		"set-callback",
		1,
		1,
	};

	ofw_stack();
	args.newfunc = newfunc;
	if (openfirmware(&args) == -1)
		return 0;
	return args.oldfunc;
}

int
OF_interpret(const char *cmd, int nargs, int nreturns, ...)
{
	va_list ap;
	int i, len, status;
	static struct {
		const char *name;
		uint32_t nargs;
		uint32_t nreturns;
		uint32_t slots[16];
	} args = {
		"interpret",
		1,
		2,
	};

	ofw_stack();
	if (nreturns > 8)
		return -1;
	if ((len = strlen(cmd)) >= PAGE_SIZE)
		return -1;
	ofbcopy(cmd, OF_buf, len + 1);
	i = 0;
	args.slots[i] = (uintptr_t)OF_buf;
	args.nargs = nargs + 1;
	args.nreturns = nreturns + 1;
	va_start(ap, nreturns);
	i++;
	while (i < args.nargs) {
		args.slots[i] = (uintptr_t)va_arg(ap, uint32_t *);
		i++;
	}

	if (openfirmware(&args) == -1)
		return -1;
	status = args.slots[i];
	i++;

	while (i < args.nargs + args.nreturns) {
		*va_arg(ap, uint32_t *) = args.slots[i];
		i++;
	}
	va_end(ap);
	return status;
}

void
OF_quiesce(void)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
	} args = {
		"quiesce",
		0,
		0,
	};

	ofw_stack();
	openfirmware(&args);
}

/*
 * This version of bcopy doesn't work for overlapping regions!
 */
void
ofbcopy(const void *src, void *dst, size_t len)
{
	const char *sp = src;
	char *dp = dst;

	if (src == dst)
		return;

	/*
	 * Do some optimization?						XXX
	 */
	while (len-- > 0)
		*dp++ = *sp++;
}
