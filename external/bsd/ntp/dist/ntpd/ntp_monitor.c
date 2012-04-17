/*	$NetBSD: ntp_monitor.c,v 1.1.1.1.6.1 2012/04/17 00:03:47 yamt Exp $	*/

/*
 * ntp_monitor - monitor ntpd statistics
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_if.h"
#include "ntp_stdlib.h"
#include <ntp_random.h>

#include <stdio.h>
#include <signal.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

/*
 * Record statistics based on source address, mode and version. The
 * receive procedure calls us with the incoming rbufp before it does
 * anything else. While at it, implement rate controls for inbound
 * traffic.
 *
 * Each entry is doubly linked into two lists, a hash table and a most-
 * recently-used (MRU) list. When a packet arrives it is looked up in
 * the hash table. If found, the statistics are updated and the entry
 * relinked at the head of the MRU list. If not found, a new entry is
 * allocated, initialized and linked into both the hash table and at the
 * head of the MRU list.
 *
 * Memory is usually allocated by grabbing a big chunk of new memory and
 * cutting it up into littler pieces. The exception to this when we hit
 * the memory limit. Then we free memory by grabbing entries off the
 * tail for the MRU list, unlinking from the hash table, and
 * reinitializing.
 */
/*
 * Limits on the number of structures allocated.  This limit is picked
 * with the illicit knowlege that we can only return somewhat less than
 * 8K bytes in a mode 7 response packet, and that each structure will
 * require about 20 bytes of space in the response.
 *
 * ... I don't believe the above is true anymore ... jdg
 */
#ifndef MAXMONMEM
#define	MAXMONMEM	600	/* we allocate up to 600 structures */
#endif
#ifndef MONMEMINC
#define	MONMEMINC	40	/* allocate them 40 at a time */
#endif

/*
 * Hashing stuff
 */
#define	MON_HASH_SIZE	NTP_HASH_SIZE
#define	MON_HASH_MASK	NTP_HASH_MASK
#define	MON_HASH(addr)	NTP_HASH_ADDR(addr)

/*
 * Pointers to the hash table, the MRU list and the count table.  Memory
 * for the hash and count tables is only allocated if monitoring is
 * turned on.
 */
static	struct mon_data *mon_hash[MON_HASH_SIZE];  /* list ptrs */
struct	mon_data mon_mru_list;

/*
 * List of free structures structures, and counters of free and total
 * structures. The free structures are linked with the hash_next field.
 */
static  struct mon_data *mon_free;      /* free list or null if none */
static	int mon_total_mem;		/* total structures allocated */
static	int mon_mem_increments;		/* times called malloc() */

/*
 * Parameters of the RES_LIMITED restriction option. We define headway
 * as the idle time between packets. A packet is discarded if the
 * headway is less than the minimum, as well as if the average headway
 * is less than eight times the increment.
 */
int	ntp_minpkt = NTP_MINPKT;	/* minimum (log 2 s) */
int	ntp_minpoll = NTP_MINPOLL;	/* increment (log 2 s) */

/*
 * Initialization state.  We may be monitoring, we may not.  If
 * we aren't, we may not even have allocated any memory yet.
 */
int	mon_enabled;			/* enable switch */
int	mon_age = 3000;			/* preemption limit */
static	int mon_have_memory;
static	void	mon_getmoremem	(void);
static	void	remove_from_hash (struct mon_data *);

/*
 * init_mon - initialize monitoring global data
 */
void
init_mon(void)
{
	/*
	 * Don't do much of anything here.  We don't allocate memory
	 * until someone explicitly starts us.
	 */
	mon_enabled = MON_OFF;
	mon_have_memory = 0;
	mon_total_mem = 0;
	mon_mem_increments = 0;
	mon_free = NULL;
	memset(&mon_hash[0], 0, sizeof mon_hash);
	memset(&mon_mru_list, 0, sizeof mon_mru_list);
}


/*
 * mon_start - start up the monitoring software
 */
void
mon_start(
	int mode
	)
{

	if (mon_enabled != MON_OFF) {
		mon_enabled |= mode;
		return;
	}
	if (mode == MON_OFF)
	    return;
	
	if (!mon_have_memory) {
		mon_total_mem = 0;
		mon_mem_increments = 0;
		mon_free = NULL;
		mon_getmoremem();
		mon_have_memory = 1;
	}

	mon_mru_list.mru_next = &mon_mru_list;
	mon_mru_list.mru_prev = &mon_mru_list;
	mon_enabled = mode;
}


/*
 * mon_stop - stop the monitoring software
 */
void
mon_stop(
	int mode
	)
{
	register struct mon_data *md, *md_next;
	register int i;

	if (mon_enabled == MON_OFF)
		return;
	if ((mon_enabled & mode) == 0 || mode == MON_OFF)
		return;

	mon_enabled &= ~mode;
	if (mon_enabled != MON_OFF)
		return;
	
	/*
	 * Put everything back on the free list
	 */
	for (i = 0; i < MON_HASH_SIZE; i++) {
		md = mon_hash[i];               /* get next list */
		mon_hash[i] = NULL;             /* zero the list head */
		while (md != NULL) {
			md_next = md->hash_next;
			md->hash_next = mon_free;
			mon_free = md;
			md = md_next;
		}
	}
	mon_mru_list.mru_next = &mon_mru_list;
	mon_mru_list.mru_prev = &mon_mru_list;
}

void
ntp_monclearinterface(struct interface *interface)
{
        struct mon_data *md;

	for (md = mon_mru_list.mru_next; md != &mon_mru_list;
	    md = md->mru_next) {
		if (md->interface == interface) {
		      /* dequeue from mru list and put to free list */
		      md->mru_prev->mru_next = md->mru_next;
		      md->mru_next->mru_prev = md->mru_prev;
		      remove_from_hash(md);
		      md->hash_next = mon_free;
		      mon_free = md;
		}
	}
}


/*
 * ntp_monitor - record stats about this packet
 *
 * Returns flags
 */
int
ntp_monitor(
	struct recvbuf *rbufp,
	int	flags
	)
{
	register struct pkt *pkt;
	register struct mon_data *md;
	sockaddr_u addr;
	register u_int hash;
	register int mode;
	int	interval;

	if (mon_enabled == MON_OFF)
		return (flags);

	pkt = &rbufp->recv_pkt;
	memset(&addr, 0, sizeof(addr));
	memcpy(&addr, &(rbufp->recv_srcadr), sizeof(addr));
	hash = MON_HASH(&addr);
	mode = PKT_MODE(pkt->li_vn_mode);
	md = mon_hash[hash];
	while (md != NULL) {
		int	head;		/* headway increment */
		int	leak;		/* new headway */
		int	limit;		/* average threshold */

		/*
		 * Match address only to conserve MRU size.
		 */
		if (SOCK_EQ(&md->rmtadr, &addr)) {
			interval = current_time - md->lasttime;
			md->lasttime = current_time;
			md->count++;
			md->flags = flags;
			md->rmtport = NSRCPORT(&rbufp->recv_srcadr);
			md->mode = (u_char) mode;
			md->version = PKT_VERSION(pkt->li_vn_mode);

			/*
			 * Shuffle to the head of the MRU list.
			 */
			md->mru_next->mru_prev = md->mru_prev;
			md->mru_prev->mru_next = md->mru_next;
			md->mru_next = mon_mru_list.mru_next;
			md->mru_prev = &mon_mru_list;
			mon_mru_list.mru_next->mru_prev = md;
			mon_mru_list.mru_next = md;

			/*
			 * At this point the most recent arrival is
			 * first in the MRU list. Decrease the counter
			 * by the headway, but not less than zero.
			 */
			md->leak -= interval;
			if (md->leak < 0)
				md->leak = 0;
			head = 1 << ntp_minpoll;
			leak = md->leak + head;
			limit = NTP_SHIFT * head;
#ifdef DEBUG
			if (debug > 1)
				printf("restrict: interval %d headway %d limit %d\n",
				    interval, leak, limit);
#endif

			/*
			 * If the minimum and average thresholds are not
			 * exceeded, douse the RES_LIMITED and RES_KOD
			 * bits and increase the counter by the headway
			 * increment. Note that we give a 1-s grace for
			 * the minimum threshold and a 2-s grace for the
			 * headway increment. If one or both thresholds
			 * are exceeded and the old counter is less than
			 * the average threshold, set the counter to the
			 * average threshold plus the inrcrment and
			 * leave the RES_KOD bit lit. Othewise, leave
			 * the counter alone and douse the RES_KOD bit.
			 * This rate-limits the KoDs to no less than the
			 * average headway.
			 */
			if (interval + 1 >= (1 << ntp_minpkt) &&
			    leak < limit) {
				md->leak = leak - 2;
				md->flags &= ~(RES_LIMITED | RES_KOD);
			} else if (md->leak < limit) {
				md->leak = limit + head;
			} else {
				md->flags &= ~RES_KOD;
			}
			return (md->flags);
		}
		md = md->hash_next;
	}

	/*
	 * If we got here, this is the first we've heard of this
	 * guy.  Get him some memory, either from the free list
	 * or from the tail of the MRU list.
	 */
	if (mon_free == NULL && mon_total_mem >= MAXMONMEM) {

		/*
		 * Preempt from the MRU list if old enough.
		 */
		md = mon_mru_list.mru_prev;
		if (ntp_random() / (2. * FRAC) > (double)(current_time
		    - md->lasttime) / mon_age)
			return (flags & ~(RES_LIMITED | RES_KOD));

		md->mru_prev->mru_next = &mon_mru_list;
		mon_mru_list.mru_prev = md->mru_prev;
		remove_from_hash(md);
	} else {
		if (mon_free == NULL)
			mon_getmoremem();
		md = mon_free;
		mon_free = md->hash_next;
	}

	/*
	 * Got one, initialize it
	 */
	md->lasttime = md->firsttime = current_time;
	md->count = 1;
	md->flags = flags & ~(RES_LIMITED | RES_KOD);
	md->leak = 0;
	memset(&md->rmtadr, 0, sizeof(md->rmtadr));
	memcpy(&md->rmtadr, &addr, sizeof(addr));
	md->rmtport = NSRCPORT(&rbufp->recv_srcadr);
	md->mode = (u_char) mode;
	md->version = PKT_VERSION(pkt->li_vn_mode);
	md->interface = rbufp->dstadr;
	md->cast_flags = (u_char)(((rbufp->dstadr->flags &
	    INT_MCASTOPEN) && rbufp->fd == md->interface->fd) ?
	    MDF_MCAST: rbufp->fd == md->interface->bfd ? MDF_BCAST :
	    MDF_UCAST);

	/*
	 * Drop him into front of the hash table. Also put him on top of
	 * the MRU list.
	 */
	md->hash_next = mon_hash[hash];
	mon_hash[hash] = md;
	md->mru_next = mon_mru_list.mru_next;
	md->mru_prev = &mon_mru_list;
	mon_mru_list.mru_next->mru_prev = md;
	mon_mru_list.mru_next = md;
	return (md->flags);
}


/*
 * mon_getmoremem - get more memory and put it on the free list
 */
static void
mon_getmoremem(void)
{
	register struct mon_data *md;
	register int i;
	struct mon_data *freedata;      /* 'old' free list (null) */

	md = (struct mon_data *)emalloc(MONMEMINC *
	    sizeof(struct mon_data));
	freedata = mon_free;
	mon_free = md;
	for (i = 0; i < (MONMEMINC-1); i++) {
		md->hash_next = (md + 1);
		md++;
	}

	/*
	 * md now points at the last.  Link in the rest of the chain.
	 */
	md->hash_next = freedata;
	mon_total_mem += MONMEMINC;
	mon_mem_increments++;
}

static void
remove_from_hash(
	struct mon_data *md
	)
{
	register u_int hash;
	register struct mon_data *md_prev;

	hash = MON_HASH(&md->rmtadr);
	if (mon_hash[hash] == md) {
		mon_hash[hash] = md->hash_next;
	} else {
		md_prev = mon_hash[hash];
		while (md_prev->hash_next != md) {
			md_prev = md_prev->hash_next;
			if (md_prev == NULL) {
				/* logic error */
				return;
			}
		}
		md_prev->hash_next = md->hash_next;
	}
}
