#	$NetBSD: Makefile,v 1.1 2002/03/24 15:47:25 bjh21 Exp $

PROG=		boot26
PROGSOURCE=	rmheader.S rmvers.c srt0.S boot26.c start.S
NEWVERSWHAT=	"RISC OS Boot"
RISCOSTYPE=	ffa

rmvers.c: ${.CURDIR}/version
	sh ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
