/*	$KAME: sainfo.h,v 1.7 2000/10/11 19:54:08 sakane Exp $	*/

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

#include <sys/queue.h>

/* SA info */
struct sainfo {
	vchar_t *idsrc;
	vchar_t *iddst;
		/*
		 * idsrc and iddst are constructed body of ID payload.
		 * that is (struct ipsecdoi_id_b) + ID value.
		 * If idsrc == NULL, that is anonymous entry.
		 */

	time_t lifetime;
	int lifebyte;
	int pfs_group;		/* only use when pfs is required. */
	int idvtype;		/* my identifier type */
	vchar_t *idv;		/* my identifier */
	struct sainfoalg *algs[MAXALGCLASS];

	LIST_ENTRY(sainfo) chain;
};

/* algorithm type */
struct sainfoalg {
	int alg;
	int encklen;			/* key length if encryption algorithm */
	struct sainfoalg *next;
};

extern struct sainfo *getsainfo __P((const vchar_t *, const vchar_t *));
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
