#	$NetBSD: Makefile.inc,v 1.4.2.1 2016/08/06 00:18:41 pgoyette Exp $

NOLINT=
.include "../Makefile.inc"
CPPFLAGS+=	-D_KERNTYPES
