/*	$NetBSD: verify_sender_addr.c,v 1.1.1.1 2013/01/02 18:59:00 tron Exp $	*/

/*++
/* NAME
/*	verify_sender_addr 3
/* SUMMARY
/*	time-dependent probe sender addresses
/* SYNOPSIS
/*	#include <verify_sender_addr.h>
/*
/*	char	*var_verify_sender;
/*	int	var_verify_sender_ttl;
/*
/*	const char *make_verify_sender_addr()
/*
/*	const char *valid_verify_sender_addr(addr)
/*	const char *addr;
/* DESCRIPTION
/*	This module computes or verifies a constant or time-dependent
/*	sender address for an address verification probe. The
/*	time-dependent portion is appended to the address localpart
/*	specified with the address_verify_sender parameter.
/*
/*	When the address_verify_sender parameter is empty or <>,
/*	the sender address always the empty address (i.e. always
/*	time-independent).
/*
/*	The caller must initialize the address_verify_sender and
/*	address_verify_sender_ttl parameter values.
/*
/*	make_verify_sender_addr() generates an envelope sender
/*	address for an address verification probe.
/*
/*	valid_verify_sender_addr() verifies that the given address
/*	is a valid sender address for address verification probes.
/*	When probe sender addresses are configured to be time-dependent,
/*	the given address is allowed to differ by +/-1 TTL unit
/*	from the expected address.  The result is a null pointer
/*	when no match is found. Otherwise, the result is the sender
/*	address without the time-dependent portion; this is the
/*	address that should be used for further delivery.
/* DIAGNOSTICS
/*	Fatal errors: malformed address_verify_sender value; out
/*	of memory.
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <events.h>

/* Global library */

#include <mail_params.h>
#include <rewrite_clnt.h>
#include <safe_ultostr.h>
#include <verify_sender_addr.h>

/* Application-specific. */

 /*
  * We convert the time-dependent portion to a safe string (no vowels) in a
  * reversible manner, so that we can check an incoming address against the
  * current and +/-1 TTL time slot. This allows for some time slippage
  * between multiple MTAs that handle mail for the same site. We use base 31
  * so that the time stamp contains B-Z0-9. This simplifies regression tests.
  */
#define VERIFY_BASE		31

 /*
  * We append the time-dependent portion to the localpart of the the address
  * verification probe sender address, so that the result has the form
  * ``fixed1variable@fixed2''. There is no delimiter between ``fixed1'' and
  * ``variable'', because that could make "old" time stamps valid depending
  * on how the recipient_delimiter feature is configured. The fixed text is
  * taken from var_verify_sender with perhaps domain information appended
  * during address canonicalization. The variable part of the address changes
  * every var_verify_sender_ttl seconds.
  */
char   *var_verify_sender;		/* "bare" probe sender address */
int     var_verify_sender_ttl;		/* time between address changes */

 /*
  * Scaffolding for stand-alone testing.
  */
#ifdef TEST
#undef event_time
#define event_time() verify_time
static unsigned long verify_time;

#endif

#define VERIFY_SENDER_ADDR_EPOCH() (event_time() / var_verify_sender_ttl)

 /*
  * SLMs.
  */
#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* make_verify_sender_addr - generate address_verify_sender address */

const char *make_verify_sender_addr(void)
{
    static VSTRING *verify_sender_buf;	/* the complete sender address */
    static VSTRING *my_epoch_buf;	/* scratch space */
    char   *my_at_domain;

    /*
     * The null sender is always time-independent.
     */
    if (*var_verify_sender == 0 || strcmp(var_verify_sender, "<>") == 0)
	return ("");

    /*
     * Sanity check.
     */
    if (*var_verify_sender == '@')
	msg_fatal("parameter %s: value \"%s\" must not start with '@'",
		  VAR_VERIFY_SENDER, var_verify_sender);
    if ((my_at_domain = strchr(var_verify_sender, '@')) != 0 && my_at_domain[1] == 0)
	msg_fatal("parameter %s: value \"%s\" must not end with '@'",
		  VAR_VERIFY_SENDER, var_verify_sender);

    /*
     * One-time initialization.
     */
    if (verify_sender_buf == 0) {
	verify_sender_buf = vstring_alloc(10);
	my_epoch_buf = vstring_alloc(10);
    }

    /*
     * Start with the bare sender address.
     */
    vstring_strcpy(verify_sender_buf, var_verify_sender);

    /*
     * Append the time stamp to the address localpart, encoded in some
     * non-decimal form for obscurity.
     * 
     * XXX It would be nice to have safe_ultostr() append-only support.
     */
    if (var_verify_sender_ttl > 0) {
	/* Strip the @domain portion, if applicable. */
	if (my_at_domain != 0)
	    vstring_truncate(verify_sender_buf,
			     (ssize_t) (my_at_domain - var_verify_sender));
	/* Append the time stamp to the address localpart. */
	vstring_sprintf_append(verify_sender_buf, "%s",
			       safe_ultostr(my_epoch_buf,
					    VERIFY_SENDER_ADDR_EPOCH(),
					    VERIFY_BASE, 0, 0));
	/* Add back the @domain, if applicable. */
	if (my_at_domain != 0)
	    vstring_sprintf_append(verify_sender_buf, "%s", my_at_domain);
    }

    /*
     * Rewrite the address to canonical form.
     */
    rewrite_clnt_internal(MAIL_ATTR_RWR_LOCAL, STR(verify_sender_buf),
			  verify_sender_buf);

    return (STR(verify_sender_buf));
}

/* valid_verify_sender_addr - decide if address matches time window +/-1 */

const char *valid_verify_sender_addr(const char *their_addr)
{
    static VSTRING *time_indep_sender_buf;	/* sender without time stamp */
    ssize_t base_len;
    unsigned long my_epoch;
    unsigned long their_epoch;
    char   *my_at_domain;
    char   *their_at_domain;
    char   *cp;

    /*
     * The null address is always time-independent.
     */
    if (*var_verify_sender == 0 || strcmp(var_verify_sender, "<>") == 0)
	return (*their_addr ? 0 : "");

    /*
     * One-time initialization. Generate the time-independent address that we
     * will return if the match is successful. This address is also used as a
     * matching template.
     */
    if (time_indep_sender_buf == 0) {
	time_indep_sender_buf = vstring_alloc(10);
	vstring_strcpy(time_indep_sender_buf, var_verify_sender);
	rewrite_clnt_internal(MAIL_ATTR_RWR_LOCAL, STR(time_indep_sender_buf),
			      time_indep_sender_buf);
    }

    /*
     * Check the time-independent sender localpart.
     */
    if ((my_at_domain = strchr(STR(time_indep_sender_buf), '@')) != 0)
	base_len = my_at_domain - STR(time_indep_sender_buf);
    else
	base_len = LEN(time_indep_sender_buf);
    if (strncasecmp(STR(time_indep_sender_buf), their_addr, base_len) != 0)
	return (0);				/* sender localpart mis-match */

    /*
     * Check the time-independent domain.
     */
    if ((their_at_domain = strchr(their_addr, '@')) == 0 && my_at_domain != 0)
	return (0);				/* sender domain mis-match */
    if (their_at_domain != 0
    && (my_at_domain == 0 || strcasecmp(their_at_domain, my_at_domain) != 0))
	return (0);				/* sender domain mis-match */

    /*
     * Check the time-dependent portion.
     */
    if (var_verify_sender_ttl > 0) {
	their_epoch = safe_strtoul(their_addr + base_len, &cp, VERIFY_BASE);
	if ((*cp != '@' && *cp != 0)
	    || (their_epoch == ULONG_MAX && errno == ERANGE))
	    return (0);				/* malformed time stamp */
	my_epoch = VERIFY_SENDER_ADDR_EPOCH();
	if (their_epoch < my_epoch - 1 || their_epoch > my_epoch + 1)
	    return (0);				/* outside time window */
    }

    /*
     * No time-dependent portion.
     */
    else {
	if (their_addr[base_len] != '@' && their_addr[base_len] != 0)
	    return (0);				/* garbage after sender base */
    }
    return (STR(time_indep_sender_buf));
}

 /*
  * Proof-of-concept test program. Read test address_verify_sender and
  * address_verify_sender_ttl values from stdin, and report results that we
  * would get on stdout.
  */
#ifdef TEST

#include <stdlib.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <vstring_vstream.h>
#include <mail_conf.h>
#include <conv_time.h>

int     main(int argc, char **argv)
{
    const char *verify_sender;
    const char *valid_sender;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    /*
     * Prepare to talk to the address rewriting service.
     */
    mail_conf_read();
    vstream_printf("using config files in %s\n", var_config_dir);
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    /*
     * Parse JCL.
     */
    if (argc != 3)
	msg_fatal("usage: %s address_verify_sender address_verify_sender_ttl",
		  argv[0]);
    var_verify_sender = argv[1];
    if (conv_time(argv[2], &var_verify_sender_ttl, 's') == 0)
	msg_fatal("bad time value: %s", argv[2]);
    verify_time = time((time_t *) 0);

    /*
     * Compute the current probe sender addres.
     */
    verify_sender = make_verify_sender_addr();

    /*
     * Check two past time slots.
     */
    if (var_verify_sender_ttl > 0) {
	verify_time -= 2 * var_verify_sender_ttl;
	vstream_printf("\"%s\" matches prev2: \"%s\"\n", verify_sender,
	     (valid_sender = valid_verify_sender_addr(verify_sender)) != 0 ?
		       valid_sender : "nope");
	verify_time += var_verify_sender_ttl;
	vstream_printf("\"%s\" matches prev1: \"%s\"\n", verify_sender,
	     (valid_sender = valid_verify_sender_addr(verify_sender)) != 0 ?
		       valid_sender : "nope");
	verify_time += var_verify_sender_ttl;
    }

    /*
     * Check the current time slot.
     */
    vstream_printf("\"%s\" matches self: \"%s\"\n", verify_sender,
	     (valid_sender = valid_verify_sender_addr(verify_sender)) != 0 ?
		   valid_sender : "nope");

    /*
     * Check two future time slots.
     */
    if (var_verify_sender_ttl > 0) {
	verify_time += var_verify_sender_ttl;
	vstream_printf("\"%s\" matches next1: \"%s\"\n", verify_sender,
	     (valid_sender = valid_verify_sender_addr(verify_sender)) != 0 ?
		       valid_sender : "nope");
	verify_time += var_verify_sender_ttl;
	vstream_printf("\"%s\" matches next2: \"%s\"\n", verify_sender,
	     (valid_sender = valid_verify_sender_addr(verify_sender)) != 0 ?
		       valid_sender : "nope");
    }
    vstream_fflush(VSTREAM_OUT);
    exit(0);
}

#endif
