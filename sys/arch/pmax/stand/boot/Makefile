#	$NetBSD: Makefile,v 1.3 1999/03/27 07:17:51 simonb Exp $
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

# XXX These aren't build by libkern at the moment - fix libkern?
SRCS+=		bcmp.c bzero.c
.PATH:		$S/lib/libkern

# Copy boot to / as well.
afterinstall:
	cp -p ${DESTDIR}${BINDIR}/${PROG} ${DESTDIR}/

.include "../Makefile.booters"
