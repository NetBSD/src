/*	$NetBSD: conf.h,v 1.5 2001/03/26 12:33:24 lukem Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * conf.h
 *
 * Prototypes for device driver functions
 */
 
#include <sys/conf.h>

bdev_decl(wd);
cdev_decl(wd);
bdev_decl(sw);
cdev_decl(sw);
bdev_decl(fd);
cdev_decl(fd);
bdev_decl(md);
cdev_decl(md);
bdev_decl(raid);
cdev_decl(raid);

/* Character device declarations */

/* open, close, read, write, ioctl, stop, tty, mmap -- XXX should be tty? */
#define cdev_physcon_init(c,n)	cdev__ttym_init(c,n,0)

/* open, close, read, ioctl */
#define cdev_prof_init(c,n)	cdev__ocri_init(c,n)

#define mmread  mmrw
#define mmwrite mmrw
cdev_decl(mm);

cdev_decl(physcon);
cdev_decl(com);
cdev_decl(lpt);
cdev_decl(pms);
cdev_decl(prof);
cdev_decl(usb);
cdev_decl(uhid);
cdev_decl(ugen);
cdev_decl(ulpt);
cdev_decl(ucom);
cdev_decl(urio);
cdev_decl(uscanner);
cdev_decl(vc_nb_);
cdev_decl(wsdisplay);
cdev_decl(wskbd);
cdev_decl(wsmouse);
cdev_decl(wsmux);
cdev_decl(scsibus);
cdev_decl(biconsdev);
