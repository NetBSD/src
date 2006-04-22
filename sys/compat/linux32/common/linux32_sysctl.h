/*	$NetBSD: linux32_sysctl.h,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef LINUX32_SYSCTL_H
#define LINUX32_SYSCTL_H

extern char linux32_sysname[];
extern char linux32_release[];
extern char linux32_version[];


#define EMUL_LINUX32_KERN	1
#define EMUL_LINUX32_MAXID	2 
	 
#define EMUL_LINUX32_NAMES { \
	 { 0, 0 }, \
	 { "kern", CTLTYPE_NODE }, \
}	
	 
#define EMUL_LINUX32_KERN_OSTYPE	1
#define EMUL_LINUX32_KERN_OSRELEASE	2
#define EMUL_LINUX32_KERN_VERSION	3
#define EMUL_LINUX32_KERN_MAXID		4 

#define EMUL_LINUX32_KERN_NAMES { \
	 { 0, 0 }, \
	 { "ostype", CTLTYPE_STRING }, \
	 { "osrelease", CTLTYPE_STRING }, \
	 { "osversion", CTLTYPE_STRING }, \
}  

#ifdef SYSCTL_SETUP_PROTO	                                
SYSCTL_SETUP_PROTO(sysctl_emul_linux32_setup);	             
#endif /* SYSCTL_SETUP_PROTO */	                          

#endif /* !_LINUX32_SYSCTL_H */
