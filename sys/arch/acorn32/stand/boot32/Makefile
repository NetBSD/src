#	$NetBSD: Makefile,v 1.1.4.1 2004/08/03 10:30:56 skrll Exp $

PROG=		boot32
PROGSOURCE=	rmheader.S rmvers.c srt0.S boot32.c start.S
NEWVERSWHAT=	"RISC OS Boot"
RISCOSTYPE=	ffa

CPPFLAGS+=	-DBOOT_MAXIMISE_MEMORY

rmvers.c: ${.CURDIR}/version
	${HOST_SH} ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
