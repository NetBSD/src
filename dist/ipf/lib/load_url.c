/*	$NetBSD: load_url.c,v 1.2 2012/02/15 17:55:06 riz Exp $	*/

/*
 * Copyright (C) 2006 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: load_url.c,v 1.1.2.2 2009/12/27 06:58:06 darrenr Exp
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
