/*	$NetBSD: xenio.h,v 1.1 2004/05/07 15:51:04 cl Exp $	*/

/*
 *
 * Copyright (c) 2003, 2004 Keir Fraser (on behalf of the Xen team)
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef __SYS_XENIO_H__
#define __SYS_XENIO_H__

typedef struct privcmd_hypercall {
	unsigned long	op;
	unsigned long	arg[5];
} privcmd_hypercall_t;

typedef struct privcmd_blkmsg {
	unsigned long	op;
	void		*buf;
	int		buf_size;
} privcmd_blkmsg_t;

#define IOCTL_PRIVCMD_HYPERCALL \
	_IOC(IOC_IN, 'P', 0, sizeof(privcmd_hypercall_t))
#define IOCTL_PRIVCMD_BLKMSG \
	_IOC(IOC_VOID, 'P', 1, sizeof(privcmd_blkmsg_t))

#endif /* __SYS_XENIO_H__ */
