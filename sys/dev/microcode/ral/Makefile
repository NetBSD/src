#	$NetBSD: Makefile,v 1.2 2006/06/07 19:14:37 rpaulo Exp $

# Based on:
# $OpenBSD: Makefile,v 1.1 2006/01/09 20:03:40 damien Exp $

NOMAN=	# defined

FIRM=	ral-rt2561 ral-rt2561s ral-rt2661

PROG=	build

all: ${FIRM}

${FIRM}: build
	${.OBJDIR}/build

proginstall:

afterinstall:
	${_MKMSG_INSTALL} ${FIRMPATH}
	${INSTALL_DIR} ${FIRMPATH}
.for i in ${FIRM}
	${_MKMSG_INSTALL} ${i}
	${INSTALL_FILE} ${i} ${FIRMPATH}
.endfor

cleanall:
	rm -f ${FIRM}

.include <bsd.prog.mk>
