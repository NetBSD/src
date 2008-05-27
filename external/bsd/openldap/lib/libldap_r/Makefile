#	$NetBSD: Makefile,v 1.2 2008/05/27 08:58:26 lukem Exp $

USE_FORT?=	yes

.include "../libldap/Makefile.libldap"

LIB=		ldap_r

.PATH:		${LDAP_DISTDIR}/libraries/libldap_r

SRCS+=		threads.c rdwr.c rmutex.c tpool.c rq.c \
		thr_posix.c thr_cthreads.c thr_thr.c thr_lwp.c thr_nt.c \
		thr_pth.c thr_stub.c thr_debug.c

CPPFLAGS+=	-DLDAP_R_COMPILE

COPTS+=		-pthread
LIBDPLIBS+=	pthread	${NETBSDSRCDIR}/lib/libpthread

.include <bsd.lib.mk>
