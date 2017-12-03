#	$NetBSD: Makefile,v 1.22.2.2 2017/12/03 11:36:49 jdolecek Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=		Multi-boot
VERSIONFILE=	${.CURDIR}/../boot/version
VERSION!=	${TOOL_AWK} -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } \
			END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text address
TEXT=		006000

# RTC offset for netboot (XXX hardcoded for JST-9)
RTC_OFFSET=	-540

PROG?=		boot
BINDIR=		/usr/mdec
BINMODE=	444
STRIPFLAG=

BFDNAME=	a.out-m68k-netbsd

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
CPPFLAGS+=	-DTEXTADDR="0x${TEXT}" 
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CPPFLAGS+=	-DLIBSA_ENABLE_LS_OP
CPPFLAGS+=	-DRTC_OFFSET=${RTC_OFFSET}
CPPFLAGS+=	-DSUPPORT_BOOTP -DSUPPORT_DHCP
#CPPFLAGS+=	-DDEBUG
CFLAGS=		-Wno-main -Os -m68020-60
LINKFLAGS=	-N -static -T ${.CURDIR}/../boot/boot.ldscript
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
	${STRIP} -F ${BFDNAME} -o ${PROG} ${PROG}.sym

CLEANFILES+=	${PROG}.sym

.include <bsd.prog.mk>
