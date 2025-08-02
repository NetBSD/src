/*	$NetBSD: mystrerror.c,v 1.2.4.2 2025/08/02 05:50:19 perseant Exp $	*/

/*++
/* NAME
/*	mystrerror 3
/* SUMMARY
/*	convert error number to string
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	const char *mystrerror()
/*	int	errnum)
/* DESCRIPTION
/*	mystrerror() maps an error number to string. Unlike strerror(3)
/*	it returns "Application error" instead of "Success".
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
#include <string.h>

 /*
  * Utility library.
  */
#include <stringops.h>

/* mystrerror - convert error number to string */

const char *mystrerror(int errnum)
{
    return (errnum ? strerror(errnum) : "Application error");
}
