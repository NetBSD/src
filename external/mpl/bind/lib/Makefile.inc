#	$NetBSD: Makefile.inc,v 1.2 2025/01/26 16:25:52 christos Exp $

.include "../Makefile.inc"
WARNS?=		2
# XXX: Should not need these with -Ac23!
LINTFLAGS+=-Ac23
LINTFLAGS+=-X89,261	# redeclared
LINTFLAGS+=-X51	 	# number of params mismatch
LINTFLAGS+=-X55	 	# constant expression
LINTFLAGS+=-X323	# continue in do while 0 loop.
