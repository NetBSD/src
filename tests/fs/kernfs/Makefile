#	$NetBSD: Makefile,v 1.1 2010/03/31 19:14:30 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/kernfs
WARNS=		4

TESTS_C=	t_basic

# use kernel module binary on i386
.if ${MACHINE} == "i386"
OSRELEASE!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh
CPPFLAGS+=	-DMODDIR=\"/stand/${MACHINE}/${OSRELEASE}/modules/kernfs/\"
CPPFLAGS+=	-DMODBASE=\"kernfs.kmod\"
.else
LDADD+=	-lrumpfs_kernfs						# fs driver
.endif
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>
