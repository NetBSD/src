#	$NetBSD: Makefile,v 1.8 2000/07/21 22:26:19 jdolecek Exp $

S=	${.CURDIR}/../../../..

### find out what to use for libkern
KERN_AS=        library
.include "${S}/lib/libkern/Makefile.inc"
#LIBKERN=        ${KERNLIB}

### find out what to use for libz
Z_AS=           library
.include "${S}/lib/libz/Makefile.inc"
#LIBZ=           ${ZLIB}

### find out what to use for libsa
SA_AS=          library
#SAMISCMAKEFLAGS= SA_USE_CREAD=yes
.include "${S}/lib/libsa/Makefile.inc"
#LIBSA=          ${SALIB}

RELOC=	4380000

# XXX make defs arch-indep.
INCLUDES+=	-I${.CURDIR} -I${.CURDIR}/obj.${MACHINE} -I${S}/arch -I${S} -I${S}/lib/libsa -I${S}/lib/libkern
DEFS+=		-D_STANDALONE -DMC68040 -DSUPPORT_BOOTP -DSUPPORT_DHCP  -DDEBUG -DSD_DEBUG -DSCSI_DEBUG # -DEN_DEBUG   -DNETIF_DEBUG  
SAMISCCPPFLAGS=	-DSUPPORT_DHCP -DSUPPORT_BOOTP -DINSECURE # -DBOOTP_DEBUG -DETHER_DEBUG -DNET_DEBUG # -DNETIF_DEBUG -DNFS_DEBUG  -DARP_DEBUG 
WARNS=1
HAVE_GCC28!=	${CC} --version | egrep "^(2\.8|egcs)" ; echo 
.if (${HAVE_GCC28} != "")
CWARNFLAGS+=	-Wno-main
.endif
CFLAGS+=	 -nostdinc ${INCLUDES} ${DEFS}
LIBCRT0=

PROG=	boot
SRCS=   boot.c machdep.c conf.c devopen.c rtc.c sd.c scsi.c en.c vers.c build.c
# @@@ dev_net.c should really be in libsa, but it doesn't
# declare ip_convertaddr correctly, so I put it here _temporarily_.

CLEANFILES+= vers.c build.c build	# generated dynamically

MKMAN=	no
BINDIR=	/usr/mdec
#LIBS=	${SALIB} ${KERNLIB} ${ZLIB}
LIBS=	${SALIB} ${KERNLIB}

OBJS+=	dev_net.o

${PROG}: links srt0.o ${OBJS} ${LIBS}
	${LD} -N -Ttext ${RELOC} -e start srt0.o ${OBJS} ${LIBS} vers.o -o $@
	@${SIZE} $@

# this looks useful to me ... let's see when it will be an official part
# of libsa
dev_net.o : ${S}/lib/libsa/dev_net.c
	${COMPILE.c} ${.IMPSRC}

# startup

srt0.o: ${.CURDIR}/srt0.s
	${CC} -m68040 ${INCPATH} ${DEFS} -c ${.CURDIR}/srt0.s

.PHONY: build.c
build.c:
	sh ${.CURDIR}/newvers.sh

vers.c: version
	sh ${S}/conf/newvers_stand.sh -MD ${.CURDIR}/version "next68k"

#installboot: ${.CURDIR}/installboot.sh
#	@rm -f installboot
#	cp -p ${.CURDIR}/installboot.sh installboot

# utilities

links:
	rm -f machine
	ln -s ${S}/arch/${MACHINE}/include machine
	rm -f ${MACHINE_ARCH}
	ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
	rm -f limits.h
	ln -s machine/limits.h limits.h

clean:
	rm -f *.o errs core make.out
	rm -f a.out boot cat ls
	rm -f machine ${MACHINE_ARCH} limits.h

.include <bsd.prog.mk>
