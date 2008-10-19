#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:58 haad Exp $

.include <bsd.own.mk>

CFDIR=		commands
CFFILES=	@List ListContexts ListGenericCommands
CFSED=		-e 's,ENVPREFIX,RSTART,g'

CONFIGSYMLINKS=	x11r6 ${FILESDIR}/x \
		x11r6 ${FILESDIR}/x11 \

SUBDIR=		x11r6

.include "../Makefile.rstart"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
