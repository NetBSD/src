/*	$NetBSD: levels.c,v 1.1.1.1.4.2 2006/11/20 13:30:52 tron Exp $	*/

 /*
  * The new legacy TLS per-site policy engine, re-implemented in terms of
  * enforcement levels, stripped down for exhaustive comparisons with the old
  * legacy policy engine.
  * 
  * This is the code that will be used in Postfix 2.3 so that sites can upgrade
  * Postfix without being forced to change to the new TLS policy model.
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
  * TLS levels
  */
#include <tls.h>

 /*
  * Application-specific.
  */
#include <smtp.h>

 /*
  * Global policy variables.
  */
int     var_smtp_enforce_tls;
int     var_smtp_tls_enforce_peername;
int     var_smtp_use_tls;

/* smtp_tls_policy_lookup - look up per-site TLS policy */

static void smtp_tls_policy_lookup(int *site_level, const char *lookup)
{

    /*
     * Look up a non-default policy. In case of multiple lookup results, the
     * precedence order is a permutation of the TLS enforcement level order:
     * VERIFY, ENCRYPT, NONE, MAY, NOTFOUND. I.e. we override MAY with a more
     * specific policy including NONE, otherwise we choose the stronger
     * enforcement level.
     */
    if (strcasecmp(lookup, "-")) {
	if (!strcasecmp(lookup, "NONE")) {
	    /* NONE overrides MAY or NOTFOUND. */
	    if (*site_level <= TLS_LEV_MAY)
		*site_level = TLS_LEV_NONE;
	} else if (!strcasecmp(lookup, "MAY")) {
	    /* MAY overrides NOTFOUND but not NONE. */
	    if (*site_level < TLS_LEV_NONE)
		*site_level = TLS_LEV_MAY;
	} else if (!strcasecmp(lookup, "MUST_NOPEERMATCH")) {
	    if (*site_level < TLS_LEV_ENCRYPT)
		*site_level = TLS_LEV_ENCRYPT;
	} else if (!strcasecmp(lookup, "MUST")) {
	    if (*site_level < TLS_LEV_VERIFY)
		*site_level = TLS_LEV_VERIFY;
	} else {
	    msg_fatal("unknown TLS policy '%s'", lookup);
	}
    }
}

static int policy(const char *host, const char *dest)
{
    int     global_level;
    int     site_level;
    int     tls_level;

    /*
     * Compute the global TLS policy. This is the default policy level when
     * no per-site policy exists. It also is used to override a wild-card
     * per-site policy.
     */
    if (var_smtp_enforce_tls)
	global_level = var_smtp_tls_enforce_peername ?
	    TLS_LEV_VERIFY : TLS_LEV_ENCRYPT;
    else
	global_level = var_smtp_use_tls ?
	    TLS_LEV_MAY : TLS_LEV_NONE;

    /*
     * Compute the per-site TLS enforcement level. For compatibility with the
     * original TLS patch, this algorithm is gives equal precedence to host
     * and next-hop policies.
     */
    site_level = TLS_LEV_NOTFOUND;

    smtp_tls_policy_lookup(&site_level, dest);
    smtp_tls_policy_lookup(&site_level, host);

    /*
     * Override a wild-card per-site policy with a more specific global
     * policy.
     * 
     * With the original TLS patch, 1) a per-site ENCRYPT could not override a
     * global VERIFY, and 2) a combined per-site (NONE+MAY) policy produced
     * inconsistent results: it changed a global VERIFY into NONE, while
     * producing MAY with all weaker global policy settings.
     * 
     * With the current implementation, a combined per-site (NONE+MAY)
     * consistently overrides global policy with NONE, and global policy can
     * override only a per-site MAY wildcard. That is, specific policies
     * consistently override wildcard policies, and (non-wildcard) per-site
     * policies consistently override global policies.
     */
    if (site_level == TLS_LEV_NOTFOUND
	|| (site_level == TLS_LEV_MAY
	    && global_level > TLS_LEV_MAY))
	tls_level = global_level;
    else
	tls_level = site_level;

    return (tls_level);
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

static const char *print_policy(int level)
{
    if (level == TLS_LEV_VERIFY)
	return ("must");
    if (level == TLS_LEV_ENCRYPT)
	return ("must_nopeermatch");
    if (level == TLS_LEV_MAY)
	return ("may");
    if (level == TLS_LEV_NONE)
	return ("none");
    msg_panic("unknown policy level %d", level);
}

int     main(int argc, char **argv)
{
    VSTRING *buf = vstring_alloc(200);
    char   *cp;
    const char *global;
    const char *host;
    const char *dest;
    const char *result;
    const char *sep = " \t\r\n";
    int     level;

    vstream_printf("%-20s %-20s %-20s %s\n",
		   "host", "dest", "global", "result");
    while (vstring_get_nonl(buf, VSTREAM_IN) > 0) {
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
	    level = policy(host, dest);
	    result = print_policy(level);
	    vstream_printf("%-20s %-20s %-20s %s\n",
			   host, dest, global, result);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    exit(0);
}
