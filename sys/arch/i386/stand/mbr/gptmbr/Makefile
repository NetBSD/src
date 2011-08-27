#	$NetBSD: Makefile,v 1.2.2.2 2011/08/27 15:59:49 jym Exp $

PROG=		gptmbr.bin
SRCS=		gptmbr.S

AFLAGS.gptmbr.S= ${${ACTIVE_CC} == "clang":?-no-integrated-as:}

.include <../Makefile.mbr>

LOADADDR=	0
