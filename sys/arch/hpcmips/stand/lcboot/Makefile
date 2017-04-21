#	$NetBSD: Makefile,v 1.12.14.1 2017/04/21 16:53:27 bouyer Exp $
.include <bsd.own.mk>

ROMICE?=	no

S=	${.CURDIR}/../../../../

PROG=	lcboot.out
SREC=	lcboot.srec
MKMAN=	no
WARNS=	1
SRCS=	start.S main.c com.c conf.c dev_flash.c dev_net.c \
	devopen.c i28f128.c if_cs.c

#CPPFLAGS+=	-DDEBUG -DNET_DEBUG -DNETIF_DEBUG -DARP_DEBUG \
#		-DETHER_DEBUG -DBOOTP_DEBUG

STARTOBJS=

NEWVERSWHAT=    "L-Card+ Boot"

# Make sure we override any optimization options specified by the user.
#COPTS=  -Os
COPTS=
DBG=

AFLAGS+=	-D_LOCORE -D_KERNEL
AFLAGS+=	-x assembler-with-cpp -mips2 -mno-abicalls
INCLUDES=	-I${.OBJDIR} -I${S} -I${S}/arch
CPPFLAGS+=	${INCLUDES} -nostdinc -D_STANDALONE -DHEAP_LIMIT=0x8002ffff
CFLAGS+=	-mips2 -EL -mno-abicalls -ffreestanding -mmemcpy
LD_SCRIPT=	lcboot.ldscript

.if defined(ROMICE) && (${ROMICE} == "yes")
CPPFLAGS+=	-DROMICE
.endif

### find out what to use for libkern
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libz
.include "${S}/lib/libz/Makefile.inc"
LIBZ=	${ZLIB}

### find out what to use for libsa
SAMISCMAKEFLAGS+="SA_USE_LOADFILE=yes"
SAMISCMAKEFLAGS+="SA_USE_CREAD=yes"
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

LIBLIST=	${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN} ${LIBSA}

CLEANFILES+=	${PROG}
${PROG}: ${OBJS} ${LIBKERN} ${LIBSA} ${LIBZ}
	${_MKTARGET_LINK}
	${LD} -o ${PROG} -T ${LD_SCRIPT} ${OBJS} ${LIBLIST}

CLEANFILES+=	${SREC}
all: ${SREC}
${SREC}: ${PROG}
	${OBJCOPY} -S -O srec ${PROG} ${SREC}

.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
