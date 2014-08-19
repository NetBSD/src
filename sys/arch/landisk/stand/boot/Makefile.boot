# $NetBSD: Makefile.boot,v 1.5.6.1 2014/08/20 00:03:09 tls Exp $

PROG?=		boot

NEWVERSWHAT?=	"Boot"
VERSIONFILE?=	${.CURDIR}/../version

SRCS=	boot.S boot2.c bootinfo.c conf.c devopen.c monitor.c
SRCS+=	delay.c getsecs.c
SRCS+=	bios.S
SRCS+=	biosdisk.c biosdisk_ll.c
SRCS+=	scifcons.c cons.c prf.c
.if !make(depend)
SRCS+=	vers.c
.endif

LDFLAGS+=	-e boot_start

CFLAGS=
CPPFLAGS=	-DSUPPORT_FFSv1
CPPFLAGS+=	-DSUPPORT_FFSv2
CPPFLAGS+=	-DSUPPORT_DOSFS
CPPFLAGS+=	-DSUPPORT_USTARFS
CPPFLAGS+=	-DDBMONITOR
CPPFLAGS+=	-DLIBSA_ENABLE_LS_OP
#CPPFLAGS+=	-DDEBUG

SAMISCMAKEFLAGS+="SA_USE_CREAD=yes"
SAMISCMAKEFLAGS+="SA_USE_LOADFILE=yes"
SAMISCMAKEFLAGS+="SA_ENABLE_LS_OP=yes"

.include "../Makefile.bootprogs"

LIBLIST=	${LIBSA} ${LIBZ} ${LIBKERN}

CLEANFILES+=	${PROG}.sym ${PROG}.map vers.c

vers.c: ${VERSIONFILE} ${SOURCES} ${.CURDIR}/../Makefile.boot
	${HOST_SH} ${S}/conf/newvers_stand.sh ${${MKREPRO} == "yes" :?:-D} \
	    ${VERSIONFILE} ${MACHINE} ${NEWVERSWHAT}

${PROG}: ${OBJS} ${LIBLIST}
	${_MKTARGET_LINK}
	${LD} -o ${PROG}.sym ${LDFLAGS} -Ttext ${SECONDARY_LOAD_ADDRESS} \
		-Map ${PROG}.map -cref ${OBJS} ${LIBLIST}
	${OBJCOPY} -O binary ${PROG}.sym ${PROG}

.include <bsd.prog.mk>
