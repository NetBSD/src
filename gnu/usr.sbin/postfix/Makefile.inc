#	$NetBSD: Makefile.inc,v 1.15.4.1 2007/06/16 17:02:19 snj Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client and server

WARNS?=	0

PFIX_DISTDIR=	${NETBSDSRCDIR}/gnu/dist/postfix

CPPFLAGS+= -DNETBSD3 -DUSE_SASL_AUTH -DDEF_SERVER_SASL_TYPE=\"dovecot\" \
	-I${DIST} \
	-I${PFIX_DISTDIR}/src/dns -I${PFIX_DISTDIR}/src/global \
	-I${PFIX_DISTDIR}/src/master -I${PFIX_DISTDIR}/src/util \
	-I${PFIX_DISTDIR}/src/tls -I${PFIX_DISTDIR}/src/milter \
	-I${PFIX_DISTDIR}/src/xsasl

# TLS support.
.if ${MKCRYPTO} != "no"
CPPFLAGS+=	-DUSE_TLS
.endif

# IPv6 support.
.if ${MKINET6} == "no"
CPPFLAGS+=	-DNO_IPV6
.endif

PFIX_SRCDIR=	${NETBSDSRCDIR}/gnu/usr.sbin/postfix
PFIX_LIBSDIRS=	dns dns masterlib master global global util util \
		milter milter xsasl xsasl

.if ${MKCRYPTO} != "no" 
PFIX_LIBSDIRS+=	tls tls
.endif

.for _LD _LL in	${PFIX_LIBSDIRS}
.if !defined(POBJDIR.${_LL})
POBJDIR.${_LL}!=cd ${PFIX_SRCDIR}/${_LD} && ${PRINTOBJDIR}
.MAKEOVERRIDES+=POBJDIR.${_LL}
.endif
LIBP${_LL:tu}=	${POBJDIR.${_LL}}/lib${_LL}.a
.endfor


# Directories to install into.
#
PFIX_LIBEXECDIR=/usr/libexec/postfix
PFIX_ETCDIR=	/etc/postfix
PFIX_EXAMPLEDIR=/usr/share/examples/postfix
PFIX_HTMLDIR=	/usr/share/doc/html/postfix
PFIX_SBINDIR=	/usr/sbin

# override defaults which are otherwise empty and/or so postconf is correct
CPPFLAGS+=	-DDEF_HTML_DIR=\"${PFIX_HTMLDIR}\" \
		-DDEF_README_DIR=\"${PFIX_EXAMPLEDIR}\" \
		-DDEF_SAMPLE_DIR=\"${PFIX_EXAMPLEDIR}\" \
		-DDEF_MANPAGE_DIR=\"${MANDIR}\"

# postfix check wants a lot of files to be owned by root
BINOWN=	root

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
