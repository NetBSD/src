/*	$NetBSD: uvm_stat.h,v 1.4 1998/02/07 11:09:43 mrg Exp $	*/

/*
 * XXXCDC: "ROUGH DRAFT" QUALITY UVM PRE-RELEASE FILE!   
 *	   >>>USE AT YOUR OWN RISK, WORK IS NOT FINISHED<<<
 */
/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
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
 *
 * from: Id: uvm_stat.h,v 1.1.2.4 1998/02/07 01:16:56 chs Exp
 */
/*
 * uvm_stat: monitor what is going on with uvm (or whatever)
 */

/*
 * counters  [XXX: maybe replace event counters with this]
 */

#define UVMCNT_MASK	0xf			/* rest are private */
#define UVMCNT_CNT	0			/* normal counter */
#define UVMCNT_DEV	1			/* device event counter */

struct uvm_cnt {
	int c;					/* the value */
	int t;					/* type */
	struct uvm_cnt *next;			/* global list of cnts */
	char *name;				/* counter name */
	void *p;				/* private data */
};

extern struct uvm_cnt *uvm_cnt_head;

/*
 * counter operations.  assume spl is set ok.
 */

#define UVMCNT_INIT(CNT,TYP,VAL,NAM,PRIV) { \
	CNT.c = VAL; \
	CNT.t = TYP; \
	CNT.next = uvm_cnt_head; \
	uvm_cnt_head = &CNT; \
	CNT.name = NAM; \
	CNT.p = PRIV; \
	}

#define UVMCNT_SET(C,V) { \
	(C).c = (V); \
	}

#define UVMCNT_ADD(C,V) { \
	(C).c += (V); \
	}

#define UVMCNT_INCR(C) UVMCNT_ADD(C,1)
#define UVMCNT_DECR(C) UVMCNT_ADD(C,-1)


/*
 * history/tracing
 */

#if !defined(UVM_NOHIST)
#define UVMHIST
#endif

struct uvm_history_ent {
  struct timeval tv; 		/* time stamp */
  char *fmt; 			/* printf format */
  char *fn;			/* function name */
  u_long call;			/* function call number */
  u_long v[4];			/* values */
};

struct uvm_history {
  int n;			/* number of entries */
  int f; 			/* next free one */
#if NCPU > 1
  simple_lock_data_t l;		/* lock on this history */
#endif /* NCPU */
  struct uvm_history_ent *e;	/* the malloc'd entries */
};

#ifndef UVMHIST
#define UVMHIST_DECL(NAME)
#define UVMHIST_INIT(NAME,N)
#define UVMHIST_INIT_STATIC(NAME,BUF)
#define UVMHIST_LOG(NAME,FMT,A,B,C,D)
#define UVMHIST_CALLED(NAME)
#define UVMHIST_FUNC(FNAME)
#define uvmhist_dump(NAME)
#else
#define UVMHIST_DECL(NAME) struct uvm_history NAME

#define UVMHIST_INIT(NAME,N) { \
	(NAME).n = (N); \
	(NAME).f = 0; \
	simple_lock_init(&(NAME).l); \
	(NAME).e = (struct uvm_history_ent *) \
		malloc(sizeof(struct uvm_history_ent) * (N), M_TEMP, \
				M_WAITOK); \
	bzero((NAME).e, sizeof(struct uvm_history_ent) * (N)); \
	}

#define UVMHIST_INIT_STATIC(NAME,BUF) { \
	(NAME).n = sizeof(BUF) / sizeof(struct uvm_history_ent); \
	(NAME).f = 0; \
	simple_lock_init(&(NAME).l); \
	(NAME).e = (struct uvm_history_ent *) (BUF); \
	bzero((NAME).e, sizeof(struct uvm_history_ent) * (NAME).n); \
	}

extern int cold;

#if defined(UVMHIST_PRINT)
#define UVMHIST_PRINTNOW(E) uvmhist_print(E); DELAY(100000);
#else
#define UVMHIST_PRINTNOW(E) /* nothing */
#endif

#define UVMHIST_LOG(NAME,FMT,A,B,C,D) { \
	register int i, s = splhigh(); \
	simple_lock(&(NAME).l); \
	i = (NAME).f; \
	(NAME).f = (i + 1) % (NAME).n; \
	simple_unlock(&(NAME).l); \
	splx(s); \
	if (!cold) microtime(&(NAME).e[i].tv); \
	(NAME).e[i].fmt = (FMT); \
	(NAME).e[i].fn = _uvmhist_name; \
	(NAME).e[i].call = _uvmhist_call; \
	(NAME).e[i].v[0] = (u_long)(A); \
	(NAME).e[i].v[1] = (u_long)(B); \
	(NAME).e[i].v[2] = (u_long)(C); \
	(NAME).e[i].v[3] = (u_long)(D); \
        UVMHIST_PRINTNOW(&((NAME).e[i])); \
	}

#define UVMHIST_CALLED(NAME) \
	{ int s = splhigh(); simple_lock(&(NAME).l); \
	 	_uvmhist_call = _uvmhist_cnt++; \
	  simple_unlock(&(NAME).l); splx(s); } \
	UVMHIST_LOG(NAME,"called!", 0, 0, 0, 0);

#define UVMHIST_FUNC(FNAME) \
	static int _uvmhist_cnt = 0; \
	static char *_uvmhist_name = FNAME; \
	int _uvmhist_call; 

static __inline void uvmhist_print __P((struct uvm_history_ent *));
	/* shut up GCC */

static __inline void uvmhist_print(e)

struct uvm_history_ent *e;
{
  printf("%06ld.%06ld ", e->tv.tv_sec, e->tv.tv_usec);
  printf("%s#%ld: ", e->fn, e->call);
  printf(e->fmt, e->v[0], e->v[1], e->v[2], e->v[3]);
  printf("\n");
}

#endif /* UVMHIST */
