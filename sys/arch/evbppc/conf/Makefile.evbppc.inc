#	$NetBSD: Makefile.evbppc.inc,v 1.3 2003/04/08 19:35:24 matt Exp $

CPPFLAGS += -D__${BOARDTYPE}__

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"
