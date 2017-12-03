# $NetBSD: Makefile.boot,v 1.5.6.2 2017/12/03 11:36:22 jdolecek Exp $

PROG?=		boot

NEWVERSWHAT?=	"Boot"

SRCS=	boot.S boot2.c bootinfo.c conf.c devopen.c monitor.c
SRCS+=	delay.c getsecs.c
SRCS+=	bios.S
SRCS+=	biosdisk.c biosdisk_ll.c
SRCS+=	scifcons.c cons.c prf.c

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

CLEANFILES+=	${PROG}.sym ${PROG}.map


${PROG}: ${OBJS} ${LIBLIST}
	${_MKTARGET_LINK}
	${LD} -o ${PROG}.sym ${LDFLAGS} -Ttext ${SECONDARY_LOAD_ADDRESS} \
		-Map ${PROG}.map -cref ${OBJS} ${LIBLIST}
	${OBJCOPY} -O binary ${PROG}.sym ${PROG}

.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>
