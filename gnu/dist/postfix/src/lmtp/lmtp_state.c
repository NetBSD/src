/*++
/* NAME
/*	lmtp_state 8
/* SUMMARY
/*	initialize/cleanup shared state
/* SYNOPSIS
/*	#include "lmtp.h"
/*
/*	LMTP_STATE *lmtp_state_alloc()
/*
/*	void	lmtp_state_free(state)
/*	LMTP_STATE *state;
/* DESCRIPTION
/*	lmtp_state_init() initializes the shared state, and allocates
/*	memory for buffers etc.
/*
/*	lmtp_cleanup() destroys memory allocated by lmtp_state_init().
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
/*
/*	Alterations for LMTP by:
/*	Philip A. Prindeville
/*	Mirapoint, Inc.
/*	USA.
/*
/*	Additional work on LMTP by:
/*	Amos Gouaux
/*	University of Texas at Dallas
/*	P.O. Box 830688, MC34
/*	Richardson, TX 75083, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>

/* Global library. */

#include <mail_conf.h>

/* Application-specific. */

#include "lmtp.h"
#include "lmtp_sasl.h"

/* lmtp_state_alloc - initialize */

LMTP_STATE *lmtp_state_alloc(void)
{
    LMTP_STATE *state = (LMTP_STATE *) mymalloc(sizeof(*state));

    state->src = 0;
    state->request = 0;
    state->session = 0;
    state->buffer = vstring_alloc(100);
    state->scratch = vstring_alloc(100);
    state->scratch2 = vstring_alloc(100);
    state->status = 0;
    state->features = 0;
    state->history = 0;
    state->error_mask = 0;
#ifdef USE_SASL_AUTH
    lmtp_sasl_connect(state);
#endif
    state->sndbufsize = 0;
    state->sndbuffree = 0;
    state->reuse = 0;
    return (state);
}

/* lmtp_state_free - destroy state */

void    lmtp_state_free(LMTP_STATE *state)
{
    vstring_free(state->buffer);
    vstring_free(state->scratch);
    vstring_free(state->scratch2);
#ifdef USE_SASL_AUTH
    lmtp_sasl_cleanup(state);
#endif
    myfree((char *) state);
}
