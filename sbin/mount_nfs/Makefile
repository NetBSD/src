#	$NetBSD: Makefile,v 1.17 2005/06/27 01:00:06 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_nfs
SRCS=	mount_nfs.c getnfsargs.c
MAN=	mount_nfs.8

CPPFLAGS+= -DNFS

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

.if defined(notdef)			# XXX no kernel kerb NFS support!
.if defined(KERBEROS)
CPPFLAGS+=-DNFSKERB
DPADD+=	${LIBKRB} ${LIBDES} ${LIBUTIL}
LDADD+=	-lkrb -ldes -lutil
.endif
.else
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>
