#ifdef _KERNEL_OPT
#include "opt_altq.h"
#include "opt_inet.h"
#endif

#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include "npf.h"
#include <altq/altq.h>

#ifdef ALTQ

TAILQ_HEAD(npf_tags, npf_tagname)	npf_tags = TAILQ_HEAD_INITIALIZER(npf_tags),
				npf_qids = TAILQ_HEAD_INITIALIZER(npf_qids);

void tag_unref(struct npf_tags *, u_int16_t);
uint16_t npftagname2tag(struct npf_tags *, char *);

struct npf_altqqueue	*npf_altqs_active;
struct npf_altqqueue	*npf_altqs_inactive;
struct npf_altqqueue  npf_altqs[2];

struct pool		 npf_altq_pl;
int npf_altq_loaded = 0;
bool npf_altq_running = false;

/* npf interface to start altq */
void
npf_altq_init(void)
{
	pool_init(&npf_altq_pl, sizeof(struct npf_altq), 0, 0, 0, "npfaltqpl",
	    &pool_allocator_nointr, IPL_NONE);
	TAILQ_INIT(&npf_altqs[0]);
	TAILQ_INIT(&npf_altqs[1]);
	npf_altqs_active = &npf_altqs[0];
	npf_altqs_inactive = &npf_altqs[1];
}

int
npf_begin_altq(void)
{
	struct npf_altq	*altq;
	int		 error = 0;
	/* Purge the old altq list */
	while ((altq = TAILQ_FIRST(npf_altqs_inactive)) != NULL) {
		TAILQ_REMOVE(npf_altqs_inactive, altq, entries);
		if (altq->qname[0] == 0) {
			/* detach and destroy the discipline */
			if ((error = altq_remove(altq)) != 0)
				return error;
		} else
			npf_qid_unref(altq->qid);
		pool_put(&npf_altq_pl, altq);
	}

	return 0;
}

void
npf_qid_unref(u_int32_t qid)
{
	tag_unref(&npf_qids, (u_int16_t)qid);
}

void
tag_unref(struct npf_tags *head, u_int16_t tag)
{
	struct npf_tagname	*p, *next;
	if (tag == 0)
		return;
	for (p = TAILQ_FIRST(head); p != NULL; p = next) {
		next = TAILQ_NEXT(p, entries);
		if (tag == p->tag) {
			if (--p->ref == 0) {
				TAILQ_REMOVE(head, p, entries);
				free(p, M_TEMP);
			}
			break;
		}
	}
}

int
npf_add_altq(void *data)
{
	struct npfioc_altq	*paa = (struct npfioc_altq *)data;
	struct npf_altq		*altq, *a;
	int error;

	altq = pool_get(&npf_altq_pl, PR_NOWAIT);
	if (altq == NULL) {
		error = ENOMEM;
		return error;
	}
	memcpy(altq, &paa->altq, sizeof(*altq));
	/*
		* if this is for a queue, find the discipline and
		* copy the necessary fields
		*/
	if (altq->qname[0] != 0) {
		if ((altq->qid = npf_qname2qid(altq->qname)) == 0) {
			error = EBUSY;
			pool_put(&npf_altq_pl, altq);
			return error;
		}
		TAILQ_FOREACH(a, npf_altqs_inactive, entries) {
			if (strncmp(a->ifname, altq->ifname,
				IFNAMSIZ) == 0 && a->qname[0] == 0) {
				altq->altq_disc = a->altq_disc;
				break;
			}
		}
	}
	error = altq_add(altq);
	if (error) {
		pool_put(&npf_altq_pl, altq);
		return error;
	}
	TAILQ_INSERT_TAIL(npf_altqs_inactive, altq, entries);
	memcpy(&paa->altq, altq, sizeof(paa->altq));

	if (!npf_altq_loaded)
		npf_altq_loaded = 1;
	return 0;
}

u_int32_t
npf_qname2qid(char *qname)
{
	return ((u_int32_t)npftagname2tag(&npf_qids, qname));
}

u_int16_t
npftagname2tag(struct npf_tags *head, char *tagname)
{
	struct npf_tagname	*tag, *p = NULL;
	u_int16_t		 new_tagid = 1;
	TAILQ_FOREACH(tag, head, entries)
		if (strcmp(tagname, tag->name) == 0) {
			tag->ref++;
			return (tag->tag);
		}
	/*
	 * to avoid fragmentation, we do a linear search from the beginning
	 * and take the first free slot we find. if there is none or the list
	 * is empty, append a new entry at the end.
	 */
	/* new entry */
	if (!TAILQ_EMPTY(head))
		for (p = TAILQ_FIRST(head); p != NULL &&
		    p->tag == new_tagid; p = TAILQ_NEXT(p, entries))
			new_tagid = p->tag + 1;
	if (new_tagid > TAGID_MAX)
		return 0;
	/* allocate and fill new struct npf_tagname */
	tag = malloc(sizeof(*tag),
	    M_TEMP, M_NOWAIT);
	if (tag == NULL)
		return 0;
	memset(tag, 0, sizeof(*tag));
	strlcpy(tag->name, tagname, sizeof(tag->name));
	tag->tag = new_tagid;
	tag->ref++;
	if (p != NULL)	/* insert new entry before p */
		TAILQ_INSERT_BEFORE(p, tag, entries);
	else	/* either list empty or no free slot in between */
		TAILQ_INSERT_TAIL(head, tag, entries);
	return (tag->tag);
}

int
npf_get_altqs(void *data)
{
	struct npfioc_altq	*paa = (struct npfioc_altq *)data;
	struct npf_altq		*altq;
	paa->nq = 0;
	TAILQ_FOREACH(altq, npf_altqs_active, entries)
		paa->nq++;
	return 0 ;
}

int
npf_altq_start(void)
{
	int error;
    struct npf_altq		*altq;
    /* enable all altq interfaces on active list */
    TAILQ_FOREACH(altq, npf_altqs_active, entries) {
        if (altq->qname[0] == 0) {
            error = npf_enable_altq(altq);
            if (error != 0)
                break;
        }
    }

	return error;
}

int
npf_enable_altq(struct npf_altq *altq)
{
	struct ifnet		*ifp;
	struct tb_profile	 tb;
	int			 s, error = 0;
	if ((ifp = ifunit(altq->ifname)) == NULL)
		return EINVAL;
	if (ifp->if_snd.altq_type != ALTQT_NONE)
		error = altq_enable(&ifp->if_snd);
	/* set tokenbucket regulator */
	if (error == 0 && ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd)) {
		tb.rate = altq->ifbandwidth;
		tb.depth = altq->tbrsize;
		s = splnet();
		error = tbr_set(&ifp->if_snd, &tb);
		splx(s);
	}
	if (error == 0)
		npf_altq_running = true;
	return error;
}

#endif /* ALTQ */