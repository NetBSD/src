# $NetBSD: Makefile,v 1.48 2025/03/08 16:40:44 christos Exp $

USE_FORT?= yes	# network client and server
WARNS=3

.include <bsd.own.mk>


PROG=	racoon
SRCS+= admin.c algorithm.c backupsa.c cfparse.y cftoken.l dnssec.c evt.c \
    getcertsbyname.c grabmyaddr.c gssapi.c handler.c ipsec_doi.c isakmp.c \
    isakmp_agg.c isakmp_base.c isakmp_cfg.c isakmp_frag.c isakmp_ident.c \
    isakmp_inf.c isakmp_newg.c isakmp_quick.c isakmp_unity.c isakmp_xauth.c \
    localconf.c main.c nattraversal.c oakley.c pfkey.c policy.c privsep.c \
    proposal.c remoteconf.c safefile.c sainfo.c schedule.c session.c \
    str2val.c strnames.c throttle.c vendorid.c

MAN=	racoon.8 racoon.conf.5

CPPFLAGS+= -DIPSEC_DEBUG -DENABLE_WILDCARD_MATCH
CPPFLAGS+= -DADMINPORTDIR=\"/var/run\"
CPPFLAGS+= -DSYSCONFDIR=\"/etc/racoon\"
.if ${HAVE_OPENSSL} < 11
CPPFLAGS+= -DHAVE_OPENSSL_RC5_H
CPPFLAGS+= -DOPENSSL_API_COMPAT=0x10100000L
.endif

LDADD+= -ll -ly -lipsec -lutil
DPADD+= ${LIBL} ${LIBY} ${LIBIPSEC} ${LIBUTIL}

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
LDADD+= -lgssapi -lheimntlm ${LIBKRB5_LDADD}
DPADD+= ${LIBGSSAPI} ${LIBHEIMNTLM} ${LIBKRB5_DPADD}
.endif
CPPFLAGS+=-DHAVE_OPENSSL_IDEA_H
.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.if (${USE_LDAP} != "no")
CPPFLAGS+=-DHAVE_LIBLDAP
LDADD+= ${LIBLDAP_LDADD}
DPADD+= ${LIBLDAP_DPADD}
.endif

LDADD+= -lcrypto -lcrypt
DPADD+= ${LIBCRYPTO} ${LIBCRYPT}

#CPPFLAGS+= -DDEBUG_RECORD_MALLOCATION
#SRCS+= debugrm.c

#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address


CWARNFLAGS.gcc+=        ${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
COPTS.cftoken.c+=-Wno-error=unused-function
COPTS.isakmp.c+=${CC_WNO_STRINGOP_TRUNCATION}
COPTS.isakmp_cfg.c+=${CC_WNO_STRINGOP_TRUNCATION} ${${ACTIVE_CC} == "gcc" :? -Wno-error=format-truncation :}

#COPTS+=		-fcommon

.include "${.CURDIR}/Makefile.racoon"
.PATH:  ${NETBSDSRCDIR}/lib/libipsec

.include <bsd.prog.mk>
