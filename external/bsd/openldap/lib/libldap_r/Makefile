#	$NetBSD: Makefile,v 1.6 2021/08/14 16:15:04 christos Exp $

USE_FORT?=	yes
CWARNFLAGS.clang=	-Wno-format-extra-args

.include "../libldap/Makefile.libldap"

LIB=		ldap_r

.PATH:		${LDAP_DISTDIR}/libraries/libldap_r

SRCS+=		threads.c rdwr.c rq.c \
		thr_posix.c thr_thr.c thr_nt.c \
		thr_pth.c thr_debug.c

COPTS+=		-pthread
LIBDPLIBS+=	pthread	${.CURDIR}/../../../../../lib/libpthread

.include <bsd.lib.mk>
