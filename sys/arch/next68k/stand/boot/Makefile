#	$NetBSD: Makefile,v 1.24.14.2 2014/08/20 00:03:17 tls Exp $

NOMAN=	# defined

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
SAMISCMAKEFLAGS= SA_USE_LOADFILE=yes # SA_USE_CREAD=yes
.include "${S}/lib/libsa/Makefile.inc"
#LIBSA=          ${SALIB}

RELOC=	4380000

# XXX make defs arch-indep.
INCLUDES+=	-I${.OBJDIR} -I${S}/arch -I${S} -I${S}/lib/libsa 
DEFS+=		-D_STANDALONE -DMC68040 -DSUPPORT_BOOTP -DSUPPORT_DHCP  -DDEBUG -DSD_DEBUG -DSCSI_DEBUG # -DEN_DEBUG   -DNETIF_DEBUG  
SAMISCCPPFLAGS=	-DSUPPORT_DHCP -DSUPPORT_BOOTP -DSA_EXEC_ANYOWNER # -DBOOTP_DEBUG -DETHER_DEBUG -DNET_DEBUG # -DNETIF_DEBUG -DNFS_DEBUG  -DARP_DEBUG 
WARNS=1
CFLAGS+=	 -ffreestanding -nostdinc ${INCLUDES} ${DEFS}
CFLAGS+=	-Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
AFLAGS+=	${INCLUDES}
# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

PROG=	boot
SRCS=   boot.c machdep.c conf.c devopen.c rtc.c sd.c scsi.c en.c vers.c build.c

# @@@ dev_net.c should really be in libsa, but it doesn't
# declare ip_convertaddr correctly, so I put it here _temporarily_.
.PATH: ${S}/lib/libsa
SRCS+=	dev_net.c

CLEANFILES+= srt0.o boot.elf boot.raw limits.h
CLEANFILES+= vers.c build.c build	# generated dynamically

BINDIR=	/usr/mdec
#LIBS=	${SALIB} ${KERNLIB} ${ZLIB}
LIBS=	${SALIB} ${KERNLIB}

# do not strip ${PROG} on install
STRIPFLAG=	# defined

AWKPROG='\
function x(v) { printf "\\0\\%o\\%o\\%o", (v / 65536) % 256, (v / 256) % 256, v % 256 } \
{ \
	printf "\047\\0\\207\\01\\07"; \
	x($$1); \
	x($$2); \
	x($$3); \
	printf "\\0\\0\\0\\0\\04\\070\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\047" \
}'

${PROG}: srt0.o ${OBJS} ${LIBS}
	${_MKTARGET_LINK}
	${LD} -N -Ttext ${RELOC} -e start srt0.o ${OBJS} ${LIBS} -o $@.elf
	${SIZE} $@.elf
	${OBJCOPY} -O binary $@.elf $@.raw
	(${SIZE} $@.elf | tail +2 | ${TOOL_AWK} ${AWKPROG} | \
		xargs printf ; cat $@.raw) > $@

# startup

srt0.o: ${.CURDIR}/srt0.s
	${CC} -m68040 -x assembler-with-cpp ${INCLUDES} ${DEFS} -c ${.CURDIR}/srt0.s

.PHONY: build.c
build.c:
	${HOST_SH} ${.CURDIR}/newvers.sh

vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "next68k"

#installboot: ${.CURDIR}/installboot.sh
#	@rm -f installboot
#	cp -p ${.CURDIR}/installboot.sh installboot

# utilities

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
