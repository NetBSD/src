/*	$NetBSD: openfirm.c,v 1.5.2.1 2001/10/01 12:37:54 fvdl Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

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

#include <machine/psl.h>
#include <machine/stdarg.h>

#include <dev/ofw/openfirm.h>


/*
 *  Wrapper routines for OFW client services.
 *
 *  This code was adapted from the PowerPC version done by
 *  Wolfgang Solfrank.  The main difference is that we don't
 *  do the silly "ofw_stack" dance to convert the OS's real-
 *  mode view of OFW to virtual-mode.  We don't need to do
 *  that because our NetBSD port assumes virtual-mode OFW.
 *
 *  We should work with Wolfgang to turn this into a MI file. -JJK
 */


int
OF_peer(phandle)
	int phandle;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		int sibling;
	} args = {
		"peer",
		1,
		1,
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
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		int child;
	} args = {
		"child",
		1,
		1,
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
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		int parent;
	} args = {
		"parent",
		1,
		1,
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
		char *name;
		int nargs;
		int nreturns;
		int ihandle;
		int phandle;
	} args = {
		"instance-to-package",
		1,
		1,
	};

	args.ihandle = ihandle;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_nextprop(handle, prop, nextprop)
	int handle;
	char *prop;
	void *nextprop;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		char *prop;
		void *nextprop;
		int flags;
	} args = {
		"nextprop",
		3,
		1,
	};

	args.phandle = handle;
	args.prop = prop;
	args.nextprop = nextprop;

	if (openfirmware(&args) == -1)
		return -1;
	return args.flags;
}

int
OF_getprop(handle, prop, buf, buflen)
	int handle;
	char *prop;
	void *buf;
	int buflen;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		char *prop;
		void *buf;
		int buflen;
		int size;
	} args = {
		"getprop",
		4,
		1,
	};

	args.phandle = handle;
	args.prop = prop;
	args.buf = buf;
	args.buflen = buflen;


	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

int
OF_getproplen(handle, prop)
	int handle;
	char *prop;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		char *prop;
		int size;
	} args = {
		"getproplen",
		2,
		1,
	};

	args.phandle = handle;
	args.prop = prop;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

int
OF_finddevice(name)
	char *name;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		char *device;
		int phandle;
	} args = {
		"finddevice",
		1,
		1,
	};	

	args.device = name;
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
		char *name;
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
		char *name;
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
		char *name;
		int nargs;
		int nreturns;
		char *method;
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
	if (openfirmware(&args) == -1) {
		va_end(ap);
		return -1;
	}
/*
	{
	    int i, res;

	    printf("call_method(%s): ihandle = %x, nargs = %d, nreturns = %d -- ",
		   method, ihandle, nargs, nreturns);
	    res = openfirmware(&args);
	    printf("res = %x\n", res);
	    printf("\targs_n_results = ");
	    for (i = 0; i < nargs + nreturns + 1; i++)
		printf("%x ", args.args_n_results[i]);
	    printf("\n");
	    if (res == -1) return -1;
	}
*/
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
		char *name;
		int nargs;
		int nreturns;
		char *method;
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
	if (openfirmware(&args) == -1)
		return -1;
/*
	{
	    int i, res;

	    printf("call_method_1(%s): ihandle = %x, nargs = %d -- ",
		   method, ihandle, nargs);
	    res = openfirmware(&args);
	    printf("res = %x\n", res);
	    printf("\targs_n_results = ");
	    for (i = 0; i < nargs + 2; i++)
		printf("%x ", args.args_n_results[i]);
	    printf("\n");
	    if (res == -1) return -1;
	}
*/
	if (args.args_n_results[nargs])
		return -1;
	return args.args_n_results[nargs + 1];
}

int
OF_open(dname)
	char *dname;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		char *dname;
		int handle;
	} args = {
		"open",
		1,
		1,
	};
	
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
		char *name;
		int nargs;
		int nreturns;
		int handle;
	} args = {
		"close",
		1,
		0,
	};

	args.handle = handle;
	openfirmware(&args);
}

int
OF_read(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	static struct {
		char *name;
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
	
	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.actual;
}

int
OF_write(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	static struct {
		char *name;
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
	
	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.actual;
}

int
OF_seek(handle, pos)
	int handle;
	u_quad_t pos;
{
	static struct {
		char *name;
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

	args.handle = handle;
	args.poshi = (int)(pos >> 32);
	args.poslo = (int)pos;
	if (openfirmware(&args) == -1)
		return -1;
	return args.status;
}

void *
OF_claim(virt, size, align)
        void *virt;
        u_int size;
        u_int align;
{
        static struct {
                char *name;
                int nargs;
                int nreturns;
                void *virt;
                u_int size;
                u_int align;
                void *baseaddr;
        } args = {
                "claim",
                3,
                1,
        };

        args.virt = virt;
        args.size = size;
        args.align = align;
        if (openfirmware(&args) == -1)
                return (void *)-1;
        return args.baseaddr;
}

void
OF_release(virt, size)
        void *virt;
        u_int size;
{
        static struct {
                char *name;
                int nargs;
                int nreturns;
                void *virt;
                u_int size;
        } args = {
                "release",
                2,
                0,
        };
        
        args.virt = virt;
        args.size = size;
        openfirmware(&args);
}

int
OF_milliseconds()
{
        static struct {
                char *name;
                int nargs;
                int nreturns;
                int ms;
        } args = {
                "milliseconds",
                0,
                1,
        };
        
        openfirmware(&args);
        return args.ms;
}

void
OF_boot(bootspec)
	char *bootspec;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		char *bootspec;
	} args = {
		"boot",
		1,
		0,
	};
	
	args.bootspec = bootspec;
	openfirmware(&args);
	while (1);			/* just in case */
}

void
OF_enter()
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
	} args = {
		"enter",
		0,
		0,
	};

	openfirmware(&args);
}

void
OF_exit()
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
	} args = {
		"exit",
		0,
		0,
	};

	openfirmware(&args);
	while (1);			/* just in case */
}

void
(*OF_set_callback(newfunc))()
	void (*newfunc)();
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		void (*newfunc)();
		void (*oldfunc)();
	} args = {
		"set-callback",
		1,
		1,
	};

	args.newfunc = newfunc;
	if (openfirmware(&args) == -1)
		return 0;
	return args.oldfunc;
}
