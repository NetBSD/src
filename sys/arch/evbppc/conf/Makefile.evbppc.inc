#	$NetBSD: Makefile.evbppc.inc,v 1.3.8.1 2005/02/06 07:45:24 jmc Exp $

CPPFLAGS+=	-D__${BOARDTYPE}__
CPPFLAGS+=	-I$S/dist/ipf

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"
