# $NetBSD: Makefile.inc,v 1.5.2.1 1999/08/22 17:22:27 he Exp $
# Original from Freebsd, no rcs id.

LIBINSTALL != cd ${.CURDIR}/../lib;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../lib ${DEBUG} -DBINDIR='"${BINDIR}"'

.if !defined(OS)
OS!= uname -s
.endif

.if ${OS} == "SunOS.5"
.if exists(/etc/mk.conf)
.include "/etc/mk.conf"
.endif
LOCALBASE?= /usr/local
TAR_CMD?= gtar
TAR_FULLPATHNAME?= ${LOCALBASE}/bin/${TAR_CMD}
CPPFLAGS+=-DTAR_CMD='"${TAR_CMD}"' -DTAR_FULLPATHNAME='"${TAR_FULLPATHNAME}"'
BINDIR=${LOCALBASE}/bsd/bin
.endif

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
