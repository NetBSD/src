#	$NetBSD: Makefile,v 1.12 2002/03/22 18:10:25 thorpej Exp $

.include <bsd.own.mk>

PROG=	racoon
SRCS=	main.c session.c isakmp.c handler.c random.c \
	isakmp_ident.c isakmp_agg.c isakmp_base.c \
	isakmp_quick.c isakmp_inf.c isakmp_newg.c \
	gssapi.c \
	pfkey.c admin.c ipsec_doi.c oakley.c grabmyaddr.c \
	policy.c localconf.c remoteconf.c crypto_openssl.c algorithm.c \
	y.tab.y cftoken.l strnames.c \
	vmbuf.c plog.c logger.c schedule.c str2val.c misc.c sockmisc.c \
	proposal.c sainfo.c vendorid.c safefile.c backupsa.c \
	dnssec.c getcertsbyname.c
SRCS+=	rijndael-alg-fst.c rijndael-api-fst.c sha2.c

MAN=	racoon.8 racoon.conf.5

LIBPFKEYDIR!=	cd ${.CURDIR}/../libpfkey; ${PRINTOBJDIR}
LDADD+=	-L${LIBPFKEYDIR} -lpfkey
DPADD+=	${LIBPFKEYDIR}/libpfkey.a
.if (${USE_KERBEROS} != "no")
LDADD+=	-lgssapi -lkrb5 -lcom_err -lroken -lasn1
DPADD+=	${LIBGSSAPI} ${LIBKRB5} ${LIBCOM_ERR} ${LIBROKEN} ${LIBASN1}
.endif
LDADD+=	-lipsec -ly -ll -lcrypto
DPADD+=	${LIBIPSEC} ${LIBY} ${LIBL} ${LIBCRYPTO}

#CFLAGS+=-g
OPTFLAG+=-DHAVE_PFKEYV2 -DYIPS_DEBUG
CPPFLAGS+=-DINET6 -DHAVE_FUNCTION_MACRO=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBL=1 -DHAVE_LIBY=1 -DENABLE_IPV6=1 -DADVAPI=1 -DHAVE_GETADDRINFO=1 -DHAVE_GETNAMEINFO=1 -DSTDC_HEADERS=1 -DHAVE_SYS_WAIT_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDARG_H=1 -DHAVE_VARARGS_H=1 -DTIME_WITH_SYS_TIME=1 -DRETSIGTYPE=void -DHAVE_VPRINTF=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_SELECT=1 -DHAVE_SOCKET=1 -DHAVE_STRERROR=1 -DHAVE_STRTOL=1 -DHAVE_STRTOUL=1 -DHAVE_STRDUP=1  $(OPTFLAG) -DIPSEC -I. -I${RACOONSRCDIR} -Dss_family=__ss_family -Dss_len=__ss_len -DSYSCONFDIR=\"$(ETCDIR)\"
CPPFLAGS+=-DHAVE_OPENSSL_OPENSSLV_H=1
CPPFLAGS+=-DYY_NO_UNPUT
CPPFLAGS+=-I${LIBPFKEYSRCDIR}
CPPFLAGS+=-I${RACOONSRCDIR}/missing
.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DHAVE_GSSAPI 
.endif
YHEADER=1

# debugging code with print-isakmp.c
SRCS+=	print-isakmp.c
CPPFLAGS+=-DRACOON_PKG_VERSION="\"${PKGVERSION}\""
CPPFLAGS+=-I${TCPDUMPSRCDIR} -I${TCPDUMPSRCDIR}/lbl -DHAVE_PRINT_ISAKMP_C=1
.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-I${DESTDIR}/usr/include/krb5
.endif

# Certificate support
CPPFLAGS+=-DHAVE_SIGNING_C=1 -DHAVE_OPENSSL_PEM_H=1 -DHAVE_OPENSSL_X509_H=1

# for debugging malloc/free
#.if exists(/usr/pkg/lib/libefence.a) || exists(/usr/pkg/lib/libefence.so)
#LDADD+=-L/usr/pkg/lib -lefence
#.endif
#.if exists(/usr/pkg/lib/libleak.a)
#CPPFLAGS+= -DGC -I/usr/pkg/include
#LDADD+=-L/usr/pkg/lib -lleak
#LDSTATIC= -static
#.endif

CLEANFILES+=	y.tab.y
y.tab.y: cfparse.y
	cp $> $@

.include <bsd.prog.mk>

.PATH:	${RACOONSRCDIR} ${TCPDUMPSRCDIR} ${RACOONSRCDIR}/missing/crypto/rijndael ${RACOONSRCDIR}/missing/crypto/sha2
