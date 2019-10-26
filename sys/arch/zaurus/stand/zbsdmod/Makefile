#	$NetBSD: Makefile,v 1.12 2019/10/26 10:10:14 tsutsui Exp $

WARNS?=	4

.include <bsd.init.mk>

S=	${.CURDIR}/../../../..

OBJS=	zbsdmod.o
SRCS=	zbsdmod.c
NOMAN=	# defined

OBJCOPY_FLAGS=			\
	-R .debug_abbrev	\
	-R .debug_aranges	\
	-R .debug_info		\
	-R .debug_line		\
	-R .debug_loc		\
	-R .debug_ranges	\
	-R .debug_str		\
	-R .eh_frame

realall: ${OBJS}
	${OBJCOPY} ${OBJCOPY_FLAGS} ${.OBJDIR}/zbsdmod.o

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

afterinstall:
	${INSTALL_FILE} ${INSTALL_COPY} -o root -g wheel -m 644 \
		${OBJS} ${DESTDIR}/${BINDIR}

CPUFLAGS=
DBG=		-Os
CFLAGS+=	-fno-strict-aliasing
CFLAGS+=	-ffreestanding -nostdinc
CFLAGS+=	${ARM_APCS_FLAGS} -mcpu=xscale
CPPFLAGS+=	-DMACHINE=\"${MACHINE}\" -DUTS_RELEASE=\"2.4.20\"
CPPFLAGS+=	-D_STANDALONE
CPPFLAGS+=	-I${.OBJDIR} -I${S}

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${OBJS} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
