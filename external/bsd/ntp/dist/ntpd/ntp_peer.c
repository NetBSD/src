/*	$NetBSD: ntp_peer.c,v 1.1.1.1 2009/12/13 16:55:35 kardel Exp $	*/

/*
 * ntp_peer.c - management of data maintained for peer associations
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>

#include "ntpd.h"
#include "ntp_lists.h"
#include "ntp_stdlib.h"
#include "ntp_control.h"
#include <ntp_random.h>
#ifdef OPENSSL
#include "openssl/rand.h"
#endif /* OPENSSL */

#ifdef SYS_WINNT
extern int accept_wildcard_if_for_winnt;
#endif

/*
 *                  Table of valid association combinations
 *                  ---------------------------------------
 *
 *                             packet->mode
 * peer->mode      | UNSPEC  ACTIVE PASSIVE  CLIENT  SERVER  BCAST
 * ----------      | ---------------------------------------------
 * NO_PEER         |   e       1       0       1       1       1
 * ACTIVE          |   e       1       1       0       0       0
 * PASSIVE         |   e       1       e       0       0       0
 * CLIENT          |   e       0       0       0       1       0
 * SERVER          |   e       0       0       0       0       0
 * BCAST           |   e       0       0       0       0       0
 * BCLIENT         |   e       0       0       0       e       1
 *
 * One point to note here: a packet in BCAST mode can potentially match
 * a peer in CLIENT mode, but we that is a special case and we check for
 * that early in the decision process.  This avoids having to keep track
 * of what kind of associations are possible etc...  We actually
 * circumvent that problem by requiring that the first b(m)roadcast
 * received after the change back to BCLIENT mode sets the clock.
 */
#define AM_MODES	7	/* number of rows and columns */
#define NO_PEER		0	/* action when no peer is found */

int AM[AM_MODES][AM_MODES] = {
/*	{ UNSPEC,   ACTIVE,     PASSIVE,    CLIENT,     SERVER,     BCAST } */

/*NONE*/{ AM_ERR, AM_NEWPASS, AM_NOMATCH, AM_FXMIT,   AM_MANYCAST, AM_NEWBCL},

/*A*/	{ AM_ERR, AM_PROCPKT, AM_PROCPKT, AM_NOMATCH, AM_NOMATCH,  AM_NOMATCH},

/*P*/	{ AM_ERR, AM_PROCPKT, AM_ERR,     AM_NOMATCH, AM_NOMATCH,  AM_NOMATCH},

/*C*/	{ AM_ERR, AM_NOMATCH, AM_NOMATCH, AM_NOMATCH, AM_PROCPKT,  AM_NOMATCH},

/*S*/	{ AM_ERR, AM_NOMATCH, AM_NOMATCH, AM_NOMATCH, AM_NOMATCH,  AM_NOMATCH},

/*BCST*/{ AM_ERR, AM_NOMATCH, AM_NOMATCH, AM_NOMATCH, AM_NOMATCH,  AM_NOMATCH},

/*BCL*/ { AM_ERR, AM_NOMATCH, AM_NOMATCH, AM_NOMATCH, AM_NOMATCH,  AM_PROCPKT},
};

#define MATCH_ASSOC(x, y)	AM[(x)][(y)]

/*
 * These routines manage the allocation of memory to peer structures
 * and the maintenance of the peer hash table. The three main entry
 * points are findpeer(), which looks for matching peer structures in
 * the peer list, newpeer(), which allocates a new peer structure and
 * adds it to the list, and unpeer(), which demobilizes the association
 * and deallocates the structure.
 */
/*
 * Peer hash tables
 */
struct peer *peer_hash[NTP_HASH_SIZE];	/* peer hash table */
int	peer_hash_count[NTP_HASH_SIZE];	/* peers in each bucket */
struct peer *assoc_hash[NTP_HASH_SIZE];	/* association ID hash table */
int	assoc_hash_count[NTP_HASH_SIZE]; /* peers in each bucket */
static struct peer *peer_free;		/* peer structures free list */
int	peer_free_count;		/* count of free structures */

/*
 * Association ID.  We initialize this value randomly, then assign a new
 * value every time the peer structure is incremented.
 */
static associd_t current_association_ID; /* association ID */

/*
 * Memory allocation watermarks.
 */
#define	INIT_PEER_ALLOC		15	/* initialize for 15 peers */
#define	INC_PEER_ALLOC		5	/* when run out, add 5 more */

/*
 * Miscellaneous statistic counters which may be queried.
 */
u_long	peer_timereset;			/* time stat counters zeroed */
u_long	findpeer_calls;			/* calls to findpeer */
u_long	assocpeer_calls;		/* calls to findpeerbyassoc */
u_long	peer_allocations;		/* allocations from free list */
u_long	peer_demobilizations;		/* structs freed to free list */
int	total_peer_structs;		/* peer structs */
int	peer_associations;		/* mobilized associations */
int	peer_preempt;			/* preemptable associations */
static struct peer init_peer_alloc[INIT_PEER_ALLOC]; /* init alloc */

static void	    getmorepeermem	 (void);
static struct interface *select_peerinterface (struct peer *, sockaddr_u *, struct interface *, u_char);

static int score(struct peer *);

/*
 * init_peer - initialize peer data structures and counters
 *
 * N.B. We use the random number routine in here. It had better be
 * initialized prior to getting here.
 */
void
init_peer(void)
{
	register int i;

	/*
	 * Clear hash tables and counters.
	 */
	memset(peer_hash, 0, sizeof(peer_hash));
	memset(peer_hash_count, 0, sizeof(peer_hash_count));
	memset(assoc_hash, 0, sizeof(assoc_hash));
	memset(assoc_hash_count, 0, sizeof(assoc_hash_count));

	/*
	 * Clear stat counters
	 */
	findpeer_calls = peer_allocations = 0;
	assocpeer_calls = peer_demobilizations = 0;

	/*
	 * Initialize peer memory.
	 */
	peer_free = NULL;
	for (i = 0; i < INIT_PEER_ALLOC; i++)
		LINK_SLIST(peer_free, &init_peer_alloc[i], next);
	total_peer_structs = INIT_PEER_ALLOC;
	peer_free_count = INIT_PEER_ALLOC;

	/*
	 * Initialize our first association ID
	 */
	while ((current_association_ID = ntp_random() & 0xffff) == 0);
}


/*
 * getmorepeermem - add more peer structures to the free list
 */
static void
getmorepeermem(void)
{
	register int i;
	register struct peer *peer;

	peer = (struct peer *)emalloc(INC_PEER_ALLOC *
	    sizeof(struct peer));
	for (i = 0; i < INC_PEER_ALLOC; i++) {
		LINK_SLIST(peer_free, peer, next);
		peer++;
	}

	total_peer_structs += INC_PEER_ALLOC;
	peer_free_count += INC_PEER_ALLOC;
}


/*
 * findexistingpeer - return a pointer to a peer in the hash table
 */
struct peer *
findexistingpeer(
	sockaddr_u *addr,
	struct peer *start_peer,
	int mode
	)
{
	register struct peer *peer;

	/*
	 * start_peer is included so we can locate instances of the
	 * same peer through different interfaces in the hash table.
	 */
	if (NULL == start_peer)
		peer = peer_hash[NTP_HASH_ADDR(addr)];
	else
		peer = start_peer->next;
	
	while (peer != NULL) {
		if (SOCK_EQ(addr, &peer->srcadr)
		    && NSRCPORT(addr) == NSRCPORT(&peer->srcadr)
		    && (-1 == mode || peer->hmode == mode))
			break;
		peer = peer->next;
	}
	return (peer);
}


/*
 * findpeer - find and return a peer in the hash table.
 */
struct peer *
findpeer(
	sockaddr_u *srcadr,
	struct interface *dstadr,
	int	pkt_mode,
	int	*action
	)
{
	register struct peer *peer;
	u_int hash;

	findpeer_calls++;
	hash = NTP_HASH_ADDR(srcadr);
	for (peer = peer_hash[hash]; peer != NULL; peer = peer->next) {
		if (SOCK_EQ(srcadr, &peer->srcadr) &&
		    NSRCPORT(srcadr) == NSRCPORT(&peer->srcadr)) {

			/*
			 * if the association matching rules determine
			 * that this is not a valid combination, then
			 * look for the next valid peer association.
			 */
			*action = MATCH_ASSOC(peer->hmode, pkt_mode);

			/*
			 * if an error was returned, exit back right
			 * here.
			 */
			if (*action == AM_ERR)
				return ((struct peer *)0);

			/*
			 * if a match is found, we stop our search.
			 */
			if (*action != AM_NOMATCH)
				break;
		}
	}

	/*
	 * If no matching association is found
	 */
	if (peer == 0) {
		*action = MATCH_ASSOC(NO_PEER, pkt_mode);
		return ((struct peer *)0);
	}
	set_peerdstadr(peer, dstadr);
	return (peer);
}

/*
 * findpeerbyassocid - find and return a peer using his association ID
 */
struct peer *
findpeerbyassoc(
	u_int assoc
	)
{
	register struct peer *peer;
	u_int hash;

	assocpeer_calls++;

	hash = assoc & NTP_HASH_MASK;
	for (peer = assoc_hash[hash]; peer != 0; peer =
	    peer->ass_next) {
		if (assoc == peer->associd)
		    return (peer);
	}
	return (NULL);
}


/*
 * clear_all - flush all time values for all associations
 */
void
clear_all(void)
{
	struct peer *peer, *next_peer;
	int n;

	/*
	 * This routine is called when the clock is stepped, and so all
	 * previously saved time values are untrusted.
	 */
	for (n = 0; n < NTP_HASH_SIZE; n++) {
		for (peer = peer_hash[n]; peer != 0; peer = next_peer) {
			next_peer = peer->next;
			if (!(peer->cast_flags & (MDF_ACAST |
			    MDF_MCAST | MDF_BCAST))) {
				peer_clear(peer, "STEP");
			}
		}
	}
#ifdef DEBUG
	if (debug)
		printf("clear_all: at %lu\n", current_time);
#endif
}


/*
 * score_all() - determine if an association can be demobilized
 */
int
score_all(
	struct peer *peer	/* peer structure pointer */
	)
{
	struct peer *speer, *next_peer;
	int	n;
	int	temp, tamp;

	/*
	 * This routine finds the minimum score for all ephemeral
	 * assocations and returns > 0 if the association can be
	 * demobilized.
	 */
	tamp = score(peer);
	temp = 100;
	for (n = 0; n < NTP_HASH_SIZE; n++) {
		for (speer = peer_hash[n]; speer != 0; speer =
		    next_peer) {
			int	x;

			next_peer = speer->next;
			if ((x = score(speer)) < temp && (peer->flags &
			    FLAG_PREEMPT))
				temp = x;
		}
	}
#ifdef DEBUG
	if (debug)
		printf("score_all: at %lu score %d min %d\n",
		    current_time, tamp, temp);
#endif
	if (tamp != temp)
		temp = 0;
	return (temp);
}


/*
 * score() - calculate preemption score
 */
static int
score(
	struct peer *peer	/* peer structure pointer */
	)
{
	int	temp;

	/*
	 * This routine calculates the premption score from the peer
	 * error bits and status. Increasing values are more cherished.
	 */
	temp = 0;
	if (!(peer->flash & TEST10))
		temp++;			/* 1 good synch and stratum */
	if (!(peer->flash & TEST13))
		temp++;			/* 2 reachable */
	if (!(peer->flash & TEST12))
		temp++;			/* 3 no loop */
	if (!(peer->flash & TEST11))
		temp++;			/* 4 good distance */
	if (peer->status >= CTL_PST_SEL_SELCAND)
		temp++;			/* 5 in the hunt */
	if (peer->status != CTL_PST_SEL_EXCESS)
		temp++;			/* 6 not spare tire */
	return (temp);			/* selection status */
}


/*
 * unpeer - remove peer structure from hash table and free structure
 */
void
unpeer(
	struct peer *peer_to_remove
	)
{
	register struct peer *unlinked;
	int	hash;
	char	tbuf[80];

	snprintf(tbuf, sizeof(tbuf), "assoc %d",
	    peer_to_remove->associd);
	report_event(PEVNT_DEMOBIL, peer_to_remove, tbuf);
	set_peerdstadr(peer_to_remove, NULL);
	hash = NTP_HASH_ADDR(&peer_to_remove->srcadr);
	peer_hash_count[hash]--;
	peer_demobilizations++;
	peer_associations--;
	if (peer_to_remove->flags & FLAG_PREEMPT)
		peer_preempt--;
#ifdef REFCLOCK
	/*
	 * If this peer is actually a clock, shut it down first
	 */
	if (peer_to_remove->flags & FLAG_REFCLOCK)
		refclock_unpeer(peer_to_remove);
#endif
	peer_to_remove->action = 0;	/* disable timeout actions */

	UNLINK_SLIST(unlinked, peer_hash[hash], peer_to_remove, next,
	    struct peer);

	if (NULL == unlinked) {
		peer_hash_count[hash]++;
		msyslog(LOG_ERR, "peer struct for %s not in table!",
		    stoa(&peer_to_remove->srcadr));
	}

	/*
	 * Remove him from the association hash as well.
	 */
	hash = peer_to_remove->associd & NTP_HASH_MASK;
	assoc_hash_count[hash]--;

	UNLINK_SLIST(unlinked, assoc_hash[hash], peer_to_remove,
	    ass_next, struct peer);

	if (NULL == unlinked) {
		assoc_hash_count[hash]++;
		msyslog(LOG_ERR,
		    "peer struct for %s not in association table!",
		    stoa(&peer_to_remove->srcadr));
	}

	LINK_SLIST(peer_free, peer_to_remove, next);
	peer_free_count++;
}


/*
 * peer_config - configure a new association
 */
struct peer *
peer_config(
	sockaddr_u *srcadr,
	struct interface *dstadr,
	int hmode,
	int version,
	int minpoll,
	int maxpoll,
	u_int flags,
	int ttl,
	keyid_t key,
	u_char *keystr
	)
{
	u_char cast_flags;

	/*
	 * We do a dirty little jig to figure the cast flags. This is
	 * probably not the best place to do this, at least until the
	 * configure code is rebuilt. Note only one flag can be set.
	 */
	switch (hmode) {
	case MODE_BROADCAST:
		if (IS_MCAST(srcadr))
			cast_flags = MDF_MCAST;
		else
			cast_flags = MDF_BCAST;
		break;

	case MODE_CLIENT:
		if (IS_MCAST(srcadr))
			cast_flags = MDF_ACAST;
		else
			cast_flags = MDF_UCAST;
		break;

	default:
		cast_flags = MDF_UCAST;
	}

	/*
	 * Mobilize the association and initialize its variables. If
	 * emulating ntpdate, force iburst.
	 */
	if (mode_ntpdate)
		flags |= FLAG_IBURST;
	return(newpeer(srcadr, dstadr, hmode, version, minpoll, maxpoll,
	    flags | FLAG_CONFIG, cast_flags, ttl, key));
}

/*
 * setup peer dstadr field keeping it in sync with the interface
 * structures
 */
void
set_peerdstadr(
	struct peer *peer,
	struct interface *interface
	)
{
	struct peer *unlinked;

	if (peer->dstadr != interface) {
		if (interface != NULL && (peer->cast_flags &
		    MDF_BCLNT) && (interface->flags & INT_MCASTIF) &&
		    peer->burst) {

			/*
			 * don't accept updates to a true multicast
			 * reception interface while a BCLNT peer is
			 * running it's unicast protocol
			 */
			return;
		}
		if (peer->dstadr != NULL) {
			peer->dstadr->peercnt--;
			UNLINK_SLIST(unlinked, peer->dstadr->peers,
			    peer, ilink, struct peer);
			msyslog(LOG_INFO,
				"%s interface %s -> %s",
				stoa(&peer->srcadr),
				stoa(&peer->dstadr->sin),
				(interface != NULL)
				    ? stoa(&interface->sin)
				    : "(null)");
		}
		peer->dstadr = interface;
		if (peer->dstadr != NULL) {
			LINK_SLIST(peer->dstadr->peers, peer, ilink);
			peer->dstadr->peercnt++;
		}
	}
}

/*
 * attempt to re-rebind interface if necessary
 */
static void
peer_refresh_interface(
	struct peer *peer
	)
{
	struct interface *niface, *piface;

	niface = select_peerinterface(peer, &peer->srcadr, NULL,
	    peer->cast_flags);

#ifdef DEBUG
	if (debug > 3)
	{
		printf(
		    "peer_refresh_interface: %s->%s mode %d vers %d poll %d %d flags 0x%x 0x%x ttl %d key %08x: new interface: ",
		    peer->dstadr == NULL ? "<null>" :
		    stoa(&peer->dstadr->sin), stoa(&peer->srcadr),
		    peer->hmode, peer->version, peer->minpoll,
		    peer->maxpoll, peer->flags, peer->cast_flags,
		    peer->ttl, peer->keyid);
		if (niface != NULL) {
			printf(
			    "fd=%d, bfd=%d, name=%.16s, flags=0x%x, scope=%d, ",
			    niface->fd,  niface->bfd, niface->name,
			    niface->flags, niface->scopeid);
			printf(", sin=%s", stoa((&niface->sin)));
			if (niface->flags & INT_BROADCAST)
				printf(", bcast=%s,",
				    stoa((&niface->bcast)));
			printf(", mask=%s\n", stoa((&niface->mask)));
		} else {
			printf("<NONE>\n");
		}
	}
#endif

	piface = peer->dstadr;
	set_peerdstadr(peer, niface);
	if (peer->dstadr) {
		/*
		 * clear crypto if we change the local address
		 */
		if (peer->dstadr != piface && !(peer->cast_flags &
		    MDF_ACAST) && peer->pmode != MODE_BROADCAST)
			peer_clear(peer, "XFAC");

		/*
	 	 * Broadcast needs the socket enabled for broadcast
	 	 */
		if (peer->cast_flags & MDF_BCAST) {
			enable_broadcast(peer->dstadr, &peer->srcadr);
		}

		/*
	 	 * Multicast needs the socket interface enabled for
		 * multicast
	 	 */
		if (peer->cast_flags & MDF_MCAST) {
			enable_multicast_if(peer->dstadr,
			    &peer->srcadr);
		}
	}
}

/*
 * refresh_all_peerinterfaces - see that all interface bindings are up
 * to date
 */
void
refresh_all_peerinterfaces(void)
{
	struct peer *peer, *next_peer;
	int n;

	/*
	 * this is called when the interface list has changed
	 * give all peers a chance to find a better interface
	 */
	for (n = 0; n < NTP_HASH_SIZE; n++) {
		for (peer = peer_hash[n]; peer != 0; peer = next_peer) {
			next_peer = peer->next;
			peer_refresh_interface(peer);
		}
	}
}

	
/*
 * find an interface suitable for the src address
 */
static struct interface *
select_peerinterface(
	struct peer *		peer,
	sockaddr_u *		srcadr,
	struct interface *	dstadr,
	u_char			cast_flags
	)
{
	struct interface *interface;
  
	/*
	 * Initialize the peer structure and dance the interface jig.
	 * Reference clocks step the loopback waltz, the others
	 * squaredance around the interface list looking for a buddy. If
	 * the dance peters out, there is always the wildcard interface.
	 * This might happen in some systems and would preclude proper
	 * operation with public key cryptography.
	 */
	if (ISREFCLOCKADR(srcadr))
		interface = loopback_interface;
	else
		if (cast_flags & (MDF_BCLNT | MDF_ACAST | MDF_MCAST | MDF_BCAST)) {
			interface = findbcastinter(srcadr);
#ifdef DEBUG
			if (debug > 3) {
				if (interface != NULL)
					printf("Found *-cast interface address %s, for address %s\n",
					    stoa(&(interface)->sin), stoa(srcadr));
				else
					printf("No *-cast local address found for address %s\n",
					       stoa(srcadr));
			}
#endif
			/*
			 * If it was a multicast packet,
			 * findbcastinter() may not find it, so try a
			 * little harder.
			 */
			if (interface == ANY_INTERFACE_CHOOSE(srcadr))
				interface = findinterface(srcadr);
		}
		else if (dstadr != NULL && dstadr !=
		    ANY_INTERFACE_CHOOSE(srcadr))
			interface = dstadr;
		else
			interface = findinterface(srcadr);

	/*
	 * we do not bind to the wildcard interfaces for output 
	 * as our (network) source address would be undefined and
	 * crypto will not work without knowing the own transmit address
	 */
	if (interface != NULL && interface->flags & INT_WILDCARD)
#ifdef SYS_WINNT
		if ( !accept_wildcard_if_for_winnt )  
#endif
			interface = NULL;


	return interface;
}

/*
 * newpeer - initialize a new peer association
 */
struct peer *
newpeer(
	sockaddr_u *srcadr,
	struct interface *dstadr,
	int	hmode,
	int	version,
	int	minpoll,
	int	maxpoll,
	u_int	flags,
	u_char	cast_flags,
	int	ttl,
	keyid_t	key
	)
{
	struct peer *peer;
	u_int	hash;
	char	tbuf[80];

#ifdef OPENSSL
	/*
	 * If Autokey is requested but not configured, complain loudly.
	 */
	if (!crypto_flags) {
		if (key > NTP_MAXKEY) {
			return (NULL);

		} else if (flags & FLAG_SKEY) {
			msyslog(LOG_ERR, "Autokey not configured");
			return (NULL);
		} 
	}
#endif /* OPENSSL */

	/*
	 * First search from the beginning for an association with given
	 * remote address and mode. If an interface is given, search
	 * from there to find the association which matches that
	 * destination. If the given interface is "any", track down the
	 * actual interface, because that's what gets put into the peer
	 * structure.
	 */
	peer = findexistingpeer(srcadr, NULL, hmode);
	if (dstadr != NULL) {
		while (peer != NULL) {
			if (peer->dstadr == dstadr)
				break;

			if (dstadr == ANY_INTERFACE_CHOOSE(srcadr) &&
			    peer->dstadr == findinterface(srcadr))
				break;

			peer = findexistingpeer(srcadr, peer, hmode);
		}
	}

	/*
	 * If a peer is found, this would be a duplicate and we don't
	 * allow that. This is mostly to avoid duplicate pool
	 * associations.
	 */
	if (peer != NULL)
		return (NULL);

	/*
	 * Allocate a new peer structure. Some dirt here, since some of
	 * the initialization requires knowlege of our system state.
	 */
	if (peer_free_count == 0)
		getmorepeermem();
	UNLINK_HEAD_SLIST(peer, peer_free, next);
	peer_free_count--;
	peer_associations++;
	if (flags & FLAG_PREEMPT)
		peer_preempt++;
	memset(peer, 0, sizeof(*peer));

	/*
	 * Assign an association ID and increment the system variable.
	 */
	peer->associd = current_association_ID;
	if (++current_association_ID == 0)
		++current_association_ID;

	DPRINTF(3, ("newpeer: cast flags: 0x%x for address: %s\n",
		    cast_flags, stoa(srcadr)));

	peer->srcadr = *srcadr;
	set_peerdstadr(peer, select_peerinterface(peer, srcadr, dstadr,
	    cast_flags));
	peer->hmode = (u_char)hmode;
	peer->version = (u_char)version;
	peer->flags = flags;

	/*
	 * It is an error to set minpoll less than NTP_MINPOLL or to
	 * set maxpoll greater than NTP_MAXPOLL. However, minpoll is
	 * clamped not greater than NTP_MAXPOLL and maxpoll is clamped
	 * not less than NTP_MINPOLL without complaint. Finally,
	 * minpoll is clamped not greater than maxpoll.
	 */
	if (minpoll == 0)
		peer->minpoll = NTP_MINDPOLL;
	else
		peer->minpoll = (u_char)min(minpoll, NTP_MAXPOLL);
	if (maxpoll == 0)
		peer->maxpoll = NTP_MAXDPOLL;
	else
		peer->maxpoll = (u_char)max(maxpoll, NTP_MINPOLL);
	if (peer->minpoll > peer->maxpoll)
		peer->minpoll = peer->maxpoll;

	if (peer->dstadr)
		DPRINTF(3, ("newpeer: using fd %d and our addr %s\n",
			    peer->dstadr->fd, stoa(&peer->dstadr->sin)));
	else
		DPRINTF(3, ("newpeer: local interface currently not bound\n"));	

	/*
	 * Broadcast needs the socket enabled for broadcast
	 */
	if ((cast_flags & MDF_BCAST) && peer->dstadr)
		enable_broadcast(peer->dstadr, srcadr);

	/*
	 * Multicast needs the socket interface enabled for multicast
	 */
	if ((cast_flags & MDF_MCAST) && peer->dstadr)
		enable_multicast_if(peer->dstadr, srcadr);

#ifdef OPENSSL
	if (key > NTP_MAXKEY)
		peer->flags |= FLAG_SKEY;
#endif /* OPENSSL */
	peer->cast_flags = cast_flags;
	peer->ttl = (u_char)ttl;
	peer->keyid = key;
	peer->precision = sys_precision;
	peer->hpoll = peer->minpoll;
	if (cast_flags & MDF_ACAST)
		peer_clear(peer, "ACST");
	else if (cast_flags & MDF_MCAST)
		peer_clear(peer, "MCST");
	else if (cast_flags & MDF_BCAST)
		peer_clear(peer, "BCST");
	else
		peer_clear(peer, "INIT");
	if (mode_ntpdate)
		peer_ntpdate++;

	/*
	 * Note time on statistics timers.
	 */
	peer->timereset = current_time;
	peer->timereachable = current_time;
	peer->timereceived = current_time;

#ifdef REFCLOCK
	if (ISREFCLOCKADR(&peer->srcadr)) {

		/*
		 * We let the reference clock support do clock
		 * dependent initialization.  This includes setting
		 * the peer timer, since the clock may have requirements
		 * for this.
		 */
		if (maxpoll == 0)
			peer->maxpoll = peer->minpoll;
		if (!refclock_newpeer(peer)) {
			/*
			 * Dump it, something screwed up
			 */
			set_peerdstadr(peer, NULL);
			LINK_SLIST(peer_free, peer, next);
			peer_free_count++;
			return (NULL);
		}
	}
#endif

	/*
	 * Put the new peer in the hash tables.
	 */
	hash = NTP_HASH_ADDR(&peer->srcadr);
	LINK_SLIST(peer_hash[hash], peer, next);
	peer_hash_count[hash]++;
	hash = peer->associd & NTP_HASH_MASK;
	LINK_SLIST(assoc_hash[hash], peer, ass_next);
	assoc_hash_count[hash]++;
	snprintf(tbuf, sizeof(tbuf), "assoc %d", peer->associd);
	report_event(PEVNT_MOBIL, peer, tbuf);
	DPRINTF(1, ("newpeer: %s->%s mode %d vers %d poll %d %d flags 0x%x 0x%x ttl %d key %08x\n",
	    peer->dstadr == NULL ? "<null>" : stoa(&peer->dstadr->sin),
	    stoa(&peer->srcadr), peer->hmode, peer->version,
	    peer->minpoll, peer->maxpoll, peer->flags, peer->cast_flags,
	    peer->ttl, peer->keyid));
	return (peer);
}


/*
 * peer_clr_stats - clear peer module statiistics counters
 */
void
peer_clr_stats(void)
{
	findpeer_calls = 0;
	assocpeer_calls = 0;
	peer_allocations = 0;
	peer_demobilizations = 0;
	peer_timereset = current_time;
}

/*
 * peer_reset - reset statistics counters
 */
void
peer_reset(
	struct peer *peer
	)
{
	if (peer == NULL)
	    return;

	peer->timereset = current_time;
	peer->sent = 0;
	peer->received = 0;
	peer->processed = 0;
	peer->badauth = 0;
	peer->bogusorg = 0;
	peer->oldpkt = 0;
	peer->seldisptoolarge = 0;
	peer->selbroken = 0;
}


/*
 * peer_all_reset - reset all peer statistics counters
 */
void
peer_all_reset(void)
{
	struct peer *peer;
	int hash;

	for (hash = 0; hash < NTP_HASH_SIZE; hash++)
	    for (peer = peer_hash[hash]; peer != 0; peer = peer->next)
		peer_reset(peer);
}


/*
 * findmanycastpeer - find and return a manycast peer
 */
struct peer *
findmanycastpeer(
	struct recvbuf *rbufp	/* receive buffer pointer */
	)
{
	register struct peer *peer;
	struct pkt *pkt;
	l_fp p_org;
	int i;

 	/*
 	 * This routine is called upon arrival of a server-mode message
	 * from a manycast client. Search the peer list for a manycast
	 * client association where the last transmit timestamp matches
	 * the originate timestamp. This assumes the transmit timestamps
	 * for possibly more than one manycast association are unique.
	 */
	pkt = &rbufp->recv_pkt;
	for (i = 0; i < NTP_HASH_SIZE; i++) {
		if (peer_hash_count[i] == 0)
			continue;

		for (peer = peer_hash[i]; peer != 0; peer =
		    peer->next) {
			if (peer->cast_flags & MDF_ACAST) {
				NTOHL_FP(&pkt->org, &p_org);
				if (L_ISEQU(&p_org, &peer->aorg))
					return (peer);
			}
		}
	}
	return (NULL);
}
