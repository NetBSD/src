#	$NetBSD: Makefile,v 1.2.2.2 2001/08/03 04:11:05 lukem Exp $

PROG=		boot26
PROGSOURCE=	rmheader.S rmvers.c srt0.S boot26.c start.S
NEWVERSWHAT=	"RISC OS Boot"
RISCOSTYPE=	ffa

rmvers.c: ${.CURDIR}/version
	sh ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
