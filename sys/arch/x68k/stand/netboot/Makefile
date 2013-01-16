#	$NetBSD: Makefile,v 1.1.2.3 2013/01/16 05:33:08 yamt Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=		Netboot
VERSIONFILE=	${.CURDIR}/version
VERSION!=	${TOOL_AWK} -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } \
			END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text address
TEXT=		006000

# RTC offset
RTC_OFFSET=	-540

PROG=		netboot
BINDIR=		/usr/mdec
BINMODE=	444
STRIPFLAG=

BFDNAME=	a.out-m68k-netbsd
STRIP?=		/usr/bin/strip
OBJCOPY?=	/usr/bin/objcopy

# XXX shared sources should be in COMMONDIR
BOOTDIR=	${.CURDIR}/../boot
.PATH:		${BOOTDIR}

SRCS=		srt0.S boot.c conf.c exec_image.S if_ne.c ne.c dp8390.c
S=		${.CURDIR}/../../../..
M=		${.CURDIR}/../..
COMMONDIR=	$M/stand/common
.PATH:		${COMMONDIR}

SRCS+=		vers.c
CLEANFILES+=	vers.c
vers.c:	${VERSIONFILE}
	${_MKTARGET_CREATE}
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${.ALLSRC} ${MACHINE} ${NEWVERSWHAT}

CPPFLAGS+=	-nostdinc -I$S -I${.OBJDIR} -I$M/stand/libsa
CPPFLAGS+=	-I$M/stand/libiocs -I${COMMONDIR}
CPPFLAGS+=	-D_STANDALONE -DHEAP_VARIABLE
CPPFLAGS+=	-DTEXTADDR="0x${TEXT}" 
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CPPFLAGS+=	-DLIBSA_ENABLE_LS_OP
CPPFLAGS+=	-DRTC_OFFSET=${RTC_OFFSET}
CPPFLAGS+=	-DSUPPORT_BOOTP -DSUPPORT_DHCP
CFLAGS=		-Wno-main -Os -m68020-60
LINKFLAGS=	-N -static -T ${BOOTDIR}/boot.ldscript
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
