/*	$NetBSD: mac_midna.c,v 1.1.1.1 2026/05/09 18:39:23 christos Exp $	*/

/*++
/* NAME
/*	mac_midna 3h
/* SUMMARY
/*	IDNA-based mac_expand() plugin
/* SYNOPSIS
/*	#include <mac_midna.h>
/*
/*	void	mac_midna_register(void)
/* DESCRIPTION
/*	mac_midna_register() registers the domain_to_ascii{} and
/*	domain_to_utf8{} caller-defined functions for mac_expand().
/* SEE ALSO
/*	mac_midna_domain(3)
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <mac_expand.h>
#include <mac_midna.h>
#include <mac_parse.h>
#include <midna_domain.h>
#include <msg.h>
#include <stringops.h>
#include <vstring.h>

#define NAME_TO_A_LABEL	"domain_to_ascii"
#define NAME_TO_U_LABEL	"domain_to_utf8"

static int mac_midna_domain_to_ascii_eval(VSTRING *out, const char *name)
{
    const char *aname;

#ifndef NO_EAI
    if (!allascii(name)) {
	if ((aname = midna_domain_to_ascii(name)) == 0) {
	    msg_warn("bad domain argument in: '%s{%s}'", NAME_TO_A_LABEL, name);
	    return (MAC_PARSE_ERROR);
	}
	if (msg_verbose)
	    msg_info("%s: %s asciified to %s", __func__, name, aname);
    } else
#endif
	aname = name;
    if (out)
	vstring_strcat(out, aname);
    return (MAC_PARSE_OK);
}

static int mac_midna_to_u_label_eval(VSTRING *out, const char *name)
{
    const char *uname;

#ifndef NO_EAI
    if (allascii(name)) {
	if ((uname = midna_domain_to_utf8(name)) == 0) {
	    msg_warn("bad domain argument in: '%s{%s}'", NAME_TO_U_LABEL, name);
	    return (MAC_PARSE_ERROR);
	}
	if (msg_verbose)
	    msg_info("%s: %s internationalzied to %s", __func__, name, uname);
    } else
#endif
	uname = name;
    if (out)
	vstring_strcat(out, uname);
    return (MAC_PARSE_OK);
}

/* mac_midna_register - register caller-defined function */
void    mac_midna_register(void)
{
    static int first = 1;

    if (first) {
	mac_expand_add_named_fn(NAME_TO_A_LABEL, mac_midna_domain_to_ascii_eval);
	mac_expand_add_named_fn(NAME_TO_U_LABEL, mac_midna_to_u_label_eval);
	first = 0;
    }
}
