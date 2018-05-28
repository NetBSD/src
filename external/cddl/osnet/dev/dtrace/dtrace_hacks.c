/*	$NetBSD: dtrace_hacks.c,v 1.6 2018/05/28 21:05:03 chs Exp $	*/

/* $FreeBSD: head/sys/cddl/dev/dtrace/dtrace_hacks.c 281916 2015-04-24 03:19:30Z markj $ */
/* XXX Hacks.... */

dtrace_cacheid_t dtrace_predcache_id;

boolean_t
priv_policy_only(const cred_t *cr, int b, boolean_t c)
{

	return kauth_authorize_generic(cr, KAUTH_GENERIC_ISSUSER, NULL) == 0;
}
