/*	$NetBSD: dtfcons.c,v 1.1 2002/07/05 13:31:52 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TTY-like device for use under SH5 simulator/debugger such that
 * console I/O is directed to the host.
 *
 * Well, that's the theory anyway. The "Posix" interface is isn't up
 * to the job and I can't find documentation for any other interface
 * hiding behind the host's DTF code.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/tty.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <sh5/dev/dtfconsvar.h>

#include <sh5/sh5/dtf_comms.h>
#include <sh5/sh5/mainbus.h>

#include <dev/cons.h>


cdev_decl(dtfcons);
cons_decl(dtfcons);


static int dtfconsmatch(struct device *, struct cfdata *, void *);
static void dtfconsattach(struct device *, struct device *, void *);

struct cfattach dtfcons_ca = {
	sizeof(struct device), dtfconsmatch, dtfconsattach
};
extern struct cfdriver dtfcons_cd;

static int dtf_is_console;

static int
dtfconsmatch(struct device *parent, struct cfdata *cf, void *args)
{
        struct mainbus_attach_args *ma = args;

	if (dtf_is_console == 0)
		return (0);

	return (strcmp(ma->ma_name, dtfcons_cd.cd_name) == 0);
}

static void
dtfconsattach(struct device *parent, struct device *self, void *args)
{
#if 0
        struct mainbus_attach_args *ma = args;
#endif
	printf(": DTF Console\n");
}

int
dtfconsopen(dev_t dev, int flag, int mode, struct proc *p)
{
	return (0);
}

int
dtfconsclose(dev_t dev, int flag, int mode, struct proc *p)
{
	return (0);
}

int
dtfconsread(dev_t dev, struct uio *uio, int flag)
{
	return (EIO);
}

int
dtfconswrite(dev_t dev, struct uio *uio, int flag)
{
	return (EIO);
}

int
dtfconspoll(dev_t dev, int events, struct proc *p)
{
	return (EIO);
}

struct tty *
dtfconstty(dev_t dev)
{
	return (NULL);
}

int
dtfconsioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	return (ENOTTY);
}

void
dtfcons_cnattach(void)
{

	dtf_is_console = 1;
}

int
dtfconscngetc(dev_t dev)
{
	u_int32_t status;
	u_int8_t buff[16];
	u_int8_t *p = buff;
	int len;
	int s;

	s = splhigh();

	do {
		p = buff;
		*p++ = DTF_POSIX_POLLKEY;
		len = (int)(p - buff);

		if (dtf_transaction(buff, &len) < 0 || len != 8)
			if (panicstr == NULL)
				panic("dtfconscngetc: lost link to DTF host");

		p = dtf_unpackdword(buff, &status);
	} while (status == 0);

	p = dtf_unpackdword(p, &status);

	splx(s);

	return ((int)status);
}

void
dtfconscnputc(dev_t dev, int ch)
{
	u_int8_t buff[16];
	u_int8_t *p = buff;
	int len;
	int s;

	s = splhigh();

	p = buff;
	*p++ = DTF_POSIX_WRITE;
	p = dtf_packdword(p, 1);
	*p++ = (u_int8_t)ch;
	len = (int)(p - buff);

	if (dtf_transaction(buff, &len) < 0 || len != 4)
		if (panicstr == NULL)
			panic("dtfconscnputc: lost link to DTF host");

	splx(s);
}

/*ARGSUSED*/
void
dtfconscnpollc(dev_t dev, int on)
{
}
