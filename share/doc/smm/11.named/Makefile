#	from: @(#)Makefile	6.4 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:17 cgd Exp $

DIR=	smm/11.named
DOC=	11.named

SRCS=	00macs.me 00title.me intro.me build.me types.me setup.me \
	files.me named.boot.primary named.boot.secondary named.boot.cache \
	resolv.conf root.cache named.local ucbhosts ucbhosts.rev manage.me
MACROS=	-me
NROFF=	nroff -rb3

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
