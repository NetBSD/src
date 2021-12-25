#	$NetBSD: Makefile.inc,v 1.31 2021/12/25 13:30:42 rillig Exp $

.include <bsd.own.mk>

# dist/src/util/attr.h(90)
# dist/src/util/vstream.h(181)
# dist/src/util/vstring.h(63)
LINTFLAGS+=	-X 0		# empty declaration

USE_FORT?= yes	# network client and server
CWARNFLAGS.clang+=	-Wno-empty-body -Wno-format-extra-args \
			-Wno-string-plus-int -Wno-string-compare
WARNS?=	0

PFIX_DISTDIR=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist

# Many issues need to be resolved.
COPTS+=	-fcommon

CPPFLAGS+= -DNETBSD7 -DUSE_SASL_AUTH -DNO_EAI \
	-I${DIST} \
	-I${PFIX_DISTDIR}/src/dns -I${PFIX_DISTDIR}/src/global \
	-I${PFIX_DISTDIR}/src/master -I${PFIX_DISTDIR}/src/util \
	-I${PFIX_DISTDIR}/src/tls -I${PFIX_DISTDIR}/src/milter \
	-I${PFIX_DISTDIR}/src/xsasl \
	-Wno-comment

CPPFLAGS+= -DUSE_SASL_AUTH
CPPFLAGS+= -DDEF_SERVER_SASL_TYPE=\"dovecot\"

CPPFLAGS+= -DUSE_SASLC_SASL
CPPFLAGS+= -DDEF_CLIENT_SASL_TYPE=\"saslc\"
DPADD+= ${LIBSASCL} ${LIBSSL}
LDADD+= -lsaslc -lssl
.if (${MKKERBEROS} != "no")
DPADD+= ${LIBGSSAPI} ${LIBCRYPTO}
LDADD+= -lgssapi -lcrypto
.endif

.if defined(HAVE_PCC)
# code uses gcc-specific aggregate dynamic array
CPPFLAGS+=	-DCANT_USE_SEND_RECV_MSG
.endif

# TLS support.
CPPFLAGS+=	-DUSE_TLS

# IPv6 support.
.if ${MKINET6} == "no"
CPPFLAGS+=	-DNO_IPV6
.endif

# SQLite support.
CPPFLAGS+=	-DHAS_SQLITE
. if defined(PROG)
DPADD+=		${LIBSQLITE3} ${LIBM}
LDADD+=		-lsqlite3 -lm
.endif

# LDAP support.
.if ${USE_LDAP} != "no"
CPPFLAGS+=	-DHAS_LDAP

# Automatically link in libldap for a program.
. if defined(PROG)
LDADD+=		${LIBLDAP_LDADD}
DPADD+=		${LIBLDAP_DPADD}
. endif
.endif # USE_LDAP != no

PFIX_SRCDIR=	${NETBSDSRCDIR}/external/ibm-public/postfix
PFIX_LIBSDIRS=	dns dns masterlib master global global util util \
		milter milter xsasl xsasl

PFIX_LIBSDIRS+=	tls tls

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
PFIX_HTMLDIR=	/usr/share/doc/reference/ref8/postfix
PFIX_SBINDIR=	/usr/sbin

# override defaults which are otherwise empty and/or so postconf is correct
CPPFLAGS+=	-DDEF_HTML_DIR=\"${PFIX_HTMLDIR}\" \
		-DDEF_README_DIR=\"${PFIX_EXAMPLEDIR}\" \
		-DDEF_SAMPLE_DIR=\"${PFIX_EXAMPLEDIR}\" \
		-DDEF_MANPAGE_DIR=\"${MANDIR}\"

# postfix check wants a lot of files to be owned by root
BINOWN=	root
