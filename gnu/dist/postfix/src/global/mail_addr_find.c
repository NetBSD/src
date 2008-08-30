/*++
/* NAME
/*	mail_addr_find 3
/* SUMMARY
/*	generic address-based lookup
/* SYNOPSIS
/*	#include <mail_addr_find.h>
/*
/*	const char *mail_addr_find(maps, address, extension)
/*	MAPS	*maps;
/*	const char *address;
/*	char	**extension;
/* DESCRIPTION
/*	mail_addr_find() searches the specified maps for an entry with as
/*	key the specified address, and derivations from that address.
/*	It is up to the caller to specify its case sensitivity
/*	preferences when it opens the maps.
/*	The result is overwritten upon each call.
/*
/*	An address that is in the form \fIuser\fR matches itself.
/*
/*	Given an address of the form \fIuser@domain\fR, the following
/*	lookups are done in the given order until one returns a result:
/* .IP user@domain
/*	Look up the entire address.
/* .IP user
/*	Look up \fIuser\fR when \fIdomain\fR is equal to $myorigin,
/*	when \fIdomain\fR matches $mydestination, or when it matches
/*	$inet_interfaces or $proxy_interfaces.
/* .IP @domain
/*	Look for an entry that matches the domain specified in \fIaddress\fR.
/* .PP
/*	With address extension enabled, the table lookup order is:
/*	\fIuser+extension\fR@\fIdomain\fR, \fIuser\fR@\fIdomain\fR,
/*	\fIuser+extension\fR, \fIuser\fR, and @\fIdomain\fR.
/* .PP
/*	Arguments:
/* .IP maps
/*	Dictionary search path (see maps(3)).
/* .IP address
/*	The address to be looked up.
/* .IP extension
/*	A null pointer, or the address of a pointer that is set to
/*	the address of a dynamic memory copy of the address extension
/*	that had to be chopped off in order to match the lookup tables.
/*	The copy includes the recipient address delimiter.
/*	The caller is expected to pass the copy to myfree().
/* DIAGNOSTICS
/*	The global \fIdict_errno\fR is non-zero when the lookup
/*	should be tried again.
/* SEE ALSO
/*	maps(3), multi-dictionary search
/*	resolve_local(3), recognize local system
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
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <dict.h>
#include <stringops.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <strip_addr.h>
#include <mail_addr_find.h>
#include <resolve_local.h>

/* Application-specific. */

#define STR	vstring_str

/* mail_addr_find - map a canonical address */

const char *mail_addr_find(MAPS *path, const char *address, char **extp)
{
    const char *myname = "mail_addr_find";
    const char *result;
    char   *ratsign = 0;
    char   *full_key;
    char   *bare_key;
    char   *saved_ext;

    /*
     * Initialize.
     */
    full_key = mystrdup(address);
    if (*var_rcpt_delim == 0) {
	bare_key = saved_ext = 0;
    } else {
	bare_key = strip_addr(full_key, &saved_ext, *var_rcpt_delim);
    }

    /*
     * Try user+foo@domain and user@domain.
     * 
     * Specify what keys are partial or full, to avoid matching partial
     * addresses with regular expressions.
     */
#define FULL	0
#define PARTIAL	DICT_FLAG_FIXED

    if ((result = maps_find(path, full_key, FULL)) == 0 && dict_errno == 0
      && bare_key != 0 && (result = maps_find(path, bare_key, PARTIAL)) != 0
	&& extp != 0) {
	*extp = saved_ext;
	saved_ext = 0;
    }

    /*
     * Try user+foo@$myorigin, user+foo@$mydestination or
     * user+foo@[${proxy,inet}_interfaces]. Then try with +foo stripped off.
     */
    if (result == 0 && dict_errno == 0
	&& (ratsign = strrchr(full_key, '@')) != 0
	&& (strcasecmp(ratsign + 1, var_myorigin) == 0
	    || resolve_local(ratsign + 1))) {
	*ratsign = 0;
	result = maps_find(path, full_key, PARTIAL);
	if (result == 0 && dict_errno == 0 && bare_key != 0) {
	    if ((ratsign = strrchr(bare_key, '@')) == 0)
		msg_panic("%s: bare key botch", myname);
	    *ratsign = 0;
	    if ((result = maps_find(path, bare_key, PARTIAL)) != 0 && extp != 0) {
		*extp = saved_ext;
		saved_ext = 0;
	    }
	}
	*ratsign = '@';
    }

    /*
     * Try @domain.
     */
    if (result == 0 && dict_errno == 0 && ratsign)
	result = maps_find(path, ratsign, PARTIAL);

    /*
     * Clean up.
     */
    if (msg_verbose)
	msg_info("%s: %s -> %s", myname, address,
		 result ? result :
		 dict_errno ? "(try again)" :
		 "(not found)");
    myfree(full_key);
    if (bare_key)
	myfree(bare_key);
    if (saved_ext)
	myfree(saved_ext);

    return (result);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read an address from stdin, and spit out
  * the lookup result.
  */
#include <vstream.h>
#include <vstring_vstream.h>
#include <mail_conf.h>

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(100);
    MAPS   *path;
    const char *result;
    char   *extent;

    /*
     * Parse JCL.
     */
    if (argc != 2)
	msg_fatal("usage: %s database", argv[0]);
    msg_verbose = 1;

    /*
     * Initialize.
     */
    mail_conf_read();
    path = maps_create(argv[0], argv[1], DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	extent = 0;
	result = mail_addr_find(path, STR(buffer), &extent);
	vstream_printf("%s -> %s (%s)\n", STR(buffer), result ? result :
		       dict_errno ? "(try again)" :
		       "(not found)", extent ? extent : "null extension");
	vstream_fflush(VSTREAM_OUT);
	if (extent)
	    myfree(extent);
    }
    vstring_free(buffer);

    maps_free(path);
    return (0);
}

#endif
