#	$NetBSD: Makefile.inc,v 1.2 2001/01/09 08:52:58 enami Exp $

.include <bsd.own.mk>

WARNS?=	0

IDIST=	${.CURDIR}/../../../dist/cvs

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/lib \
	-I${IDIST}/diff -I${IDIST}/src

DOBJDIR!=cd $(.CURDIR)/../libdiff && ${PRINTOBJDIR}
COBJDIR!=cd $(.CURDIR)/../libcvs && ${PRINTOBJDIR}

LIBDIFF=	${DOBJDIR}/libdiff.a
LIBCVS=		${COBJDIR}/libcvs.a

# Directories to install into.
#PFIX_LIBEXECDIR=/usr/libexec/postfix
#PFIX_ETCDIR=	/etc/postfix
#PFIX_EXAMPLEDIR=/usr/share/examples/postfix
#PFIX_SBINDIR=	/usr/sbin

# postfix check wants a lot of files to be owned by root
#BINOWN=	root

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
