#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:09 lukem Exp $

.include <bsd.own.mk>

CFDIR=		commands
CFFILES=	@List ListContexts ListGenericCommands
CFSED=		-e 's,ENVPREFIX,RSTART,g'

SYMLINKS=	x11r6 ${FILESDIR}/x \
		x11r6 ${FILESDIR}/x11 \

SUBDIR=		x11r6

.include "../Makefile.rstart"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
