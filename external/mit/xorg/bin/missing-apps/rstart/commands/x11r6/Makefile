#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:27 wrstuden Exp $

.include <bsd.own.mk>

CFDIR=		commands/x11r6
CFFILES=	@List LoadMonitor Terminal
CFSED=		-e 's,ENVPREFIX,RSTART,g'

.include "../../Makefile.rstart"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
