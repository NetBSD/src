/*	$NetBSD: rf_netbsd.h,v 1.12.8.2 2001/10/11 00:02:20 fvdl Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Greg Oster; Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _RF__RF_NETBSDSTUFF_H_
#define _RF__RF_NETBSDSTUFF_H_

#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/vnode.h>

#include <dev/raidframe/raidframevar.h>

struct raidcinfo {
	struct vnode *ci_vp;	/* component device's vnode */
	RF_ComponentLabel_t ci_label; /* components RAIDframe label */
#if 0
	size_t  ci_size;	/* size */
	char   *ci_path;	/* path to component */
	size_t  ci_pathlen;	/* length of component path */
#endif
};



/* XXX probably belongs in a different .h file. */
typedef struct RF_AutoConfig_s {
	char devname[56];       /* the name of this component */
	int flag;               /* a general-purpose flag */
	struct vnode *vp;       /* Mr. Vnode Pointer */
	RF_ComponentLabel_t *clabel;  /* the label */
	struct RF_AutoConfig_s *next; /* the next autoconfig structure 
				         in this set. */
} RF_AutoConfig_t;

typedef struct RF_ConfigSet_s {
	struct RF_AutoConfig_s *ac; /* all of the autoconfig structures for
				       this config set. */
	int rootable;               /* Set to 1 if this set can be root */
	struct RF_ConfigSet_s *next;
} RF_ConfigSet_t;

#endif /* _RF__RF_NETBSDSTUFF_H_ */
