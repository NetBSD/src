#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

CFDIR=		contexts
CFFILES=	@List default x11r6
CFSED=		-e 's,_PATH,/bin:/usr/bin:/usr/pkg/bin:/usr/local/bin:${X11BINDIR},g' \
    		-e 's,_MANPATH,${X11MANDIR}:/usr/man,g'

SYMLINKS=	x11r6 ${FILESDIR}/x \
		x11r6 ${FILESDIR}/x11 \

.include "../Makefile.rstart"

.include <bsd.prog.mk>
.include <bsd.x11.mk>
