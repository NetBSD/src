#	$NetBSD: Makefile.inc,v 1.4 2026/04/19 16:36:47 rillig Exp $

.include "../Makefile.inc"

WARNS?=		2

LINTFLAGS+=	-Ac23
LINTFLAGS+=	-X 323	# continue in do while 0 loop
