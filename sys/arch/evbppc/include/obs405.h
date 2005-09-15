/*	$NetBSD: obs405.h,v 1.5.4.2 2005/09/15 14:28:44 riz Exp $	*/

/*
 * Copyright 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima for The NetBSD Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef	_EVBPPC_OBS405_H_
#define	_EVBPPC_OBS405_H_

#include <sys/param.h>
#include <sys/device.h>

#include <powerpc/ibm4xx/ibm405gp.h>

#include "com.h"
#if (NCOM > 0)

#include <sys/termios.h>

#  ifndef CONADDR
#  define CONADDR	IBM405GP_UART0_BASE
#  endif
#  ifndef CONSPEED
#  define CONSPEED	B9600
#  endif
#  ifndef CONMODE
   /* 8N1 */
#  define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)
#  endif

#define OBS405_CONADDR		(CONADDR)
#define OBS405_CONSPEED		(CONSPEED)
#define OBS405_CONMODE		(CONMODE)

#endif /* NCOM */

#include <dev/ic/comreg.h>

/*
 * extern variables and functions
 */
extern void obs405_consinit(int com_freq);
extern void obs405_device_register(struct device *dev, void *aux, int com_freq);

#endif	/* _EVBPPC_OBS405_H_ */
