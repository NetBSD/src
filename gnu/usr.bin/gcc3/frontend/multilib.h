/*	$NetBSD: multilib.h,v 1.1 2003/07/25 16:32:35 mrg Exp $	*/
/* For the moment we don't do multilib.  Use a static empty multilib spec file. */

static const char *const multilib_raw[] = {
". ;",
NULL
};

static const char *const multilib_matches_raw[] = {
NULL
};

static const char *multilib_extra = "";

static const char *const multilib_exclusions_raw[] = {
NULL
};

static const char *multilib_options = "";
