#	$NetBSD: Makefile.inc,v 1.11 2004/02/07 05:10:44 lukem Exp $

.include <bsd.own.mk>

WARNS?=	0

PFIX_DISTDIR=	${NETBSDSRCDIR}/gnu/dist/postfix

CPPFLAGS+= -DNETBSD1 -I${DIST} \
	-I${PFIX_DISTDIR}/src/dns -I${PFIX_DISTDIR}/src/global \
	-I${PFIX_DISTDIR}/src/master -I${PFIX_DISTDIR}/src/util
# IPv6
#CPPFLAGS+= -DINET6

PFIX_SRCDIR=	${NETBSDSRCDIR}/gnu/usr.sbin/postfix

.for _LD _LL in	dns dns masterlib master global global util util
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

# postfix check wants a lot of files to be owned by root
BINOWN=	root

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
