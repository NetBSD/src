#	$NetBSD: Makefile.inc,v 1.4 2001/03/13 18:13:16 itojun Exp $

.include <bsd.own.mk>

WARNS?=	0

IDIST=	${.CURDIR}/../../../dist/postfix

CPPFLAGS+= -DNETBSD1 -I${DIST} \
	-I${IDIST}/src/dns -I${IDIST}/src/global -I${IDIST}/src/master \
	-I${IDIST}/src/util

DOBJDIR!=	cd $(.CURDIR)/../dns && ${PRINTOBJDIR}
LIBPDNS=	${DOBJDIR}/libdns.a

MOBJDIR!=	cd $(.CURDIR)/../masterlib && ${PRINTOBJDIR}
LIBPMASTER=	${MOBJDIR}/libmaster.a

GOBJDIR!=	cd $(.CURDIR)/../global && ${PRINTOBJDIR}
LIBPGLOBAL=	${GOBJDIR}/libglobal.a

UOBJDIR!=	cd $(.CURDIR)/../util && ${PRINTOBJDIR}
LIBPUTIL=	${UOBJDIR}/libutil.a

# Directories to install into.
PFIX_LIBEXECDIR=/usr/libexec/postfix
PFIX_ETCDIR=	/etc/postfix
PFIX_EXAMPLEDIR=/usr/share/examples/postfix
PFIX_SBINDIR=	/usr/sbin

# postfix check wants a lot of files to be owned by root
BINOWN=	root

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
