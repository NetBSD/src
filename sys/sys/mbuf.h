/*	$NetBSD: mbuf.h,v 1.77 2003/03/02 22:35:32 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1996 Matt Thomas.  All rights reserved.
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mbuf.h	8.5 (Berkeley) 2/19/95
 */

#ifndef _SYS_MBUF_H_
#define _SYS_MBUF_H_

#ifdef _KERNEL_OPT
#include "opt_mbuftrace.h"
#endif

#ifndef M_WAITOK
#include <sys/malloc.h>
#endif
#include <sys/pool.h>
#include <sys/queue.h>

/*
 * Mbufs are of a single size, MSIZE (machine/param.h), which
 * includes overhead.  An mbuf may add a single "mbuf cluster" of size
 * MCLBYTES (also in machine/param.h), which has no additional overhead
 * and is used instead of the internal data area; this is done when
 * at least MINCLSIZE of data must be stored.
 */

#define	MLEN		(MSIZE - sizeof(struct m_hdr))	/* normal data len */
#define	MHLEN		(MLEN - sizeof(struct pkthdr))	/* data len w/pkthdr */

#define	MINCLSIZE	(MHLEN+MLEN+1)	/* smallest amount to put in cluster */
#define	M_MAXCOMPRESS	(MHLEN / 2)	/* max amount to copy for compression */

/* Packet tags structure */
struct m_tag {
	SLIST_ENTRY(m_tag)	m_tag_link;	/* List of packet tags */
	u_int16_t		m_tag_id;	/* Tag ID */
	u_int16_t		m_tag_len;	/* Length of data */
};

/* mbuf ownership structure */
struct mowner {
	char mo_name[16];		/* owner name (fxp0) */
	char mo_descr[16];		/* owner description (input) */
	LIST_ENTRY(mowner) mo_link;	/* */
	u_long mo_claims;		/* # of small mbuf claimed */
	u_long mo_releases;		/* # of small mbuf released */
	u_long mo_cluster_claims;	/* # of M_CLUSTER mbuf claimed */
	u_long mo_cluster_releases;	/* # of M_CLUSTER mbuf released */
	u_long mo_ext_claims;		/* # of M_EXT mbuf claimed */
	u_long mo_ext_releases;		/* # of M_EXT mbuf released */
};

/*
 * Macros for type conversion
 * mtod(m,t) -	convert mbuf pointer to data pointer of correct type
 */
#define	mtod(m,t)	((t)((m)->m_data))

/* header at beginning of each mbuf: */
struct m_hdr {
	struct	mbuf *mh_next;		/* next buffer in chain */
	struct	mbuf *mh_nextpkt;	/* next chain in queue/record */
	caddr_t	mh_data;		/* location of data */
	struct	mowner *mh_owner;	/* mbuf owner */
	int	mh_len;			/* amount of data in this mbuf */
	short	mh_type;		/* type of data in this mbuf */
	short	mh_flags;		/* flags; see below */
};

/*
 * record/packet header in first mbuf of chain; valid if M_PKTHDR set
 *
 * A note about csum_data: For the out-bound direction, this indicates the
 * offset after the L4 header where the final L4 checksum value is to be
 * stored.  For the in-bound direction, it is only valid if the M_CSUM_DATA
 * flag is set.  In this case, an L4 checksum has been calculated by
 * hardware, but it is up to software to perform final verification.
 *
 * Note for in-bound TCP/UDP checksums, we expect the csum_data to NOT
 * be bit-wise inverted (the final step in the calculation of an IP
 * checksum) -- this is so we can accumulate the checksum for fragmented
 * packets during reassembly.
 */
struct	pkthdr {
	struct	ifnet *rcvif;		/* rcv interface */
	SLIST_HEAD(packet_tags, m_tag) tags; /* list of packet tags */
	int	len;			/* total packet length */
	int	csum_flags;		/* checksum flags */
	u_int32_t csum_data;		/* checksum data */
};

/*
 * Note: These bits are carefully arrange so that the compiler can have
 * a prayer of generating a jump table.
 */
#define	M_CSUM_TCPv4		0x00000001	/* TCP header/payload */
#define	M_CSUM_UDPv4		0x00000002	/* UDP header/payload */
#define	M_CSUM_TCP_UDP_BAD	0x00000004	/* TCP/UDP checksum bad */
#define	M_CSUM_DATA		0x00000008	/* consult csum_data */
#define	M_CSUM_TCPv6		0x00000010	/* IPv6 TCP header/payload */
#define	M_CSUM_UDPv6		0x00000020	/* IPv6 UDP header/payload */
#define	M_CSUM_IPv4		0x00000040	/* IPv4 header */
#define	M_CSUM_IPv4_BAD		0x00000080	/* IPv4 header checksum bad */

/* description of external storage mapped into mbuf, valid if M_EXT set */
struct m_ext {
	caddr_t	ext_buf;		/* start of buffer */
	void	(*ext_free)		/* free routine if not the usual */
		(struct mbuf *, caddr_t, size_t, void *);
	void	*ext_arg;		/* argument for ext_free */
	size_t	ext_size;		/* size of buffer, for ext_free */
	struct malloc_type *ext_type;	/* malloc type */
	struct mbuf *ext_nextref;
	struct mbuf *ext_prevref;
#ifdef DEBUG
	const char *ext_ofile;
	const char *ext_nfile;
	int ext_oline;
	int ext_nline;
#endif
};

struct mbuf {
	struct	m_hdr m_hdr;
	union {
		struct {
			struct	pkthdr MH_pkthdr;	/* M_PKTHDR set */
			union {
				struct	m_ext MH_ext;	/* M_EXT set */
				char	MH_databuf[MHLEN];
			} MH_dat;
		} MH;
		char	M_databuf[MLEN];		/* !M_PKTHDR, !M_EXT */
	} M_dat;
};
#define	m_next		m_hdr.mh_next
#define	m_len		m_hdr.mh_len
#define	m_data		m_hdr.mh_data
#define	m_owner		m_hdr.mh_owner
#define	m_type		m_hdr.mh_type
#define	m_flags		m_hdr.mh_flags
#define	m_nextpkt	m_hdr.mh_nextpkt
#define	m_pkthdr	M_dat.MH.MH_pkthdr
#define	m_ext		M_dat.MH.MH_dat.MH_ext
#define	m_pktdat	M_dat.MH.MH_dat.MH_databuf
#define	m_dat		M_dat.M_databuf

/* mbuf flags */
#define	M_EXT		0x0001	/* has associated external storage */
#define	M_PKTHDR	0x0002	/* start of record */
#define	M_EOR		0x0004	/* end of record */
#define	M_CLUSTER	0x0008	/* external storage is a cluster */

/* mbuf pkthdr flags, also in m_flags */
#define M_AUTHIPHDR	0x0010	/* data origin authentication for IP header */
#define M_DECRYPTED	0x0020	/* confidentiality */
#define M_LOOP		0x0040	/* for Mbuf statistics */
#define M_AUTHIPDGM     0x0080  /* data origin authentication */
#define	M_BCAST		0x0100	/* send/received as link-level broadcast */
#define	M_MCAST		0x0200	/* send/received as link-level multicast */
#define	M_CANFASTFWD	0x0400	/* used by filters to indicate packet can
				   be fast-forwarded */
#define M_ANYCAST6	0x0800	/* received as IPv6 anycast */
#define	M_LINK0		0x1000	/* link layer specific flag */
#define	M_LINK1		0x2000	/* link layer specific flag */
#define	M_LINK2		0x4000	/* link layer specific flag */

/* flags copied when copying m_pkthdr */
#define	M_COPYFLAGS	(M_PKTHDR|M_EOR|M_BCAST|M_MCAST|M_CANFASTFWD|M_ANYCAST6|M_LINK0|M_LINK1|M_LINK2|M_AUTHIPHDR|M_DECRYPTED|M_LOOP|M_AUTHIPDGM)

/* mbuf types */
#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SONAME	3	/* socket name */
#define	MT_SOOPTS	4	/* socket options */
#define	MT_FTABLE	5	/* fragment reassembly header */
#define MT_CONTROL	6	/* extra-data protocol message */
#define MT_OOBDATA	7	/* expedited data  */

/* flags to m_get/MGET */
#define	M_DONTWAIT	M_NOWAIT
#define	M_WAIT		M_WAITOK

/*
 * mbuf utility macros:
 *
 *	MBUFLOCK(code)
 * prevents a section of code from from being interrupted by network
 * drivers.
 */
#define	MBUFLOCK(code)							\
do {									\
	int ms = splvm();						\
	{ code }							\
	splx(ms);							\
} while (/* CONSTCOND */ 0)

#ifdef MBUFTRACE
/*
 * mbuf allocation tracing macros
 *
 */
#define _MOWNERINIT(m, type)						\
	((m)->m_owner = &unknown_mowners[(type)], (m)->m_owner->mo_claims++)

#define	_MOWNERREF(m, flags)	do {					\
	if ((flags) & M_EXT)						\
		(m)->m_owner->mo_ext_claims++;				\
	if ((flags) & M_CLUSTER)					\
		(m)->m_owner->mo_cluster_claims++;			\
} while (/* CONSTCOND */ 0)

#define	MOWNERREF(m, flags)	MBUFLOCK( _MOWNERREF((m), (flags)); );

#define	_MOWNERREVOKE(m, all, flags)	do {				\
	if ((flags) & M_EXT)						\
		(m)->m_owner->mo_ext_releases++;			\
	if ((flags) & M_CLUSTER)					\
		(m)->m_owner->mo_cluster_releases++;			\
	if (all) {							\
		(m)->m_owner->mo_releases++;				\
		(m)->m_owner = &revoked_mowner;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	_MOWNERCLAIM(m, mowner)	do {					\
	(m)->m_owner = (mowner);					\
	(mowner)->mo_claims++;						\
	if ((m)->m_flags & M_EXT)					\
		(mowner)->mo_ext_claims++;				\
	if ((m)->m_flags & M_CLUSTER)					\
		(mowner)->mo_cluster_claims++;				\
} while (/* CONSTCOND */ 0)

#define	MCLAIM(m, mowner) 						\
	MBUFLOCK(							\
		if ((m)->m_owner != (mowner) && (mowner) != NULL) {	\
			_MOWNERREVOKE((m), 1, (m)->m_flags);		\
			_MOWNERCLAIM((m), (mowner));			\
		}							\
	)

#define	MOWNER_ATTACH(mo)	LIST_INSERT_HEAD(&mowners, (mo), mo_link)
#define	MOWNER_DETACH(mo)	LIST_REMOVE((mo), mo_link)
#define MBUFTRACE_ASSERT(cond)	KASSERT(cond)
#else
#define _MOWNERINIT(m, type)		do { } while (/* CONSTCOND */ 0)
#define	_MOWNERREF(m, flags)		do { } while (/* CONSTCOND */ 0)
#define	MOWNERREF(m, flags)		do { } while (/* CONSTCOND */ 0)
#define	_MOWNERREVOKE(m, all, flags)	do { } while (/* CONSTCOND */ 0)
#define	_MOWNERCLAIM(m, mowner)		do { } while (/* CONSTCOND */ 0)
#define	MCLAIM(m, mowner) 		do { } while (/* CONSTCOND */ 0)
#define	MOWNER_ATTACH(mo)		do { } while (/* CONSTCOND */ 0)
#define	MOWNER_DETACH(mo)		do { } while (/* CONSTCOND */ 0)
#define	m_claim(m, mo)			do { } while (/* CONSTCOND */ 0)
#define MBUFTRACE_ASSERT(cond)		do { } while (/* CONSTCOND */ 0)
#endif


/*
 * mbuf allocation/deallocation macros:
 *
 *	MGET(struct mbuf *m, int how, int type)
 * allocates an mbuf and initializes it to contain internal data.
 *
 *	MGETHDR(struct mbuf *m, int how, int type)
 * allocates an mbuf and initializes it to contain a packet header
 * and internal data.
 *
 * If 'how' is M_WAIT, these macros (and the corresponding functions)
 * are guaranteed to return successfully.
 */
#define	MGET(m, how, type)						\
do {									\
	MBUFLOCK((m) = pool_cache_get(&mbpool_cache,			\
	    (how) == M_WAIT ? PR_WAITOK|PR_LIMITFAIL : 0););		\
	if (m) {							\
		MBUFLOCK(mbstat.m_mtypes[type]++;			\
		    _MOWNERINIT((m), (type)); );			\
		(m)->m_type = (type);					\
		(m)->m_next = (struct mbuf *)NULL;			\
		(m)->m_nextpkt = (struct mbuf *)NULL;			\
		(m)->m_data = (m)->m_dat;				\
		(m)->m_flags = 0;					\
	}								\
} while (/* CONSTCOND */ 0)

#define	MGETHDR(m, how, type)						\
do {									\
	MBUFLOCK((m) = pool_cache_get(&mbpool_cache,			\
	    (how) == M_WAIT ? PR_WAITOK|PR_LIMITFAIL : 0););		\
	if (m) {							\
		MBUFLOCK(mbstat.m_mtypes[type]++;			\
		    _MOWNERINIT((m), (type)); );			\
		(m)->m_type = (type);					\
		(m)->m_next = (struct mbuf *)NULL;			\
		(m)->m_nextpkt = (struct mbuf *)NULL;			\
		(m)->m_data = (m)->m_pktdat;				\
		(m)->m_flags = M_PKTHDR;				\
		(m)->m_pkthdr.csum_flags = 0;				\
		(m)->m_pkthdr.csum_data = 0;				\
		SLIST_INIT(&(m)->m_pkthdr.tags);			\
	}								\
} while (/* CONSTCOND */ 0)

#define	_M_
/*
 * Macros for tracking external storage associated with an mbuf.
 *
 * Note: add and delete reference must be called at splvm().
 */
#ifdef DEBUG
#define MCLREFDEBUGN(m, file, line)					\
do {									\
	(m)->m_ext.ext_nfile = (file);					\
	(m)->m_ext.ext_nline = (line);					\
} while (/* CONSTCOND */ 0)

#define MCLREFDEBUGO(m, file, line)					\
do {									\
	(m)->m_ext.ext_ofile = (file);					\
	(m)->m_ext.ext_oline = (line);					\
} while (/* CONSTCOND */ 0)
#else
#define MCLREFDEBUGN(m, file, line)
#define MCLREFDEBUGO(m, file, line)
#endif

#define	MCLBUFREF(p)
#define	MCLISREFERENCED(m)	((m)->m_ext.ext_nextref != (m))
#define	_MCLDEREFERENCE(m)						\
do {									\
	(m)->m_ext.ext_nextref->m_ext.ext_prevref =			\
		(m)->m_ext.ext_prevref;					\
	(m)->m_ext.ext_prevref->m_ext.ext_nextref =			\
		(m)->m_ext.ext_nextref;					\
} while (/* CONSTCOND */ 0)

#define	_MCLADDREFERENCE(o, n)						\
do {									\
	(n)->m_flags |= ((o)->m_flags & (M_EXT|M_CLUSTER));		\
	(n)->m_ext.ext_nextref = (o)->m_ext.ext_nextref;		\
	(n)->m_ext.ext_prevref = (o);					\
	(o)->m_ext.ext_nextref = (n);					\
	(n)->m_ext.ext_nextref->m_ext.ext_prevref = (n);		\
	_MOWNERREF((n), (n)->m_flags);					\
	MCLREFDEBUGN((n), __FILE__, __LINE__);				\
} while (/* CONSTCOND */ 0)

#define	MCLINITREFERENCE(m)						\
do {									\
	(m)->m_ext.ext_prevref = (m);					\
	(m)->m_ext.ext_nextref = (m);					\
	MCLREFDEBUGO((m), __FILE__, __LINE__);				\
	MCLREFDEBUGN((m), NULL, 0);					\
} while (/* CONSTCOND */ 0)

#define	MCLADDREFERENCE(o, n)	MBUFLOCK(_MCLADDREFERENCE((o), (n));)

/*
 * Macros for mbuf external storage.
 *
 * MCLGET allocates and adds an mbuf cluster to a normal mbuf;
 * the flag M_EXT is set upon success.
 *
 * MEXTMALLOC allocates external storage and adds it to
 * a normal mbuf; the flag M_EXT is set upon success.
 *
 * MEXTADD adds pre-allocated external storage to
 * a normal mbuf; the flag M_EXT is set upon success.
 */
#define	MCLGET(m, how)							\
do {									\
	MBUFLOCK(							\
		(m)->m_ext.ext_buf =					\
		    pool_cache_get(&mclpool_cache, (how) == M_WAIT ?	\
			(PR_WAITOK|PR_LIMITFAIL) : 0);			\
		if ((m)->m_ext.ext_buf != NULL)				\
			_MOWNERREF((m), M_EXT|M_CLUSTER);		\
	);								\
	if ((m)->m_ext.ext_buf != NULL) {				\
		(m)->m_data = (m)->m_ext.ext_buf;			\
		(m)->m_flags |= M_EXT|M_CLUSTER;			\
		(m)->m_ext.ext_size = MCLBYTES;				\
		(m)->m_ext.ext_free = NULL;				\
		(m)->m_ext.ext_arg = NULL;				\
		MCLINITREFERENCE(m);					\
	}								\
} while (/* CONSTCOND */ 0)

#define	MEXTMALLOC(m, size, how)					\
do {									\
	(m)->m_ext.ext_buf =						\
	    (caddr_t)malloc((size), mbtypes[(m)->m_type], (how));	\
	if ((m)->m_ext.ext_buf != NULL) {				\
		(m)->m_data = (m)->m_ext.ext_buf;			\
		(m)->m_flags |= M_EXT;					\
		(m)->m_flags &= ~M_CLUSTER;				\
		(m)->m_ext.ext_size = (size);				\
		(m)->m_ext.ext_free = NULL;				\
		(m)->m_ext.ext_arg = NULL;				\
		(m)->m_ext.ext_type = mbtypes[(m)->m_type];		\
		MCLINITREFERENCE(m);					\
		MOWNERREF((m), M_EXT);					\
	}								\
} while (/* CONSTCOND */ 0)

#define	MEXTADD(m, buf, size, type, free, arg)				\
do {									\
	(m)->m_data = (m)->m_ext.ext_buf = (caddr_t)(buf);		\
	(m)->m_flags |= M_EXT;						\
	(m)->m_flags &= ~M_CLUSTER;					\
	(m)->m_ext.ext_size = (size);					\
	(m)->m_ext.ext_free = (free);					\
	(m)->m_ext.ext_arg = (arg);					\
	(m)->m_ext.ext_type = (type);					\
	MCLINITREFERENCE(m);						\
	MOWNERREF((m), M_EXT);						\
} while (/* CONSTCOND */ 0)

#define	MEXTREMOVE(m)							\
do {									\
	int _ms_ = splvm(); /* MBUFLOCK */				\
	_MOWNERREVOKE((m), 0, (m)->m_flags);				\
	if (MCLISREFERENCED(m)) {					\
		_MCLDEREFERENCE(m);					\
		splx(_ms_);						\
	} else if ((m)->m_flags & M_CLUSTER) {				\
		pool_cache_put(&mclpool_cache, (m)->m_ext.ext_buf);	\
		splx(_ms_);						\
	} else if ((m)->m_ext.ext_free) {				\
		/*							\
		 * NOTE: We assume that MEXTREMOVE() is called from	\
		 * code where it is safe to invoke the free routine	\
		 * without the mbuf to perform bookkeeping.		\
		 */							\
		splx(_ms_);						\
		(*((m)->m_ext.ext_free))(NULL, (m)->m_ext.ext_buf,	\
		    (m)->m_ext.ext_size, (m)->m_ext.ext_arg);		\
	} else {							\
		splx(_ms_);						\
		free((m)->m_ext.ext_buf, (m)->m_ext.ext_type);		\
	}								\
	(m)->m_flags &= ~(M_CLUSTER|M_EXT);				\
	(m)->m_ext.ext_size = 0;	/* why ??? */			\
} while (/* CONSTCOND */ 0)

/*
 * Reset the data pointer on an mbuf.
 */
#define	MRESETDATA(m)							\
do {									\
	if ((m)->m_flags & M_EXT)					\
		(m)->m_data = (m)->m_ext.ext_buf;			\
	else if ((m)->m_flags & M_PKTHDR)				\
		(m)->m_data = (m)->m_pktdat;				\
	else								\
		(m)->m_data = (m)->m_dat;				\
} while (/* CONSTCOND */ 0)

/*
 * MFREE(struct mbuf *m, struct mbuf *n)
 * Free a single mbuf and associated external storage.
 * Place the successor, if any, in n.
 */
#define	MFREE(m, n)							\
	MBUFLOCK(							\
		mbstat.m_mtypes[(m)->m_type]--;				\
		if ((m)->m_flags & M_PKTHDR)				\
			m_tag_delete_chain((m), NULL);			\
		(n) = (m)->m_next;					\
		_MOWNERREVOKE((m), 1, m->m_flags);			\
		if ((m)->m_flags & M_EXT) {				\
			if (MCLISREFERENCED(m)) {			\
				_MCLDEREFERENCE(m);			\
				pool_cache_put(&mbpool_cache, (m));	\
			} else if ((m)->m_flags & M_CLUSTER) {		\
				pool_cache_put(&mclpool_cache,		\
				    (m)->m_ext.ext_buf);		\
				pool_cache_put(&mbpool_cache, (m));	\
			} else if ((m)->m_ext.ext_free) {		\
				/*					\
				 * (*ext_free)() is responsible for	\
				 * freeing the mbuf when it is safe.	\
				 */					\
				(*((m)->m_ext.ext_free))((m),		\
				    (m)->m_ext.ext_buf,			\
				    (m)->m_ext.ext_size,		\
				    (m)->m_ext.ext_arg);		\
			} else {					\
				free((m)->m_ext.ext_buf,		\
				    (m)->m_ext.ext_type);		\
				pool_cache_put(&mbpool_cache, (m));	\
			}						\
		} else {						\
			pool_cache_put(&mbpool_cache, (m));		\
		}							\
	)

/*
 * Copy mbuf pkthdr from `from' to `to'.
 * `from' must have M_PKTHDR set, and `to' must be empty.
 */
#define	M_COPY_PKTHDR(to, from)						\
do {									\
	(to)->m_pkthdr = (from)->m_pkthdr;				\
	(to)->m_flags = (from)->m_flags & M_COPYFLAGS;			\
	SLIST_INIT(&(to)->m_pkthdr.tags);				\
	m_tag_copy_chain((to), (from));					\
	(to)->m_data = (to)->m_pktdat;					\
} while (/* CONSTCOND */ 0)

/*
 * Set the m_data pointer of a newly-allocated mbuf (m_get/MGET) to place
 * an object of the specified size at the end of the mbuf, longword aligned.
 */
#define	M_ALIGN(m, len)							\
do {									\
	(m)->m_data += (MLEN - (len)) &~ (sizeof(long) - 1);		\
} while (/* CONSTCOND */ 0)

/*
 * As above, for mbufs allocated with m_gethdr/MGETHDR
 * or initialized by M_COPY_PKTHDR.
 */
#define	MH_ALIGN(m, len)						\
do {									\
	(m)->m_data += (MHLEN - (len)) &~ (sizeof(long) - 1);		\
} while (/* CONSTCOND */ 0)

/*
 * Determine if an mbuf's data area is read-only.  This is true
 * for non-cluster external storage and for clusters that are
 * being referenced by more than one mbuf.
 */
#define	M_READONLY(m)							\
	(((m)->m_flags & M_EXT) != 0 &&					\
	  (((m)->m_flags & M_CLUSTER) == 0 || MCLISREFERENCED(m)))

/*
 * Compute the amount of space available
 * before the current start of data in an mbuf.
 */
#define	_M_LEADINGSPACE(m)						\
	((m)->m_flags & M_EXT ? (m)->m_data - (m)->m_ext.ext_buf :	\
	 (m)->m_flags & M_PKTHDR ? (m)->m_data - (m)->m_pktdat :	\
	 (m)->m_data - (m)->m_dat)

#define	M_LEADINGSPACE(m)						\
	(M_READONLY((m)) ? 0 : _M_LEADINGSPACE((m)))

/*
 * Compute the amount of space available
 * after the end of data in an mbuf.
 */
#define	_M_TRAILINGSPACE(m)						\
	((m)->m_flags & M_EXT ? (m)->m_ext.ext_buf + (m)->m_ext.ext_size - \
	 ((m)->m_data + (m)->m_len) :					\
	 &(m)->m_dat[MLEN] - ((m)->m_data + (m)->m_len))

#define	M_TRAILINGSPACE(m)						\
	(M_READONLY((m)) ? 0 : _M_TRAILINGSPACE((m)))

/*
 * Arrange to prepend space of size plen to mbuf m.
 * If a new mbuf must be allocated, how specifies whether to wait.
 * If how is M_DONTWAIT and allocation fails, the original mbuf chain
 * is freed and m is set to NULL.
 */
#define	M_PREPEND(m, plen, how)						\
do {									\
	if (M_LEADINGSPACE(m) >= (plen)) {				\
		(m)->m_data -= (plen);					\
		(m)->m_len += (plen);					\
	} else								\
		(m) = m_prepend((m), (plen), (how));			\
	if ((m) && (m)->m_flags & M_PKTHDR)				\
		(m)->m_pkthdr.len += (plen);				\
} while (/* CONSTCOND */ 0)

/* change mbuf to new type */
#define MCHTYPE(m, t)							\
do {									\
	MBUFLOCK(mbstat.m_mtypes[(m)->m_type]--; mbstat.m_mtypes[t]++;); \
	(m)->m_type = t;						\
} while (/* CONSTCOND */ 0)

/* length to m_copy to copy all */
#define	M_COPYALL	1000000000

/* compatibility with 4.3 */
#define  m_copy(m, o, l)	m_copym((m), (o), (l), M_DONTWAIT)

/*
 * Allow drivers and/or protocols to use the rcvif member of
 * PKTHDR mbufs to store private context information.
 */
#define	M_GETCTX(m, t)		((t) (m)->m_pkthdr.rcvif + 0)
#define	M_SETCTX(m, c)		((void) ((m)->m_pkthdr.rcvif = (void *) (c)))

/*
 * Mbuf statistics.
 * For statistics related to mbuf and cluster allocations, see also the
 * pool headers (mbpool and mclpool).
 */
struct mbstat {
	u_long	_m_spare;	/* formerly m_mbufs */
	u_long	_m_spare1;	/* formerly m_clusters */
	u_long	_m_spare2;	/* spare field */
	u_long	_m_spare3;	/* formely m_clfree - free clusters */
	u_long	m_drops;	/* times failed to find space */
	u_long	m_wait;		/* times waited for space */
	u_long	m_drain;	/* times drained protocols for space */
	u_short	m_mtypes[256];	/* type specific mbuf allocations */
};

/*
 * Mbuf sysctl variables.
 */
#define	MBUF_MSIZE		1	/* int: mbuf base size */
#define	MBUF_MCLBYTES		2	/* int: mbuf cluster size */
#define	MBUF_NMBCLUSTERS	3	/* int: limit on the # of clusters */
#define	MBUF_MBLOWAT		4	/* int: mbuf low water mark */
#define	MBUF_MCLLOWAT		5	/* int: mbuf cluster low water mark */
#define	MBUF_STATS		6	/* struct: mbstat */
#define	MBUF_MOWNERS		7	/* struct: m_owner[] */
#define	MBUF_MAXID		8	/* number of valid MBUF ids */

#define	CTL_MBUF_NAMES {						\
	{ 0, 0 },							\
	{ "msize", CTLTYPE_INT },					\
	{ "mclbytes", CTLTYPE_INT },					\
	{ "nmbclusters", CTLTYPE_INT },					\
	{ "mblowat", CTLTYPE_INT },					\
	{ "mcllowat", CTLTYPE_INT },					\
	{ 0 /* "stats" */, CTLTYPE_STRUCT },				\
	{ 0 /* "mowners" */, CTLTYPE_STRUCT },				\
}

#ifdef	_KERNEL
/* always use m_pulldown codepath for KAME IPv6/IPsec */
#define PULLDOWN_TEST

extern struct mbstat mbstat;
extern int	nmbclusters;		/* limit on the # of clusters */
extern int	mblowat;		/* mbuf low water mark */
extern int	mcllowat;		/* mbuf cluster low water mark */
extern int	max_linkhdr;		/* largest link-level header */
extern int	max_protohdr;		/* largest protocol header */
extern int	max_hdr;		/* largest link+protocol header */
extern int	max_datalen;		/* MHLEN - max_hdr */
extern const int msize;			/* mbuf base size */
extern const int mclbytes;		/* mbuf cluster size */
extern struct pool mbpool;
extern struct pool mclpool;
extern struct pool_cache mbpool_cache;
extern struct pool_cache mclpool_cache;
#ifdef MBUFTRACE
LIST_HEAD(mownerhead, mowner);
extern struct mownerhead mowners;
extern struct mowner unknown_mowners[];
extern struct mowner revoked_mowner;
#endif

MALLOC_DECLARE(M_MBUF);
MALLOC_DECLARE(M_SONAME);
MALLOC_DECLARE(M_SOOPTS);

struct	mbuf *m_copym(struct mbuf *, int, int, int);
struct	mbuf *m_copypacket(struct mbuf *, int);
struct	mbuf *m_devget(char *, int, int, struct ifnet *,
			    void (*copy)(const void *, void *, size_t));
struct	mbuf *m_dup(struct mbuf *, int, int, int);
struct	mbuf *m_free(struct mbuf *);
struct	mbuf *m_get(int, int);
struct	mbuf *m_getclr(int, int);
struct	mbuf *m_gethdr(int, int);
struct	mbuf *m_prepend(struct mbuf *,int,int);
struct	mbuf *m_pulldown(struct mbuf *, int, int, int *);
struct	mbuf *m_pullup(struct mbuf *, int);
struct	mbuf *m_copyup(struct mbuf *, int, int);
struct	mbuf *m_split(struct mbuf *,int,int);
void	m_adj(struct mbuf *, int);
void	m_cat(struct mbuf *,struct mbuf *);
#ifdef MBUFTRACE
void	m_claim(struct mbuf *, struct mowner *);
#endif
void	m_clget(struct mbuf *, int);
int	m_mballoc(int, int);
void	m_copyback(struct mbuf *, int, int, caddr_t);
void	m_copydata(struct mbuf *, int, int, caddr_t);
void	m_freem(struct mbuf *);
void	m_reclaim(void *, int);
void	mbinit(void);

/* Packet tag routines */
struct	m_tag *m_tag_get(int, int, int);
void	m_tag_free(struct m_tag *);
void	m_tag_prepend(struct mbuf *, struct m_tag *);
void	m_tag_unlink(struct mbuf *, struct m_tag *);
void	m_tag_delete(struct mbuf *, struct m_tag *);
void	m_tag_delete_chain(struct mbuf *, struct m_tag *);
struct	m_tag *m_tag_find(struct mbuf *, int, struct m_tag *);
struct	m_tag *m_tag_copy(struct m_tag *);
int	m_tag_copy_chain(struct mbuf *, struct mbuf *);
void	m_tag_init(struct mbuf *);
struct	m_tag *m_tag_first(struct mbuf *);
struct	m_tag *m_tag_next(struct mbuf *, struct m_tag *);

/* Packet tag types */
#define PACKET_TAG_NONE				0  /* Nothing */
#define PACKET_TAG_VLAN				1  /* VLAN ID */
#define PACKET_TAG_ENCAP			2  /* encapsulation data */
#define PACKET_TAG_ESP				3  /* ESP information */
#define PACKET_TAG_PF_GENERATED			11 /* PF generated, pass always */
#define PACKET_TAG_PF_ROUTED			12 /* PF routed, no route loops */
#define PACKET_TAG_PF_FRAGCACHE			13 /* PF fragment cached */
#define PACKET_TAG_PF_QID			14 /* PF queue id */
#endif /* _KERNEL */
#endif /* !_SYS_MBUF_H_ */

#ifdef _KERNEL
#ifdef MBTYPES
struct malloc_type *mbtypes[] = {		/* XXX */
	M_FREE,		/* MT_FREE	0	should be on free list */
	M_MBUF,		/* MT_DATA	1	dynamic (data) allocation */
	M_MBUF,		/* MT_HEADER	2	packet header */
	M_SONAME,	/* MT_SONAME	3	socket name */
	M_SOOPTS,	/* MT_SOOPTS	4	socket options */
	M_FTABLE,	/* MT_FTABLE	5	fragment reassembly header */
	M_MBUF,		/* MT_CONTROL	6	extra-data protocol message */
	M_MBUF,		/* MT_OOBDATA	7	expedited data  */
};
#undef MBTYPES
#else
extern struct malloc_type *mbtypes[];
#endif /* MBTYPES */
#endif /* _KERNEL */
