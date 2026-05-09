/*	$NetBSD: pol_stats.c,v 1.2 2026/05/09 18:49:16 christos Exp $	*/

/*++
/* NAME
/*	pol_stats 3
/* SUMMARY
/*	manage per-feature policy compliance status
/* SYNOPSIS
/*	#include <pol_stats.h>
/*
/*	POL_STATS *pol_stats_create(void)
/*
/*	void	pol_stats_revert(POL_STATS *pstats)
/*
/*	void	pol_stats_free(POL_STATS *pstats)
/*
/*	void	pol_stat_activate(
/*	POL_STATS *pstats,
/*	int	idx,
/*	const char *init_name)
/*
/*	void	pol_stat_decide(
/*	POL_STATS *pstats,
/*	int	idx,
/*	const char *final_name,
/*	int	status)
/*
/*	int	pol_stats_used(POL_STATS *pstats)
/*
/*	pol_stats_format(
/*	VSTRING *buf,
/*	const POL_STATS *pstats)
/* DESCRIPTION
/*	This module records for each activated feature whether the
/*	current program state satisfies the policy requirements for that
/*	feature. For example, whether a TLS handshake result matches
/*	DANE or STS requirements. The combined feature state can
/*	concisely be formatted with pol_stats_format() and exposed
/*	with logging.
/*
/*	Each feature has an initial name with the desired policy state,
/*	and a final name that corresponds to the policy state that was
/*	actually achieved. If the two names differ, then both names will
/*	be logged as initial:final.
/*
/*	pol_stats_create() creates one POL_STATS instance with all status
/*	information set to POL_STAT_INACTIVE.
/*
/*	pol_stats_revert() reverts all changes after pol_stats_create().
/*
/*	pol_stats_free() recycles storage for a POL_STATS instance.
/*
/*	Specific status information is accessed with an index. Valid
/*	indices are 0 or 1 (the caller decides their purpose).
/*
/*	pol_stat_activate() changes the status in pstats at index idx
/*	from POL_STAT_INACTIVE to POL_STAT_UNDECIDED, and updates the
/*	feature's initial name (pointer copy). Calls with an invalid
/*	index result in a panic(), and calls with an already active
/*	index result in a warning.
/*
/*	pol_stat_decide() updates the status in pstats at index idx from
/*	POL_STAT_UNDECIDED to POL_STAT_COMPLIANT or POL_STAT_VIOLATION,
/*	and records its final name or NULL (pointer copy). Calls with
/*	an invalid index or an unexpected decision status result in
/*	a panic(), and calls with an inactive or already decided index
/*	status result in a warning.
/*
/*	pol_stats_used() returns the number of activated categories for
/*	its argument.
/*
/*	pol_stats_format() formats TLS feature status information as
/*	feature/feature/etc., where:
/* .IP \(bu
/*	Each feature name is the initial name given to
/*	pol_stat_activate().
/* .IP \(bu
/*	When ':final-name' is appended to a feature name, the feature
/*	was downgraded to the final name given to pol_stats_decide().
/*	This does not necessarily imply policy compliance or
/*	non-compliance for that feature.
/* .IP \(bu
/*	When "!" is prepended to the feature, the policy for that feature
/*	was not satisfied and the operation was blocked.
/* .IP \(bu
/*	When "?" is appended to the feature, the policy for that feature
/*	was left undecided. This typically caused by a lost connection.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#ifdef USE_TLS

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>

 /*
  * Global library.
  */
#include <pol_stats.h>

/* pol_stats_create - create an all-inactive POL_STATS instance */

POL_STATS *pol_stats_create(void)
{
    POL_STATS *pstats;
    POL_STAT *tp;

#define POL_STAT_INIT(tp) do { \
	(tp)->init_name = 0; \
	(tp)->final_name = 0; \
	(tp)->status = POL_STAT_INACTIVE; \
    } while (0);

    pstats = (POL_STATS *) mymalloc(sizeof(*pstats));
    pstats->used = 0;
    for (tp = pstats->st; tp < pstats->st + POL_STATS_SIZE; tp++)
	POL_STAT_INIT(tp);
    return pstats;
}

/* pol_stats_revert - revert changes after pol_stats_create() */

void    pol_stats_revert(POL_STATS *pstats)
{
    POL_STAT *tp;

    pstats->used = 0;
    for (tp = pstats->st; tp < pstats->st + POL_STATS_SIZE; tp++)
	if (tp->status != POL_STAT_INACTIVE)
	    POL_STAT_INIT(tp);
}

/* pol_stats_free - POL_STATS destructor */

void    pol_stats_free(POL_STATS *pstats)
{
    myfree(pstats);
}

/* pol_stat_activate - activate status at index */

void    pol_stat_activate(POL_STATS *pstats, int idx, const char *init_name)
{
    POL_STAT *pol_stat;

    if (idx < 0 || idx >= POL_STATS_SIZE)
	msg_panic("%s: bad index: %d", __func__, idx);
    pol_stat = pstats->st + idx;
    if (pol_stat->status != POL_STAT_INACTIVE)
	msg_warn("%s: already active POL_STAT at index %d", __func__, idx);
    pol_stat->init_name = init_name;
    pol_stat->final_name = 0;
    pol_stat->status = POL_STAT_UNDECIDED;
    pstats->used += 1;
}

/* pol_stat_decide - update undecided status at index */

extern void pol_stat_decide(POL_STATS *pstats, int idx, const char *final_name,
			            int status)
{
    POL_STAT *pol_stat;

    if (status != POL_STAT_VIOLATION && status != POL_STAT_COMPLIANT)
	msg_panic("%s: bad new status: %d", __func__, status);
    if (idx < 0 || idx >= POL_STATS_SIZE)
	msg_panic("%s: bad index: %d", __func__, idx);
    pol_stat = pstats->st + idx;
    if (pol_stat->status != POL_STAT_UNDECIDED)
	msg_warn("%s: unexpected status %d at index %d",
		 __func__, pol_stat->status, idx);
    pol_stat->final_name = final_name;
    pol_stat->status = status;
}

/* pol_stat_access - peek at specific POL_STAT instance. */

static const POL_STAT *pol_stat_access(const POL_STATS *pstats, int idx)
{
    const POL_STAT *pol_stat;

    if (idx < 0 || idx >= POL_STATS_SIZE)
	msg_panic("%s: bad index: %d", __func__, idx);
    pol_stat = pstats->st + idx;
    return (pol_stat);
}

/* pol_stats_format - render in external representation */

void    pol_stats_format(VSTRING *buf, const POL_STATS *pstats)
{
    int     idx;
    const POL_STAT *pstat;
    int     field_count = 0;

    for (idx = 0; idx < POL_STATS_SIZE; idx++) {
	pstat = pol_stat_access(pstats, idx);
	if (pstat->status == POL_STAT_INACTIVE)
	    continue;
	if (field_count > 0)
	    vstring_strcat(buf, "/");
	if (pstat->status == POL_STAT_VIOLATION)
	    vstring_strcat(buf, "!");
	vstring_strcat(buf, pstat->init_name);
	if (pstat->final_name != 0
	    && strcmp(pstat->init_name, pstat->final_name) != 0)
	    vstring_sprintf_append(buf, ":%s", (pstat)->final_name);
	if (pstat->status == POL_STAT_UNDECIDED)
	    vstring_strcat(buf, "?");
	field_count += 1;
    }
}

#endif					/* USE_TLS */
