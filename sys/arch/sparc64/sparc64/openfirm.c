/*	$NetBSD: openfirm.c,v 1.2.2.1 1998/07/30 14:03:56 eeh Exp $	*/

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
#include <sys/param.h>
#include <sys/systm.h>

#include <machine/psl.h>
#include <machine/stdarg.h>

#include <machine/openfirm.h>

#define min(x,y)	((x<y)?(x):(y))

/* Prolly never used */
void ofbcopy __P((const void *src, void *dst, size_t len));


int
OF_peer(phandle)
	int phandle;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int phandle;
		int pad2; int sibling;
	} args = {
		0, "peer",
		1,
		1,
		0, 0,
		0, 0
	};
	
	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.sibling;
}

int
OF_child(phandle)
	int phandle;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int phandle;
		int pad2; int child;
	} args = {
		0, "child",
		1,
		1,
		0, 0,
		0, 0
	};
	
	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.child;
}

int
OF_parent(phandle)
	int phandle;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int phandle;
		int pad2; int parent;
	} args = {
		0, "parent",
		1,
		1,
		0, 0,
		0, 0
	};
	
	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.parent;
}

int
OF_instance_to_package(ihandle)
	int ihandle;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int ihandle;
		int pad2; int phandle;
	} args = {
		0, "instance-to-package",
		1,
		1,
		0, 0,
		0, 0
	};

	args.ihandle = ihandle;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_getproplen(handle, prop)
	int handle;
	char *prop;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int phandle;
		int pad2; char *prop;
		int64_t size;
	} args = {
		0, "getproplen",
		2,
		1,
		0, 0,
		0, NULL
	};

	args.phandle = handle;
	args.prop = prop;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

int
OF_getprop(handle, prop, buf, buflen)
	int handle;
	char *prop;
	void *buf;
	int buflen;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int phandle;
		int pad2; char *prop;
		int pad3; void *buf;
		int64_t buflen;
		int64_t size;
	} args = {
		0, "getprop",
		4,
		1,
		0, 0,
		0, NULL,
		0, NULL
	};

	if (buflen > NBPG)
		return -1;
	args.phandle = handle;
	args.prop = prop;
	args.buf = buf;
	args.buflen = buflen;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

int
OF_finddevice(name)
char *name;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *device;
		int pad2; int phandle;
	} args = {
		0, "finddevice",
		1,
		1,
		0, NULL,
		0, 0
	};	

	args.device = name;
	args.phandle = -1;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_instance_to_path(ihandle, buf, buflen)
	int ihandle;
	char *buf;
	int buflen;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int ihandle;
		int pad2; char *buf;
		int64_t buflen;
		int64_t length;
	} args = {
		0,"instance-to-path",
		3,
		1,
		0, 0,
		0, NULL
	};

	if (buflen > NBPG)
		return -1;
	args.ihandle = ihandle;
	args.buf = buf;
	args.buflen = buflen;
	if (openfirmware(&args) < 0)
		return -1;
	return args.length;
}

int
OF_package_to_path(phandle, buf, buflen)
	int phandle;
	char *buf;
	int buflen;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int phandle;
		int pad2; char *buf;
		int64_t buflen;
		int64_t length;
	} args = {
		0,"package-to-path",
		3,
		1,
		0, 0,
		0, NULL
	};
	
	if (buflen > NBPG)
		return -1;
	args.phandle = phandle;
	args.buf = buf;
	args.buflen = buflen;
	if (openfirmware(&args) < 0)
		return -1;
	return args.length;
}

int
#ifdef	__STDC__
OF_call_method(char *method, int ihandle, int nargs, int nreturns, ...)
#else
OF_call_method(method, ihandle, nargs, nreturns, va_alist)
	char *method;
	int ihandle;
	int nargs;
	int nreturns;
	va_dcl
#endif
{
	va_list ap;
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t args_n_results[12];
	} args = {
		0, "call-method",
		2,
		1,
		0, NULL,
		0, 0
	};
	int *ip, n;
	
	if (nargs > 6)
		return -1;
	args.nargs = nargs + 2;
	args.nreturns = nreturns + 1;
	args.method = method;
	args.ihandle = ihandle;
	va_start(ap, nreturns);
	for (ip = (int*)(args.args_n_results + (n = nargs)); --n >= 0;)
		*--ip = va_arg(ap, int);
	if (openfirmware(&args) == -1)
		return -1;
	if (args.args_n_results[nargs])
		return args.args_n_results[nargs];
	for (ip = (int*)(args.args_n_results + nargs + (n = args.nreturns)); --n > 0;)
		*va_arg(ap, int *) = *--ip;
	va_end(ap);
	return 0;
}

int
#ifdef	__STDC__
OF_call_method_1(char *method, int ihandle, int nargs, ...)
#else
OF_call_method_1(method, ihandle, nargs, va_alist)
	char *method;
	int ihandle;
	int nargs;
	va_dcl
#endif
{
	va_list ap;
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t args_n_results[16];
	} args = {
		0, "call-method",
		2,
		1,
		0, NULL,
		0, 0
	};
	int *ip, n;
	
	if (nargs > 6)
		return -1;
	args.nargs = nargs + 2;
	args.method = method;
	args.ihandle = ihandle;
	va_start(ap, nargs);
	for (ip = (int*)(args.args_n_results + (n = nargs)); --n >= 0;)
		*--ip = va_arg(ap, int);
	va_end(ap);
	if (openfirmware(&args) == -1)
		return -1;
	if (args.args_n_results[nargs])
		return -1;
	return args.args_n_results[nargs + 1];
}

int
OF_open(dname)
	char *dname;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *dname;
		int pad2; int handle;
	} args = {
		0, "open",
		1,
		1,
		0, NULL,
		0, 0
	};
	int l;
	
	if ((l = strlen(dname)) >= NBPG)
		return -1;
	args.dname = dname;
	if (openfirmware(&args) == -1)
		return -1;
	return args.handle;
}

void
OF_close(handle)
	int handle;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int handle;
	} args = {
		0, "close",
		1,
		0,
		0, 0
	};

	args.handle = handle;
	openfirmware(&args);
}

int
OF_test(service)
	char* service;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *service;
		int64_t status;
	} args = {
		0, "test",
		1,
		1,
		0, NULL
	};

	args.service = service;
	if (openfirmware(&args) == -1)
		return -1;
	return args.status;
}

int
OF_test_method(service, method)
	int service;
	char* method;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int service;
		int pad2; char *method;
		int64_t status;
	} args = {
		0, "test-method",
		2,
		1,
		0, 0,
		0, 0,
		-1
	};

	args.service = service;
	args.method = method;
	openfirmware(&args);
	return args.status;
}
  
    
/* 
 * This assumes that character devices don't read in multiples of NBPG.
 */
int
OF_read(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int ihandle;
		int pad2; void *addr;
		int64_t len;
		int64_t actual;
	} args = {
		0, "read",
		3,
		1,
		0, 0,
		0, NULL
	};
	int l, act = 0;
	
	args.ihandle = handle;
	args.addr = addr;
	for (; len > 0; len -= l, addr += l) {
		l = min(NBPG, len);
		args.len = l;
		if (openfirmware(&args) == -1)
			return -1;
		if (args.actual > 0) {
			act += args.actual;
		}
		if (args.actual < l)
			if (act)
				return act;
			else
				return args.actual;
	}
	return act;
}

int
OF_write(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int ihandle;
		int pad2; void *addr;
		int64_t len;
		int64_t actual;
	} args = {
		0, "write",
		3,
		1,
		0, 0,
		0, NULL
	};
	int l, act = 0;
	
	args.ihandle = handle;
	args.addr = addr;
	for (; len > 0; len -= l, addr += l) {
		l = min(NBPG, len);
		args.len = l;
		if (openfirmware(&args) == -1)
			return -1;
		l = args.actual;
		act += l;
	}
	return act;
}


int
OF_seek(handle, pos)
	int handle;
	u_quad_t pos;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; int handle;
		u_int64_t poshi;
		u_int64_t poslo;
		int64_t status;
	} args = {
		0, "seek",
		3,
		1,
		0, 0
	};

	args.handle = handle;
	args.poshi = (unsigned int)(pos >> 32);
	args.poslo = (unsigned int)pos;
	if (openfirmware(&args) == -1)
		return -1;
	return args.status;
}

void
OF_boot(bootspec)
	char *bootspec;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *bootspec;
	} args = {
		0, "boot",
		1,
		0,
		0, NULL
	};
	int l;
	
	if ((l = strlen(bootspec)) >= NBPG)
		panic("OF_boot");
	args.bootspec = bootspec;
	openfirmware(&args);
	panic("OF_boot failed");
}

void
OF_enter()
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
	} args = {
		0, "enter",
		0,
		0,
	};

	openfirmware(&args);
}

void
OF_exit()
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
	} args = {
		0, "exit",
		0,
		0,
	};

	openfirmware(&args);
	panic("OF_exit failed");
}

void
OF_poweroff()
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
	} args = {
		0, "SUNW,power-off",
		0,
		0,
	};

	openfirmware(&args);
	panic("OF_poweroff failed");
}

void
(*OF_set_callback(newfunc))()
	void (*newfunc)();
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; void (*newfunc)();
		int pad2; void (*oldfunc)();
	} args = {
		0, "set-callback",
		1,
		1,
		0, NULL,
		0, NULL,
	};

	args.newfunc = newfunc;
	if (openfirmware(&args) == -1)
		return 0;
	return args.oldfunc;
}

/*
 * This version of bcopy doesn't work for overlapping regions!
 */
void
ofbcopy(src, dst, len)
	const void *src;
	void *dst;
	size_t len;
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
