/*-
 * Copyright (c) 2006 Michael Lorenz
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cudavar.h,v 1.2.44.1 2017/12/03 11:36:24 jdolecek Exp $");

#ifndef CUDAVAR_H
#define CUDAVAR_H

/* Cuda addresses */
#define CUDA_ADB	0
#define CUDA_PSEUDO	1
#define CUDA_ERROR	2	/* error codes? */
#define CUDA_TIMER	3
#define CUDA_POWER	4
#define CUDA_IIC	5	/* XXX ??? */
#define CUDA_PMU	6
#define CUDA_ADB_QUERY	7

/* Cuda commands */
#define CMD_AUTOPOLL	1
#define CMD_READ_RTC	3
#define CMD_READ_PRAM	7	/* addr is 16bit, upper byte first */
#define CMD_WRITE_RTC	9
#define CMD_POWEROFF	10
#define CMD_WRITE_PRAM	12
#define CMD_RESET	17
#define CMD_IIC		34

struct cuda_attach_args {
	void *cookie;
	int (*send)(void *, int, int, uint8_t *);	/* send a message */
	void (*poll)(void *);		/* poll until the chip is idle */
	int (*set_handler)(void *, int, int (*)(void *, int, uint8_t *), void *);
};

void cuda_poweroff(void);
void cuda_restart(void);

#endif /* CUDAVAR_H */
