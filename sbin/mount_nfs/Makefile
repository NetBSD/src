#	$NetBSD: Makefile,v 1.14 2003/04/11 17:39:09 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_nfs
SRCS=	mount_nfs.c
MAN=	mount_nfs.8

CPPFLAGS+= -DNFS -DINET6

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
