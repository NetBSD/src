/*	$NetBSD: device.h,v 1.12 2010/02/05 12:13:36 phx Exp $	*/

/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
#ifndef _AMIGA_DEVICE_H_
#define _AMIGA_DEVICE_H_

/*
 * devices that need to configure before console use this
 * *and know it* (i.e. everything is really tight certain params won't be
 * passed in some cases and the devices will deal with it)
 */
void config_console(void);
int amiga_config_found(struct cfdata *, struct device *, void *, cfprint_t );
int simple_devprint(void *, const char *);
int matchname(const char *, const char *);
/*
 * false when initing for the console.
 */
extern int amiga_realconfig;


#define getsoftc(cdnam, unit)	device_lookup_private(&(cdnam), (unit))

/*
 * Reorder protection when accessing device registers.
 */
#if defined(__m68k__)
#define amiga_membarrier()
#elif defined(__powerpc__)
#define amiga_membarrier() __asm volatile ("eieio")
#endif

/*
 * Finish all bus operations and flush pipelines.
 */
#if defined(__m68k__)
#define amiga_cpu_sync() __asm volatile ("nop")
#elif defined(__powerpc__)
#define amiga_cpu_sync() __asm volatile ("sync; isync")
#endif

#endif /* _AMIGA_DEVICE_H_ */
