/*	$NetBSD: legacy.c,v 1.1.1.1.4.2 2006/11/20 13:30:52 tron Exp $	*/

 /*
  * The old legacy TLS per-site policy engine, implemented with multiple
  * boolean variables, stripped down for exhaustive comparison with the new
  * legacy policy engine.
  */
/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <stringops.h>

 /*
  * Global policy variables.
  */
int     var_smtp_enforce_tls;
int     var_smtp_tls_enforce_peername;
int     var_smtp_use_tls;

 /*
  * Simplified session structure.
  */
typedef struct {
    int     tls_use_tls;
    int     tls_enforce_tls;
    int     tls_enforce_peername;
} SMTP_SESSION;

 /*
  * Per-site policies can override main.cf settings.
  */
typedef struct {
    int     dont_use;			/* don't use TLS */
    int     use;			/* useless, see above */
    int     enforce;			/* must always use TLS */
    int     enforce_peername;		/* must verify certificate name */
} SMTP_TLS_SITE_POLICY;

/* smtp_tls_site_policy - look up per-site TLS policy */

static void smtp_tls_site_policy(SMTP_TLS_SITE_POLICY *policy,
				         const char *lookup)
{

    /*
     * Initialize the default policy.
     */
    policy->dont_use = 0;
    policy->use = 0;
    policy->enforce = 0;
    policy->enforce_peername = 0;

    /*
     * Look up a non-default policy.
     */
    if (strcasecmp(lookup, "-")) {
	if (!strcasecmp(lookup, "NONE"))
	    policy->dont_use = 1;
	else if (!strcasecmp(lookup, "MAY"))
	    policy->use = 1;
	else if (!strcasecmp(lookup, "MUST"))
	    policy->enforce = policy->enforce_peername = 1;
	else if (!strcasecmp(lookup, "MUST_NOPEERMATCH"))
	    policy->enforce = 1;
	else
	    msg_fatal("unknown TLS policy '%s'", lookup);
    }
}

static void policy(SMTP_SESSION *session, const char *host, const char *dest)
{
    SMTP_TLS_SITE_POLICY host_policy;
    SMTP_TLS_SITE_POLICY rcpt_policy;

    session->tls_use_tls = session->tls_enforce_tls = 0;
    session->tls_enforce_peername = 0;

    /*
     * Override the main.cf TLS policy with an optional per-site policy.
     */
    smtp_tls_site_policy(&host_policy, host);
    smtp_tls_site_policy(&rcpt_policy, dest);

    /*
     * Fix 200601: a combined per-site (NONE + MAY) policy changed global
     * MUST into NONE, and all weaker global policies into MAY. This was
     * discovered with exhaustive simulation. Fix verified by comparing
     * exhaustive simulation results with Postfix 2.3 which re-implements
     * per-site policies from the ground up.
     */
#ifdef FIX200601
    if ((host_policy.dont_use || rcpt_policy.dont_use)
	&& (host_policy.use || rcpt_policy.use)) {
	host_policy.use = rcpt_policy.use = 0;
	host_policy.dont_use = rcpt_policy.dont_use = 1;
    }
#endif

    /*
     * Set up TLS enforcement for this session.
     */
    if ((var_smtp_enforce_tls && !host_policy.dont_use && !rcpt_policy.dont_use)
	|| host_policy.enforce || rcpt_policy.enforce)
	session->tls_enforce_tls = session->tls_use_tls = 1;

    /*
     * Set up peername checking for this session.
     * 
     * We want to make sure that a MUST* entry in the tls_per_site table always
     * has precedence. MUST always must lead to a peername check,
     * MUST_NOPEERMATCH must always disable it. Only when no explicit setting
     * has been found, the default will be used.
     * 
     * Fix 200601: a per-site MUST_NOPEERMATCH policy could not override a
     * global MUST policy. Fix verified by comparing exhaustive simulation
     * results with Postfix 2.3 which re-implements per-site policy from the
     * ground up.
     */
    if (host_policy.enforce && host_policy.enforce_peername)
	session->tls_enforce_peername = 1;
    else if (rcpt_policy.enforce && rcpt_policy.enforce_peername)
	session->tls_enforce_peername = 1;
    else if (
#ifdef FIX200601
	     !host_policy.enforce && !rcpt_policy.enforce &&	/* Fix 200601 */
#endif
	     var_smtp_enforce_tls && var_smtp_tls_enforce_peername)
	session->tls_enforce_peername = 1;
    else if ((var_smtp_use_tls && !host_policy.dont_use && !rcpt_policy.dont_use) || host_policy.use || rcpt_policy.use)
	session->tls_use_tls = 1;
}

static void set_global_policy(const char *global)
{
    var_smtp_tls_enforce_peername = var_smtp_enforce_tls = var_smtp_use_tls = 0;

    if (strcasecmp(global, "must") == 0) {
	var_smtp_enforce_tls = 1;		/* XXX */
	var_smtp_tls_enforce_peername = 1;
    } else if (strcasecmp(global, "must_nopeermatch") == 0) {
	var_smtp_enforce_tls = 1;
    } else if (strcasecmp(global, "may") == 0) {
	var_smtp_use_tls = 1;
    } else if (strcasecmp(global, "-") !=0) {
	msg_fatal("unknown global policy: %s", global);
    }
}

static const char *print_policy(SMTP_SESSION *session)
{
    if (session->tls_enforce_peername && session->tls_enforce_tls)
	return ("must");
    if (session->tls_enforce_tls)
	return ("must_nopeermatch");
    if (session->tls_use_tls)
	return ("may");
    return ("none");
}

int     main(int argc, char **argv)
{
    SMTP_SESSION session;
    VSTRING *buf = vstring_alloc(200);
    char   *cp;
    const char *global;
    const char *host;
    const char *dest;
    const char *result;
    const char *sep = " \t\r\n";

    vstream_printf("%-20s %-20s %-20s %s\n",
		   "host", "dest", "global", "result");
    while (vstring_get_nonl(buf, VSTREAM_IN) >= 0) {
	cp = vstring_str(buf);
	if (*cp == 0 || *cp == '#') {
	    vstream_printf("%s\n", cp);
	} else {
	    if ((host = mystrtok(&cp, sep)) == 0)
		msg_fatal("missing host policy");
	    if ((dest = mystrtok(&cp, sep)) == 0)
		msg_fatal("missing nexthop policy");
	    if ((global = mystrtok(&cp, sep)) == 0)
		msg_fatal("missing global policy");
	    if (mystrtok(&cp, sep) != 0)
		msg_fatal("garbage after global policy");
	    set_global_policy(global);
	    policy(&session, host, dest);
	    result = print_policy(&session);
	    vstream_printf("%-20s %-20s %-20s %s\n",
			   host, dest, global, result);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    exit(0);
}
