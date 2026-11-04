#	$NetBSD: Makefile.inc,v 1.1.16.2 2026/11/04 23:18:26 perseant Exp $

.include "../Makefile.inc"

WARNS?=		2

LINTFLAGS+=	-Ac23
LINTFLAGS+=	-X 323	# continue in do while 0 loop
