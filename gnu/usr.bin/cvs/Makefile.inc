#	$NetBSD: Makefile.inc,v 1.1 2000/09/04 21:46:56 christos Exp $

WARNS?=	0

IDIST=	${.CURDIR}/../../../dist/cvs

CPPFLAGS+= -DHAVE_CONFIG_H -I${.CURDIR}/../include -I${IDIST}/lib \
	-I${IDIST}/diff -I${IDIST}/src

DOBJDIR!=cd $(.CURDIR)/../libdiff; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

COBJDIR!=cd $(.CURDIR)/../libcvs; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

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
