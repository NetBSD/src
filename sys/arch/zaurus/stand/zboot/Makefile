#	$NetBSD: Makefile,v 1.8.6.3 2017/12/03 11:36:52 jdolecek Exp $

PROG=		zboot

S=		${.CURDIR}/../../../..

SRCS=		crt0.c
SRCS+=		boot.c bootinfo.c bootmenu.c conf.c devopen.c diskprobe.c
SRCS+=		loadfile_zboot.c
SRCS+=		getsecs.c termios.c unixcons.c unixdev.c unixsys.S
SRCS+=		pathfs.c

NOMAN=		# defined

.include <bsd.init.mk>

CFLAGS+=	-Wall -Wno-main
CFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CFLAGS+=	-fno-stack-protector -fno-builtin -ffreestanding
CFLAGS+=	-fno-unwind-tables
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -I${.OBJDIR} -I${S}
CPPFLAGS+=	-D_STANDALONE -DHEAP_VARIABLE
CPPFLAGS+=	-DLIBSA_ENABLE_LS_OP
CPPFLAGS+=	${ARM_APCS_FLAGS} -mcpu=xscale
CPPFLAGS+=	-marm
CPUFLAGS=
AFLAGS+=	-D_LOCORE
LDFLAGS+=	-nostdlib -Bstatic

LIBCRT0=	crt0.o
LIBCRTI=	# nothing
LIBC=		# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing

NEWVERSWHAT?=   "Boot"

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes SA_ENABLE_LS_OP=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${_MKTARGET_LINK}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}

.include "${S}/conf/newvers_stand.mk"

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG} \
	    ${RELEASEDIR}/${MACHINE}/installation
