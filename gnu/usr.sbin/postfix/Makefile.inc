#	$NetBSD: Makefile.inc,v 1.3 2001/01/09 08:53:00 enami Exp $

.include <bsd.own.mk>

WARNS?=	0

IDIST=	${.CURDIR}/../../../dist/postfix

CPPFLAGS+= -DNETBSD1 -I${DIST} \
	-I${IDIST}/dns -I${IDIST}/global -I${IDIST}/master \
	-I${IDIST}/util

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
