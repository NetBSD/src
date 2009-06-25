#	$NetBSD: Makefile.inc,v 1.2 2009/06/25 18:21:50 tron Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client and server

WARNS?=	0

PFIX_DISTDIR=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist

CPPFLAGS+= -DNETBSD4 -DUSE_SASL_AUTH -DDEF_SERVER_SASL_TYPE=\"dovecot\" \
	-I${DIST} \
	-I${PFIX_DISTDIR}/src/dns -I${PFIX_DISTDIR}/src/global \
	-I${PFIX_DISTDIR}/src/master -I${PFIX_DISTDIR}/src/util \
	-I${PFIX_DISTDIR}/src/tls -I${PFIX_DISTDIR}/src/milter \
	-I${PFIX_DISTDIR}/src/xsasl

.if defined(HAVE_PCC)
# code uses gcc-specific aggregate dynamic array
CPPFLAGS+=	-DCANT_USE_SEND_RECV_MSG
.endif

# TLS support.
.if ${MKCRYPTO} != "no"
CPPFLAGS+=	-DUSE_TLS
.endif

# IPv6 support.
.if ${MKINET6} == "no"
CPPFLAGS+=	-DNO_IPV6
.endif

# LDAP support.
.if ${USE_LDAP} != "no"
CPPFLAGS+=	-DHAS_LDAP

# Automatically link in libldap for a program.
. if defined(PROG)
LDADD+=		-lldap
DPADD+=		${LIBLDAP}
.  if (${MKPIC} == "no" || (defined(LDSTATIC) && ${LDSTATIC} != ""))
LDADD+=		-llber
DPADD+=		${LIBLBER}
.   if ${MKCRYPTO} != "no" 
DPADD+=		${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lssl -lcrypto
.   endif
.  endif
. endif
.endif # USE_LDAP != no

PFIX_SRCDIR=	${NETBSDSRCDIR}/external/ibm-public/postfix
PFIX_LIBSDIRS=	dns dns masterlib master global global util util \
		milter milter xsasl xsasl

.if ${MKCRYPTO} != "no" 
PFIX_LIBSDIRS+=	tls tls
.endif

.for _LD _LL in	${PFIX_LIBSDIRS}
.if !defined(POBJDIR.${_LL})
POBJDIR.${_LL}!=cd ${PFIX_SRCDIR}/lib/${_LD} && ${PRINTOBJDIR}
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
