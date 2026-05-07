#	$NetBSD: Makefile.inc,v 1.2.2.1 2026/05/07 16:18:58 martin Exp $

.include "../Makefile.inc"

WARNS?=		2

LINTFLAGS+=	-Ac23
LINTFLAGS+=	-X 323	# continue in do while 0 loop
