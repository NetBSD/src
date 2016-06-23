/*	$NetBSD: dtrace_hacks.c,v 1.4 2016/06/23 04:35:35 pgoyette Exp $	*/

/* $FreeBSD: src/sys/cddl/dev/dtrace/dtrace_hacks.c,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $ */
/* XXX Hacks.... */

dtrace_cacheid_t dtrace_predcache_id;

cpu_core_t	cpu_core[MAXCPUS];

boolean_t
priv_policy_only(const cred_t *a, int b, boolean_t c)
{
	return 1;
}
