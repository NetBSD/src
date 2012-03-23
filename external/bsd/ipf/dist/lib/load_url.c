/*	$NetBSD: load_url.c,v 1.1.1.1 2012/03/23 21:20:09 christos Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: load_url.c,v 1.3.2.1 2012/01/26 05:44:26 darren_r Exp 
 */

#include "ipf.h"

alist_t *
load_url(char *url)
{
	alist_t *hosts = NULL;

	if (strncmp(url, "file://", 7) == 0) {
		/*
		 * file:///etc/passwd
		 *        ^------------s
		 */
		hosts = load_file(url);

	} else if (*url == '/' || *url == '.') {
		hosts = load_file(url);

	} else if (strncmp(url, "http://", 7) == 0) {
		hosts = load_http(url);
	}

	return hosts;
}
