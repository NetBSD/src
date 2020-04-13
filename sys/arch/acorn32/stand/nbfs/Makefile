#	$NetBSD: Makefile,v 1.3.154.1 2020/04/13 08:03:28 martin Exp $

PROG=		nbfs
PROGSOURCE=	rmheader.S rmvers.c nbfs.c rmalloc.c
NEWVERSWHAT=	"Boot Filing System"
MKMODULE=	yes

CPPFLAGS+=	-DLIBSA_SINGLE_DEVICE=rodisk_ -DLIBSA_NO_TWIDDLE

rmvers.c: ${.CURDIR}/version
	${HOST_SH} ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot FS"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

AFLAGS.rmheader.S= ${${ACTIVE_CC} == "clang":?-no-integrated-as:}

.include "../Makefile.buildboot"
