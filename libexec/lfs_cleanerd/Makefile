#	$NetBSD: Makefile,v 1.12.12.1 2009/05/13 19:18:42 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=3		# XXX: too many sign-compare issues

.include <bsd.own.mk>

PROG=	lfs_cleanerd
SRCS=	lfs_cleanerd.c fdfs.c coalesce.c cleansrv.c
SRCS+=  lfs_cksum.c
SRCS+=	bufcache.c vnode.c lfs.c # segwrite.c
MAN=	lfs_cleanerd.8
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${NETBSDSRCDIR}/sbin/fsck_lfs

FSCK_LFS= ${NETBSDSRCDIR}/sbin/fsck_lfs
DPADD=  ${LIBUTIL}
LDADD=  -lutil
CPPFLAGS+=-I${FSCK_LFS} "-Dmutex_enter(x)=" "-Dmutex_exit(x)=" # -DUSE_CLIENT_SERVER 
CPPFLAGS+="-DKASSERT(x)="
#CPPFLAGS+=-DREPAIR_ZERO_FINFO
#CPPFLAGS+=-DTEST_PATTERN

BINDIR=		/libexec

.include <bsd.prog.mk>
