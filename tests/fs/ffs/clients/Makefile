#	$NetBSD: Makefile,v 1.1.2.2 2011/01/28 22:15:36 bouyer Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

PROGS=			rump_quota rump_repquota

.PATH: ${NETBSDSRCDIR}/usr.bin/quota
SRCS.rump_quota=	quota.c printquota.c getvfsquota.c quota_rumpops.c
SRCS.rump_repquota=	printquota.c quota_rumpops.c
CPPFLAGS.quota.c+=	-I${NETBSDSRCDIR}/usr.bin/quota
CPPFLAGS.printquota.c+=	-I${NETBSDSRCDIR}/usr.bin/quota
CPPFLAGS.getvfsquota.c+=-I${NETBSDSRCDIR}/usr.bin/quota -I${NETBSDSRCDIR}/sys

.PATH: ${NETBSDSRCDIR}/usr.sbin/repquota
SRCS.rump_repquota+=	repquota.c
CPPFLAGS.repquota.c+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS.rump_quota+=	quota2_prop.c
SRCS.rump_repquota+=	quota2_prop.c quota1_subr.c
CPPFLAGS.quota2_prop.c+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota

DPADD.rump_quota=	${LIBRPCSVC} ${LIBPROP} ${LIBRUMPCLIENT}
LDADD.rump_quota=	-lrpcsvc -lprop -lrumpclient
MAN.rump_quota=	# empty
BINDIR.rump_quota=	${TESTSDIR}

DPADD.rump_repquota=	${LIBRPCSVC} ${LIBPROP} ${LIBRUMPCLIENT}
LDADD.rump_repquota=	-lrpcsvc -lprop -lrumpclient
MAN.rump_repquota=	# empty
BINDIR.rump_repquota=	${TESTSDIR}

.include <bsd.test.mk>
