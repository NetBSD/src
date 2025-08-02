#	$NetBSD: Makefile.inc,v 1.1.16.1 2025/08/02 05:54:20 perseant Exp $

.include "../Makefile.inc"
WARNS?=		2
# XXX: Should not need these with -Ac23!
LINTFLAGS+=-Ac23
LINTFLAGS+=-X89,261	# redeclared
LINTFLAGS+=-X51	 	# number of params mismatch
LINTFLAGS+=-X55	 	# constant expression
LINTFLAGS+=-X323	# continue in do while 0 loop.
