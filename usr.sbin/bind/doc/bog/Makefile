#	$NetBSD: Makefile,v 1.1 2000/04/28 06:21:21 itojun Exp $

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
ROFF=	nroff ${MACROS}

all: bog.ps

bog.ps: ${SRCS}
	${RM} ${.TARGET}
	${TBL} ${.ALLSRC} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
