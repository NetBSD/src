/*	$NetBSD: mail_addr_form.c,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*	mail_addr_form 3
/* SUMMARY
/*	mail address formats
/* SYNOPSIS
/*	#include <mail_addr_form.h>
/*
/*	int	mail_addr_form_from_string(const char *addr_form_name)
/*
/*	const char *mail_addr_form_to_string(int addr_form)
/* DESCRIPTION
/*	mail_addr_form_from_string() converts a symbolic mail address
/*	form name ("internal", "external", "internal-first") into the
/*	corresponding internal code. The result is -1 if an unrecognized
/*	name was specified.
/*
/*	mail_addr_form_to_string() converts from internal code
/*	to the corresponding symbolic name. The result is null if
/*	an unrecognized code was specified.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <name_code.h>

 /*
  * Global library.
  */
#include <mail_addr_form.h>

static const NAME_CODE addr_form_table[] = {
    "external", MA_FORM_EXTERNAL,
    "internal", MA_FORM_INTERNAL,
    "external-first", MA_FORM_EXTERNAL_FIRST,
    "internal-first", MA_FORM_INTERNAL_FIRST,
    0, -1,
};

/* mail_addr_form_from_string - symbolic mail address to internal form */

int     mail_addr_form_from_string(const char *addr_form_name)
{
    return (name_code(addr_form_table, NAME_CODE_FLAG_NONE, addr_form_name));
}

const char *mail_addr_form_to_string(int addr_form)
{
    return (str_name_code(addr_form_table, addr_form));
}
