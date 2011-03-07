#	$NetBSD: Makefile,v 1.3 2011/03/07 03:29:26 enami Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

PROGS=			rump_quota rump_repquota rump_edquota rump_quotactl

.PATH: ${NETBSDSRCDIR}/usr.bin/quota
SRCS.rump_quota=	quota.c printquota.c getvfsquota.c quota_rumpops.c \
			    quotautil.c
SRCS.rump_repquota=	printquota.c quota_rumpops.c quotautil.c
SRCS.rump_edquota=	printquota.c getvfsquota.c quota_rumpops.c
CPPFLAGS.quota.c+=	-I${NETBSDSRCDIR}/usr.bin/quota
CPPFLAGS.printquota.c+=	-I${NETBSDSRCDIR}/usr.bin/quota
CPPFLAGS.getvfsquota.c+=-I${NETBSDSRCDIR}/usr.bin/quota -I${NETBSDSRCDIR}/sys

.PATH: ${NETBSDSRCDIR}/usr.sbin/repquota
SRCS.rump_repquota+=	repquota.c
CPPFLAGS.repquota.c+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota

.PATH: ${NETBSDSRCDIR}/usr.sbin/edquota
SRCS.rump_edquota+=	edquota.c quotautil.c
CPPFLAGS.edquota.c+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota

.PATH: ${NETBSDSRCDIR}/usr.sbin/quotactl
SRCS.rump_quotactl+=	quotactl.c quota_rumpops.c
CPPFLAGS.quotactl.c+=	-I${NETBSDSRCDIR}/sys

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS.rump_quota+=	quota2_prop.c quota2_subr.c quota1_subr.c
SRCS.rump_repquota+=	quota2_prop.c quota2_subr.c quota1_subr.c
SRCS.rump_edquota+=	quota2_prop.c quota1_subr.c
SRCS.rump_quotactl+=	quota2_prop.c
CPPFLAGS.quota2_prop.c+=-I${NETBSDSRCDIR}/sys
CPPFLAGS.quota1_subr.c+=-I${NETBSDSRCDIR}/sys
CPPFLAGS.quota2_subr.c+=-I${NETBSDSRCDIR}/sys

DPADD.rump_quota=	${LIBRPCSVC} ${LIBPROP} ${LIBRUMPCLIENT}
LDADD.rump_quota=	-lrpcsvc -lprop -lrumpclient
MAN.rump_quota=	# empty
BINDIR.rump_quota=	${TESTSDIR}

DPADD.rump_repquota=	${LIBRPCSVC} ${LIBPROP} ${LIBRUMPCLIENT}
LDADD.rump_repquota=	-lrpcsvc -lprop -lrumpclient
MAN.rump_repquota=	# empty
BINDIR.rump_repquota=	${TESTSDIR}

DPADD.rump_edquota=	${LIBRPCSVC} ${LIBPROP} ${LIBRUMPCLIENT}
LDADD.rump_edquota=	-lrpcsvc -lprop -lrumpclient
MAN.rump_edquota=	# empty
BINDIR.rump_edquota=	${TESTSDIR}

DPADD.rump_quotactl=	${LIBRPCSVC} ${LIBPROP} ${LIBRUMPCLIENT}
LDADD.rump_quotactl=	-lrpcsvc -lprop -lrumpclient
MAN.rump_quotactl=	# empty
BINDIR.rump_quotactl=	${TESTSDIR}

.include <bsd.test.mk>
