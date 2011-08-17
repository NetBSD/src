#	$NetBSD: Makefile,v 1.2 2011/08/17 22:26:40 joerg Exp $

PROG=		gptmbr.bin
SRCS=		gptmbr.S

AFLAGS.gptmbr.S= ${${ACTIVE_CC} == "clang":?-no-integrated-as:}

.include <../Makefile.mbr>

LOADADDR=	0
