/*	$NetBSD: multilib.h,v 1.1 2001/06/18 16:18:36 tv Exp $	*/
/* For the moment we don't do multilib.  Use a static empty multilib spec file. */

static char *multilib_raw[] = {
". ;",
NULL
};

static char *multilib_matches_raw[] = {
NULL
};

static char *multilib_extra = "";
