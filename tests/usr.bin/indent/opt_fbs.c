/* $NetBSD: opt_fbs.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
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

#indent input
void example(int n) {}
#indent end

#indent run -nfbs
void
example(int n) {
}
#indent end
