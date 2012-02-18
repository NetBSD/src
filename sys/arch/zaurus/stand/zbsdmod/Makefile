#	$NetBSD: Makefile,v 1.5.16.1 2012/02/18 07:33:52 mrg Exp $

WARNS?=	4

S=	${.CURDIR}/../../../..

OBJS=	zbsdmod.o
SRCS=	zbsdmod.c
NOMAN=	# defined

realall: ${OBJS}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

afterinstall:
	${INSTALL_FILE} ${INSTALL_COPY} -o root -g wheel -m 644 \
		${OBJS} ${DESTDIR}/${BINDIR}

CPUFLAGS=
COPTS=		-Os
CFLAGS+=	-fno-strict-aliasing
CFLAGS+=	-DMACHINE=\"${MACHINE}\" -DUTS_RELEASE=\"2.4.20\"
CPPFLAGS+=      -nostdinc -D_STANDALONE
CPPFLAGS+=	-I${.OBJDIR} -I${S}

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${OBJS} \
	    ${RELEASEDIR}/${MACHINE}/installation
