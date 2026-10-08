#	$NetBSD: Makefile,v 1.9 2026/10/08 14:28:03 christos Exp $

USE_FORT?=	yes
CWARNFLAGS.clang=	-Wno-format-extra-args

.include "../libldap/Makefile.libldap"

LIB=		ldap_r

.PATH:		${LDAP_DISTDIR}/libraries/libldap_r

SRCS+=		threads.c rdwr.c rq.c \
		thr_posix.c thr_thr.c thr_nt.c \
		thr_pth.c thr_debug.c tpool.c

COPTS+=		-pthread
LIBDPLIBS+=	pthread	${.CURDIR}/../../../../../lib/libpthread

EXPSYM_SRCS+=	ldap_r.common.expsym
.if ${MKKERBEROS:Uno} != "no"
EXPSYM_SRCS+=	ldap_r.kerberos.expsym
.endif

LIB_EXPSYM=	ldap_r.expsym
${LIB_EXPSYM}: ${EXPSYM_SRCS}
	${_MKTARGET_CREATE}
	LC_ALL=C sort -m ${.ALLSRC} > ${.TARGET}.tmp && \
	${MV} ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	${LIB_EXPSYM} ${LIB_EXPSYM}.tmp

.include <bsd.lib.mk>
