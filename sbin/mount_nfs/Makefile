#	$NetBSD: Makefile,v 1.13 2003/03/22 12:44:00 jdolecek Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_nfs
SRCS=	mount_nfs.c
MAN=	mount_nfs.8

CPPFLAGS+= -DNFS

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
