/*++
/* NAME
/*	smtp_state 8
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
#include <vstream.h>

/* Global library. */

#include <mail_conf.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

/* smtp_state_alloc - initialize */

SMTP_STATE *smtp_state_alloc(void)
{
    SMTP_STATE *state = (SMTP_STATE *) mymalloc(sizeof(*state));

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
    smtp_sasl_connect(state);
#endif
    state->size_limit = 0;
    return (state);
}

/* smtp_state_free - destroy state */

void    smtp_state_free(SMTP_STATE *state)
{
    vstring_free(state->buffer);
    vstring_free(state->scratch);
    vstring_free(state->scratch2);
#ifdef USE_SASL_AUTH
    smtp_sasl_cleanup(state);
#endif
    myfree((char *) state);
}
