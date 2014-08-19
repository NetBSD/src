/*	$NetBSD: smtp_state.c,v 1.1.1.1.16.1 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	smtp_state 3
/* SUMMARY
/*	initialize/cleanup shared state
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	SMTP_STATE *smtp_state_alloc()
/*
/*	void	smtp_state_free(state)
/*	SMTP_STATE *state;
/* DESCRIPTION
/*	smtp_state_init() initializes the shared state, and allocates
/*	memory for buffers etc.
/*
/*	smtp_cleanup() destroys memory allocated by smtp_state_init().
/* STANDARDS
/* DIAGNOSTICS
/* BUGS
/* SEE ALSO
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstring.h>
#include <msg.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

/* smtp_state_alloc - initialize */

SMTP_STATE *smtp_state_alloc(void)
{
    SMTP_STATE *state = (SMTP_STATE *) mymalloc(sizeof(*state));

    state->misc_flags = 0;
    state->src = 0;
    state->service = 0;
    state->request = 0;
    state->session = 0;
    state->status = 0;
    state->space_left = 0;
    state->iterator->request_nexthop = vstring_alloc(100);
    state->iterator->dest = vstring_alloc(100);
    state->iterator->host = vstring_alloc(100);
    state->iterator->addr = vstring_alloc(100);
    state->iterator->saved_dest = vstring_alloc(100);
    if (var_smtp_cache_conn) {
	state->dest_label = vstring_alloc(10);
	state->dest_prop = vstring_alloc(10);
	state->endp_label = vstring_alloc(10);
	state->endp_prop = vstring_alloc(10);
	state->cache_used = htable_create(1);
    } else {
	state->dest_label = 0;
	state->dest_prop = 0;
	state->endp_label = 0;
	state->endp_prop = 0;
	state->cache_used = 0;
    }
    state->why = dsb_create();
    return (state);
}

/* smtp_state_free - destroy state */

void    smtp_state_free(SMTP_STATE *state)
{
#ifdef USE_TLS
    /* The TLS policy cache lifetime is one delivery. */
    smtp_tls_policy_cache_flush();
#endif
    vstring_free(state->iterator->request_nexthop);
    vstring_free(state->iterator->dest);
    vstring_free(state->iterator->host);
    vstring_free(state->iterator->addr);
    vstring_free(state->iterator->saved_dest);
    if (state->dest_label)
	vstring_free(state->dest_label);
    if (state->dest_prop)
	vstring_free(state->dest_prop);
    if (state->endp_label)
	vstring_free(state->endp_label);
    if (state->endp_prop)
	vstring_free(state->endp_prop);
    if (state->cache_used)
	htable_free(state->cache_used, (void (*) (char *)) 0);
    if (state->why)
	dsb_free(state->why);

    myfree((char *) state);
}
