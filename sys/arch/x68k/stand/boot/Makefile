#	$NetBSD: Makefile,v 1.13.34.1 2008/01/09 01:49:41 matt Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=		Multi-boot
VERSIONFILE=	${.CURDIR}/version
VERSION!=	awk -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text address
TEXT=		006000

PROG=		boot
BINDIR=		/usr/mdec
BINMODE=	444
STRIPFLAG=

BFDNAME=	a.out-m68k-netbsd
STRIP?=		/usr/bin/strip
OBJCOPY?=	/usr/bin/objcopy

SRCS=		srt0.S boot.c conf.c exec_image.S
S=		${.CURDIR}/../../../..
M=		$S/arch/${MACHINE}
COMMONDIR=	$M/stand/common
.PATH:		${COMMONDIR}

SRCS+=		vers.c
CLEANFILES+=	vers.c
vers.c:	${VERSIONFILE}
	${_MKTARGET_CREATE}
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.ALLSRC} ${MACHINE} ${NEWVERSWHAT}

CPPFLAGS+=	-nostdinc -I$S -I${.OBJDIR} -I$M/stand/libsa
CPPFLAGS+=	-I$M/stand/libiocs -I${COMMONDIR}
CPPFLAGS+=	-D_STANDALONE -DHEAP_VARIABLE
CPPFLAGS+=	-DTEXTADDR="0x${TEXT}" 
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CFLAGS=		-Wno-main -Os -m68020-60
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -static -T ${.CURDIR}/boot.ldscript
.else
LINKFLAGS=	-N -static -Ttext ${TEXT}
.endif
LIBIOCS!=	cd $M/stand/libiocs && ${PRINTOBJDIR}
LIBSA!=		cd $M/stand/libsa && ${PRINTOBJDIR}
LDLIBS=		-L${LIBSA} -lsa -L${LIBIOCS} -liocs

.include "../Makefile.booters"

realall: machine-links ${PROG}
${PROG}:	${OBJS} ${LIBSA}/libsa.a ${LIBIOCS}/libiocs.a
	${_MKTARGET_LINK}
	${LD} ${LINKFLAGS} -o ${PROG}.sym ${OBJS} ${LDLIBS}
	${STRIP} -F ${BFDNAME} -o ${PROG} ${PROG}.sym

CLEANFILES+=	${PROG}.sym

.include <bsd.prog.mk>
