/*	$NetBSD: conf.h,v 1.11 2002/02/28 03:17:25 simonb Exp $	*/

/*
 * Copyright (c) 1996 Bernd Ernesti.  All rights reserved.
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
 *	This product includes software developed by Bernd Ernesti.
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

#define	DEV_RELOAD	20	/* minor device 20  is magic memory
				 * which you can write a kernel image to,
				 * causing a reboot into that kernel
				 */
#include <sys/conf.h>

cdev_decl(ctty);

bdev_decl(fd);
cdev_decl(fd);

/* open, close, read, write, ioctl -- XXX should be a generic device */
#define	cdev_par_init(c,n)	cdev__ocrwi_init(c,n)

cdev_decl(ms);

cdev_decl(grf);

cdev_decl(kbd);

cdev_decl(ite);

cdev_decl(par);

cdev_decl(ser);

cdev_decl(msc);

cdev_decl(mfcs);

cdev_decl(com);

cdev_decl(lpt);

cdev_decl(view);

bdev_decl(sw);
cdev_decl(sw);

cdev_decl(scsibus);
