#	$NetBSD: Makefile,v 1.1.2.2 2006/04/11 11:53:25 yamt Exp $

PROG=		nbfs
PROGSOURCE=	rmheader.S rmvers.c nbfs.c rmalloc.c
NEWVERSWHAT=	"RISC OS Boot"
MKMODULE=	yes

CPPFLAGS+=	-DLIBSA_SINGLE_DEVICE=rodisk_

rmvers.c: ${.CURDIR}/version
	${HOST_SH} ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot FS"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
