/*	$NetBSD: sainfo.h,v 1.6.16.1 2011/02/08 16:18:30 bouyer Exp $	*/

/* Id: sainfo.h,v 1.5 2006/07/09 17:19:38 manubsd Exp */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SAINFO_H
#define _SAINFO_H

#include <sys/queue.h>

#define SAINFO_ANONYMOUS	((void *)NULL)
#define	SAINFO_CLIENTADDR	((void *)~0)

/* SA info */
struct sainfo {
	vchar_t *idsrc;
	vchar_t *iddst;
		/*
		 * idsrc and iddst are constructed body of ID payload.
		 * that is (struct ipsecdoi_id_b) + ID value.
		 * If idsrc == NULL, that is anonymous entry.
		 * If idsrc == ~0, that is client address entry.
		 */

#ifdef ENABLE_HYBRID
	vchar_t *group;
#endif

	time_t lifetime;
	int lifebyte;
	int pfs_group;		/* only use when pfs is required. */
	vchar_t *id_i;		/* identifier of the authorized initiator */
	struct sainfoalg *algs[MAXALGCLASS];

	uint32_t remoteid;

	LIST_ENTRY(sainfo) chain;
};

/* algorithm type */
struct sainfoalg {
	int alg;
	int encklen;			/* key length if encryption algorithm */
	struct sainfoalg *next;
};

extern struct sainfo *getsainfo __P((const vchar_t *,
	const vchar_t *, const vchar_t *, const vchar_t *, uint32_t));
extern struct sainfo *newsainfo __P((void));
extern void delsainfo __P((struct sainfo *));
extern void inssainfo __P((struct sainfo *));
extern void remsainfo __P((struct sainfo *));
extern void flushsainfo __P((void));
extern void initsainfo __P((void));
extern struct sainfoalg *newsainfoalg __P((void));
extern void delsainfoalg __P((struct sainfoalg *));
extern void inssainfoalg __P((struct sainfoalg **, struct sainfoalg *));
extern const char * sainfo2str __P((const struct sainfo *));

extern void sainfo_start_reload __P((void));
extern void sainfo_finish_reload __P((void));
extern void save_sainfotree_restore __P((void));

#endif /* _SAINFO_H */
