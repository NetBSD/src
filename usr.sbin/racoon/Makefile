# $NetBSD: Makefile,v 1.21.10.1 2009/05/13 19:20:36 jym Exp $

WARNS?=	0	# XXX third-party program, many issues

.include <bsd.own.mk>

USE_FORT?= yes	# network client and server

PROG=	racoon
SRCS=	main.c session.c isakmp.c handler.c 			\
	isakmp_ident.c isakmp_agg.c isakmp_base.c 		\
	isakmp_quick.c isakmp_inf.c isakmp_newg.c 		\
	gssapi.c dnssec.c getcertsbyname.c privsep.c		\
	pfkey.c admin.c evt.c ipsec_doi.c oakley.c 		\
	grabmyaddr.c vendorid.c policy.c localconf.c 		\
	remoteconf.c crypto_openssl.c algorithm.c		\
	proposal.c sainfo.c strnames.c plog.c logger.c 		\
	schedule.c str2val.c safefile.c backupsa.c 		\
	genlist.c rsalist.c cftoken.l cfparse.y 		\
	prsa_tok.l prsa_par.y isakmp_xauth.c isakmp_cfg.c 	\
	isakmp_unity.c throttle.c isakmp_frag.c 		\
	nattraversal.c vmbuf.c sockmisc.c misc.c
MAN=	racoon.8 racoon.conf.5

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
CPPFLAGS+= -I${DIST}/src/racoon -I${DIST}/src/racoon/missing
CPPFLAGS+= -I${DIST}/src/libipsec
CPPFLAGS+= -I${NETBSDSRCDIR}/lib/libipsec -I.
CPPFLAGS+= -DIPSEC_DEBUG -DHAVE_CONFIG_H
CPPFLAGS+= -DADMINPORTDIR=\"/var/run\"
CPPFLAGS+= -DSYSCONFDIR=\"/etc/racoon\"
CPPFLAGS+= -DSADB_X_EALG_AESCBC=SADB_X_EALG_AES

LDADD+= -ll -ly -lipsec -lutil
DPADD+= ${LIBL} ${LIBY} ${LIBCRYPTO} ${LIBUTIL}

YHEADER=cfparse.h

CPPFLAGS+=-DHAVE_LIBRADIUS
LDADD+=-lradius
DPADD+= ${LIBRADIUS}
.if (${USE_PAM} != "no")
CPPFLAGS+=-DHAVE_LIBPAM
LDADD+=	-lpam -lutil ${PAM_STATIC_LDADD}
DPADD+=	${LIBPAM} ${PAM_STATIC_DPADD}
.endif
.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DHAVE_GSSAPI
LDADD+= -lgssapi -lkrb5 -lhx509 -lheimntlm -lcom_err \
	-lroken -lasn1
DPADD+= ${LIBGSSAPI} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBCOM_ERR} \
	${LIBROKEN} ${LIBASN1}
.endif
.if (${MKCRYPTO_IDEA} != "no")
CPPFLAGS+=-DHAVE_OPENSSL_IDEA_H
LDADD+= -lcrypto_idea
LDADD+= ${LIBCRYPTO_IDEA}
.endif
.if (${MKCRYPTO_RC5} != "no")
CPPFLAGS+=-DHAVE_OPENSSL_RC5_H
LDADD+= -lcrypto_rc5
LDADD+= ${LIBCRYPTO_RC5}
.endif
.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

LDADD+= -lcrypto -lcrypt
DPADD+= ${LIBIPSEC} ${LIBCRYPT}

.PATH:  ${NETBSDSRCDIR}/lib/libipsec ${DIST}/src/racoon

prsa_tok.c: ${DIST}/src/racoon/prsa_tok.l
	${LEX} -Pprsa -o${.TARGET} ${.ALLSRC}

prsa_par.c: ${DIST}/src/racoon/prsa_par.y
	${YACC} -pprsa -d -o ${.TARGET} ${.ALLSRC}

.include <bsd.prog.mk>
