#	$NetBSD: Makefile.evbppc.inc,v 1.2 2003/03/04 07:52:21 matt Exp $

CPPFLAGS += -D${BOARDTYPE}

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"
