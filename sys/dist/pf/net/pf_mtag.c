/*	$NetBSD: pf_mtag.c,v 1.1.6.1 2008/06/18 16:33:34 simonb Exp $	*/
/*	$OpenBSD: pf.c,v 1.504 2005/10/17 08:43:35 henning Exp $ */

/*
 * Copyright (c) 2001 Daniel Hartmeier
 * Copyright (c) 2002,2003 Henning Brauer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Effort sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F30602-01-2-0537.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pf_mtag.c,v 1.1.6.1 2008/06/18 16:33:34 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>

#include <net/pf_mtag.h>

struct pf_mtag *
pf_find_mtag(struct mbuf *m)
{
	struct m_tag	*mtag;

	if ((mtag = m_tag_find(m, PACKET_TAG_PF, NULL)) == NULL)
		return (NULL);

	return ((struct pf_mtag *)(mtag + 1));
}

struct pf_mtag *
pf_get_mtag(struct mbuf *m)
{
	struct m_tag	*mtag;

	if ((mtag = m_tag_find(m, PACKET_TAG_PF, NULL)) == NULL) {
		mtag = m_tag_get(PACKET_TAG_PF, sizeof(struct pf_mtag),
		    M_NOWAIT);
		if (mtag == NULL)
			return (NULL);
		bzero(mtag + 1, sizeof(struct pf_mtag));
		m_tag_prepend(m, mtag);
	}

	return ((struct pf_mtag *)(mtag + 1));
}
