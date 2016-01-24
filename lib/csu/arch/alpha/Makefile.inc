#	$NetBSD: Makefile.inc,v 1.2 2016/01/24 16:27:38 christos Exp $

CPPFLAGS+=	-I${ARCHDIR} -DELFSIZE=64 -D_KERNTYPES
