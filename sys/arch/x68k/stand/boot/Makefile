#	$NetBSD: Makefile,v 1.34 2020/08/10 07:00:49 rin Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=		Multi-boot
VERSIONFILE=	${.CURDIR}/../boot/version
VERSION!=	${TOOL_AWK} -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } \
			END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text address
TEXT=		0x6000

# RTC offset for netboot (XXX hardcoded for JST-9)
RTC_OFFSET=	-540

PROG?=		boot
BINDIR=		/usr/mdec
BINMODE=	444
STRIPFLAG=

SRCS=		srt0.S boot.c conf.c switch.c exec_image.S
SRCS+=		if_ne.c ne.c dp8390.c
S=		${.CURDIR}/../../../..
M=		${.CURDIR}/../..
COMMONDIR=	$M/stand/common
.PATH:		${COMMONDIR}
.PATH:		${.CURDIR}/../boot

.include "${S}/conf/newvers_stand.mk"

CPPFLAGS+=	-nostdinc -I$S -I${.OBJDIR} -I$M/stand/libsa
CPPFLAGS+=	-I$M/stand/libiocs -I${COMMONDIR}
CPPFLAGS+=	-D_STANDALONE -DHEAP_VARIABLE
CPPFLAGS+=	-DTEXTADDR="${TEXT}"
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CPPFLAGS+=	-DLIBSA_ENABLE_LS_OP
CPPFLAGS+=	-DRTC_OFFSET=${RTC_OFFSET}
CPPFLAGS+=	-DSUPPORT_BOOTP -DSUPPORT_DHCP
#CPPFLAGS+=	-DDEBUG
CFLAGS=		-Wno-main -Os -m68020-60 -Wno-attributes
LINKFLAGS=	-N -static -T ${.CURDIR}/../boot/boot.ldscript
LINKFLAGS+=	--defsym=TEXTADDR=$(TEXT)
ELF2AOUT_OPTS=	-O
LIBIOCS!=	cd $M/stand/libiocs && ${PRINTOBJDIR}
LIBSA!=		cd $M/stand/libsa && ${PRINTOBJDIR}
L=		${LIBSA}/lib
LDLIBS= 	${L}/sa/libsa.a ${L}/kern/libkern.a ${L}/z/libz.a
LDLIBS+=	${LIBIOCS}/libiocs.a

.include "../Makefile.booters"

realall: ${PROG}
${PROG}:	${OBJS} ${LDLIBS}
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${PROG}.sym ${OBJS} ${LDLIBS}
	${STRIP} ${PROG}.sym -o ${PROG}.strip
	${M68K_ELF2AOUT} ${ELF2AOUT_OPTS} ${PROG}.strip ${PROG}

CLEANFILES+=	${PROG}.sym ${PROG}.strip

.include <bsd.prog.mk>
