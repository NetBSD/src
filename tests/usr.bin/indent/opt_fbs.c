/* $NetBSD: opt_fbs.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void example(int n) {}
#indent end

#indent run -fbs
void
example(int n)
{
}
#indent end

#indent run -nfbs
void
example(int n) {
}
#indent end
