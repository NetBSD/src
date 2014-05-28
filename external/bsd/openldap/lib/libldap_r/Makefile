#	$NetBSD: Makefile,v 1.5 2014/05/28 11:32:19 tron Exp $

USE_FORT?=	yes
CWARNFLAGS.clang=	-Wno-format-extra-args

.include "../libldap/Makefile.libldap"

LIB=		ldap_r

.PATH:		${LDAP_DISTDIR}/libraries/libldap_r

SRCS+=		threads.c rdwr.c rmutex.c tpool.c rq.c \
		thr_posix.c thr_cthreads.c thr_thr.c thr_nt.c \
		thr_pth.c thr_stub.c thr_debug.c

CPPFLAGS+=	-DLDAP_R_COMPILE

COPTS+=		-pthread
LIBDPLIBS+=	pthread	${.CURDIR}/../../../../../lib/libpthread

.include <bsd.lib.mk>
