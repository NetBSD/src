#	$NetBSD: Makefile,v 1.1 2002/12/28 23:57:37 reinoud Exp $

PROG=		boot32
PROGSOURCE=	rmheader.S rmvers.c srt0.S boot32.c start.S
NEWVERSWHAT=	"RISC OS Boot"
RISCOSTYPE=	ffa

rmvers.c: ${.CURDIR}/version
	sh ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
