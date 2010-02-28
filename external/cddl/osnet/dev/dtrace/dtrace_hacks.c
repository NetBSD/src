/*	$NetBSD: dtrace_hacks.c,v 1.3 2010/02/28 07:01:43 darran Exp $	*/

/* $FreeBSD: src/sys/cddl/dev/dtrace/dtrace_hacks.c,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $ */
/* XXX Hacks.... */

dtrace_cacheid_t dtrace_predcache_id;

int panic_quiesce;
char panic_stack[PANICSTKSIZE];

cpu_core_t	cpu_core[MAXCPUS];

boolean_t
priv_policy_only(const cred_t *a, int b, boolean_t c)
{
	return 1;
}
