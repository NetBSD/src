/*++
/* NAME
/*	mail_error 3
/* SUMMARY
/*	mail error classes
/* SYNOPSIS
/*	#include <mail_error.h>
/*
/*	NAME_MASK mail_error_masks[];
/* DESCRIPTION
/*	This module implements error class support.
/*
/*	mail_error_masks[] is a null-terminated table with mail error
/*	class names and their corresponding bit masks.
/*
/*	The following is a list of implemented names, with the
/*	corresponding bit masks indicated in parentheses:
/* .IP "bounce (MAIL_ERROR_BOUNCE)"
/*	A message could not be delivered because it was too large,
/*	because was sent via too many hops, because the recipient
/*	does not exist, and so on.
/* .IP "2bounce (MAIL_ERROR_2BOUNCE)"
/*	A bounce message could not be delivered.
/* .IP "policy (MAIL_ERROR_POLICY)"
/*	Policy violation. This depends on what restrictions have
/*	been configured locally.
/* .IP "protocol (MAIL_ERROR_PROTOCOL)"
/*	Protocol violation. Something did not follow the appropriate
/*	standard, or something requested an unimplemented service.
/* .IP "resource (MAIL_ERROR_RESOURCE)"
/*	A message could not be delivered due to lack of system
/*	resources, for example, lack of file system space.
/* .IP "software (MAIL_ERROR_SOFTWARE)"
/*	Software bug. The author of this program made a mistake.
/*	Fixing this requires change to the software.
/* SEE ALSO
/*	name_mask(3), name to mask conversion
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

/* Global library. */

#include "mail_error.h"

 /*
  * The table that maps names to error bit masks. This will work on most UNIX
  * compilation environments.
  * 
  * In a some environments the table will not be linked in unless this module
  * also contains a function that is being called explicitly. REF/DEF and all
  * that.
  */
NAME_MASK mail_error_masks[] = {
    "bounce", MAIL_ERROR_BOUNCE,
    "2bounce", MAIL_ERROR_2BOUNCE,
    "delay", MAIL_ERROR_DELAY,
    "policy", MAIL_ERROR_POLICY,
    "protocol", MAIL_ERROR_PROTOCOL,
    "resource", MAIL_ERROR_RESOURCE,
    "software", MAIL_ERROR_SOFTWARE,
    0, 0,
};
