#	from: @(#)Makefile	5.2 (Berkeley) 3/5/91
#	$Id: Makefile,v 1.14 1993/10/15 01:06:47 jtc Exp $
#
# All library objects contain sccsid strings by default; they may be
# excluded as a space-saving measure.  To produce a library that does
# not contain these strings, delete -DLIBC_SCCS and -DSYSLIBC_SCCS
# from CFLAGS below.  To remove these strings from just the system call
# stubs, remove just -DSYSLIBC_SCCS from CFLAGS.
#
# The YP functions are always in libc. To choose that getpwent() and friends
# actually call the YP functions, put -DYP on the CFLAGS line below.

LIB=c
CFLAGS+=-DYP -DLIBC_SCCS -DSYSLIBC_SCCS
AINC=	-I${.CURDIR}/arch/${MACHINE}

.if exists (${.CURDIR}/arch/${MACHINE}/Makefile.inc)
.PATH:	${.CURDIR}/arch/${MACHINE}
.include "${.CURDIR}/arch/${MACHINE}/Makefile.inc"
.endif

.include "${.CURDIR}/db/Makefile.inc"
.include "${.CURDIR}/compat-43/Makefile.inc"
.include "${.CURDIR}/gen/Makefile.inc"
.include "${.CURDIR}/locale/Makefile.inc"
.include "${.CURDIR}/net/Makefile.inc"
.include "${.CURDIR}/stdio/Makefile.inc"
.include "${.CURDIR}/stdlib/Makefile.inc"
.include "${.CURDIR}/string/Makefile.inc"
.include "${.CURDIR}/sys/Makefile.inc"
.include "${.CURDIR}/yp/Makefile.inc"
.include "${.CURDIR}/rpc/Makefile.inc"
.include "${.CURDIR}/quad/Makefile.inc"

LIBKERN=	/sys/lib/libkern

KSRCS=	bcmp.c bzero.c ffs.c strcat.c strcmp.c strcpy.c strlen.c strncpy.c \
	htonl.c htons.c ntohl.c ntohl.c

copy-to-libkern:	${KSRCS}
	cp -p ${.ALLSRC} ${LIBKERN}

rm-from-libkern:
	for i in ${KSRCS}; do rm -f ${LIBKERN}/$$i; done
	
.include <bsd.lib.mk>
