#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:47 lukem Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist
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
	${TOOL_TBL} ${.ALLSRC} | ${TOOL_ROFF_PS} ${MACROS} >${.TARGET}

.include <bsd.doc.mk>
