#	$NetBSD: Makefile.inc,v 1.1 2000/04/30 21:21:51 perry Exp $

WARNS?=	0

IDIST=	${.CURDIR}/../../../dist/postfix

CPPFLAGS+= -DNETBSD1 -I${DIST} \
	-I${IDIST}/dns -I${IDIST}/global -I${IDIST}/master \
	-I${IDIST}/util

DOBJDIR!=cd $(.CURDIR)/../dns; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

MOBJDIR!=cd $(.CURDIR)/../masterlib; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

GOBJDIR!=cd $(.CURDIR)/../global; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

UOBJDIR!=cd $(.CURDIR)/../util; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

LIBPDNS=	${DOBJDIR}/libdns.a
LIBPMASTER=	${MOBJDIR}/libmaster.a
LIBPGLOBAL=	${GOBJDIR}/libglobal.a
LIBPUTIL=	${UOBJDIR}/libutil.a

# Directories to install into.
PFIX_LIBEXECDIR=/usr/libexec/postfix
PFIX_ETCDIR=	/etc/postfix
PFIX_EXAMPLEDIR=/usr/share/examples/postfix
PFIX_SBINDIR=	/usr/sbin

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
