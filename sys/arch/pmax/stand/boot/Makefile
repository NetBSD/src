#	$NetBSD: Makefile,v 1.4 1999/03/31 02:35:14 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

PROG=		boot
RELOC=		80710000
BINMODE=	555
MKMAN=		no
SRCS=		start.S boot.c
NEWVERSWHAT=	"Secondary Boot"
VERSIONFILE=	${.CURDIR}/version
.PATH:  	${.CURDIR}/../lib

# Copy boot to / as well.
afterinstall:
	cp -p ${DESTDIR}${BINDIR}/${PROG} ${DESTDIR}/

.include "../Makefile.booters"
