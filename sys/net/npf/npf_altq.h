/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#include <sys/queue.h>
#ifndef NPF_ALTQ_H_
#define NPF_ALTQ_H_

#ifndef IFNAMSIZ
#define	IFNAMSIZ	16
#endif
/* queueing flags */
#ifndef NPF_QNAME_SIZE
#define NPF_QNAME_SIZE		64
#endif
#ifndef	TAGID_MAX
#define	TAGID_MAX	 50000
#endif
#ifndef NPF_TAG_NAME_SIZE
#define	NPF_TAG_NAME_SIZE	 64
#endif

/*
 * options defined on the cbq, priq and hfsc when configuring them
 */
struct npf_cbq_opts {
	u_int		minburst;
	u_int		maxburst;
	u_int		pktsize;
	u_int		maxpktsize;
	u_int		ns_per_byte;
	u_int		maxidle;
	int		minidle;
	u_int		offtime;
	int		flags;
};

struct npf_priq_opts {
	int		flags;
};

struct npf_hfsc_opts {
	/* real-time service curve */
	u_int		rtsc_m1;	/* slope of the 1st segment in bps */
	u_int		rtsc_d;		/* the x-projection of m1 in msec */
	u_int		rtsc_m2;	/* slope of the 2nd segment in bps */
	/* link-sharing service curve */
	u_int		lssc_m1;
	u_int		lssc_d;
	u_int		lssc_m2;
	/* upper-limit service curve */
	u_int		ulsc_m1;
	u_int		ulsc_d;
	u_int		ulsc_m2;
	int		flags;
};

/* entries for our tail queue for our altqs */
struct npf_altq {
	char			 ifname[IFNAMSIZ];
	void			*altq_disc;	/* discipline-specific state */
	TAILQ_ENTRY(npf_altq)	 entries;
	/* scheduler spec */
	uint8_t		 scheduler;	/* scheduler type */
	uint16_t		 tbrsize;	/* tokenbucket regulator size */
	uint32_t		 ifbandwidth;	/* interface bandwidth */
	/* queue spec */
	char			 qname[NPF_QNAME_SIZE];	/* queue name */
	char			 parent[NPF_QNAME_SIZE];	/* parent name */
	uint32_t		 parent_qid;	/* parent queue id */
	uint32_t		 bandwidth;	/* queue bandwidth */
	uint8_t		 priority;	/* priority */
	uint16_t		 qlimit;	/* queue size limit */
	uint16_t		 flags;		/* misc flags */
	union {
		struct npf_cbq_opts		 cbq_opts;
		struct npf_priq_opts	 priq_opts;
		struct npf_hfsc_opts	 hfsc_opts;
	} pq_u;
	u_int32_t		 qid;		/* return value */
};

struct qid {
	uint32_t qid;
};

struct npf_tag {
	uint16_t	tag;		/* tag id */
};

struct npf_tagname {
	TAILQ_ENTRY(npf_tagname)	entries;
	char			name[NPF_TAG_NAME_SIZE];
	uint16_t		tag;
	int			ref;
};

TAILQ_HEAD(npf_altqqueue, npf_altq);

extern int npf_altq_loaded;
extern int altqattached;
extern bool npf_altq_running;

extern int npf_get_altqs(void *);
extern void npf_altq_init(void);
extern int npf_begin_altq(void);
extern int npf_commit_altq(void);
extern int npf_add_altq(void *);
extern uint32_t npf_qname2qid(char *);
void npf_qid_unref(uint32_t);
extern int npf_altq_destroy(void);
int npf_altq_start(void);
int npf_disable_altq(struct npf_altq *);
int npf_stop_altq(void);
int npf_enable_altq(struct npf_altq );
extern int npf_get_qstats(void *);
