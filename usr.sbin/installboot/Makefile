#	$NetBSD: Makefile,v 1.10 2002/04/11 07:56:13 lukem Exp $
#

PROG=	installboot
SRCS=	installboot.c sum.c machines.c \
	alpha.c pmax.c sparc64.c vax.c
MAN=	installboot.8

WARNS?=	3

CPPFLAGS+=	-I${.CURDIR}
.PATH:		${.CURDIR}/arch

.ifndef HOSTPROG
# list of MACHINEs to enable the compat symlink /usr/mdec/installboot
# XXX: eventually this should be done for all machines.
# XXX: or /usr/mdec/installboot is replaced with a shell script that
# XXX: tells the user to use /usr/sbin/installboot instead
#
COMPATLINKS?=	pmax
afterinstall:
.for l in ${COMPATLINKS}
.if (${MACHINE} == ${l})
	${INSTALL_SYMLINK} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${BINDIR}/${PROG} ${DESTDIR}/usr/mdec/${PROG}
.endif
.endfor

.include <bsd.prog.mk>
.endif
