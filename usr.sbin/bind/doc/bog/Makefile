#	$NetBSD: Makefile,v 1.1.4.1 2002/07/01 17:17:08 he Exp $

DIST=	${.CURDIR}/../../../../dist
.PATH: ${DIST}/bind/doc/bog

DIR=	smm/10.named
SRCS=	00macs.me 00title.me intro.me ns.me types.me\
	files.me named.boot.primary\
	named.boot.secondary named.boot.cache resolv.conf\
	root.cache named.local ucbhosts.rev ucbhosts \
	setup.me manage.me build.me ack.me
MACROS=	-me
RM=	rm -f
CLEANFILES+=	bog.ps

all: bog.ps

bog.ps: ${SRCS}
	${RM} ${.TARGET}
	${TBL} ${.ALLSRC} | ${ROFF} ${MACROS} >${.TARGET}

.include <bsd.doc.mk>
