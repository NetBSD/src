#	$NetBSD: Makefile,v 1.3 2005/12/11 12:16:05 christos Exp $

PROG=		boot26
PROGSOURCE=	rmheader.S rmvers.c srt0.S boot26.c start.S
NEWVERSWHAT=	"RISC OS Boot"
RISCOSTYPE=	ffa

rmvers.c: ${.CURDIR}/version
	${HOST_SH} ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
