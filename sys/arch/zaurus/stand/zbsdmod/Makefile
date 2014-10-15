#	$NetBSD: Makefile,v 1.8.4.1 2014/10/15 08:37:34 martin Exp $

WARNS?=	4

S=	${.CURDIR}/../../../..

OBJS=	zbsdmod.o
SRCS=	zbsdmod.c
NOMAN=	# defined

realall: ${OBJS}
	${OBJCOPY} -R .eh_frame ${.OBJDIR}/zbsdmod.o

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

afterinstall:
	${INSTALL_FILE} ${INSTALL_COPY} -o root -g wheel -m 644 \
		${OBJS} ${DESTDIR}/${BINDIR}

CPUFLAGS=
COPTS=		-Os
CFLAGS+=	-fno-strict-aliasing
CFLAGS+=	-DMACHINE=\"${MACHINE}\" -DUTS_RELEASE=\"2.4.20\"
CPPFLAGS+=	${ARM_APCS_FLAGS} -mcpu=xscale
CPPFLAGS+=      -nostdinc -D_STANDALONE
CPPFLAGS+=	-I${.OBJDIR} -I${S}

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${OBJS} \
	    ${RELEASEDIR}/${MACHINE}/installation
