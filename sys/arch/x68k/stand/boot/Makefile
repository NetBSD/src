#	$NetBSD: Makefile,v 1.5 2001/10/15 16:23:00 minoura Exp $

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
MKMAN=		no
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
	sh ${S}/conf/newvers_stand.sh ${.ALLSRC} ${MACHINE} ${NEWVERSWHAT}

CPPFLAGS+=	-nostdinc -I$S -I${.OBJDIR} -I$M/stand/libsa
CPPFLAGS+=	-I$M/stand/libiocs -I${COMMONDIR}
CPPFLAGS+=	-D_STANDALONE -DHEAP_VARIABLE
CPPFLAGS+=	-DTEXTADDR="0x${TEXT}" 
CPPFLAGS+=	-DBOOT=\"${BOOT}\" -DBOOT_VERS=\"${VERSION}\"
CFLAGS=		-Wno-main -Os -m68020-60
.if ${OBJECT_FMT} == "ELF"
LDFLAGS=	-N -static -T ${.CURDIR}/boot.ldscript
.else
LDFLAGS=	-N -static -Ttext ${TEXT}
.endif
LIBIOCS!=	cd $M/stand/libiocs && ${MAKE} print-objdir
LIBSA!=		cd $M/stand/libsa && ${MAKE} print-objdir
LDLIBS=		-L${LIBSA} -lsa -L${LIBIOCS} -liocs

.PHONY:	machine-links
beforedepend: machine-links
machine-links:
	-rm -f machine && \
	    ln -s $M/include machine
	-rm -f ${MACHINE_ARCH} && \
	    ln -s $S/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
CLEANFILES+=	machine ${MACHINE_ARCH}

realall: machine-links ${PROG}
${PROG}:	${OBJS}
	${LD} ${LDFLAGS} -o ${PROG}.sym ${OBJS} ${LDLIBS}
	${STRIP} -F ${BFDNAME} -o ${PROG} ${PROG}.sym

CLEANFILES+=	${PROG}.sym

.include <bsd.prog.mk>
