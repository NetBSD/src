/*	$NetBSD: rec_attr_map.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	rec_attr_map 3
/* SUMMARY
/*	map named attribute record type to pseudo record type
/* SYNOPSIS
/*	#include <rec_attr_map.h>
/*
/*	int	rec_attr_map(attr_name)
/*	const char *attr_name;
/* DESCRIPTION
/*	rec_attr_map() maps the record type of a named attribute to
/*	a pseudo record type, if such a mapping exists. The result
/*	is the pseudo record type in case of success, 0 on failure.
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

/* Global library. */

#include <mail_proto.h>
#include <rec_type.h>
#include <rec_attr_map.h>

/* rec_attr_map - map named attribute to pseudo record type */

int     rec_attr_map(const char *attr_name)
{
    if (strcmp(attr_name, MAIL_ATTR_DSN_ORCPT) == 0) {
	return (REC_TYPE_DSN_ORCPT);
    } else if (strcmp(attr_name, MAIL_ATTR_DSN_NOTIFY) == 0) {
	return (REC_TYPE_DSN_NOTIFY);
    } else if (strcmp(attr_name, MAIL_ATTR_DSN_ENVID) == 0) {
	return (REC_TYPE_DSN_ENVID);
    } else if (strcmp(attr_name, MAIL_ATTR_DSN_RET) == 0) {
	return (REC_TYPE_DSN_RET);
    } else if (strcmp(attr_name, MAIL_ATTR_CREATE_TIME) == 0) {
	return (REC_TYPE_CTIME);
    } else {
	return (0);
    }
}
