#	$NetBSD: Makefile.inc,v 1.3 2026/04/19 14:59:54 rillig Exp $

.include "../Makefile.inc"
WARNS?=		2
# XXX: Should not need these with -Ac23!
LINTFLAGS+=-Ac23
LINTFLAGS+=-X89,261	# redeclared
LINTFLAGS+=-X323	# continue in do while 0 loop.
