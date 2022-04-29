#	$NetBSD: Makefile,v 1.27 2022/04/29 07:18:17 rin Exp $

.include <bsd.init.mk>
.include <bsd.sys.mk>

### what we need:

DIR_TOP=	${.CURDIR}/../../../../..
DIR_SA = 	${DIR_TOP}/lib/libsa
DIR_KERN=	${DIR_TOP}/lib/libkern
DIR_KERN_MD=	${DIR_TOP}/lib/libkern/arch/$(MACHINE_ARCH)
DIR_LIBC=	${DIR_TOP}/../common/lib/libc

.PATH:  ${.CURDIR}/../boot $(DIR_SA) $(DIR_KERN) $(DIR_KERN_MD) \
	${DIR_LIBC}/gen ${DIR_LIBC}/arch/m68k/gen \
	${DIR_LIBC}/inet ${DIR_LIBC}/arch/m68k/inet \
	${DIR_LIBC}/quad ${DIR_LIBC}/arch/m68k/quad \
	${DIR_LIBC}/string ${DIR_LIBC}/arch/m68k/string

S=		${DIR_TOP}

# prefer our assembler versions over assembler, and assembler over C:

.SUFFIXES:
.SUFFIXES: .out .o .po .pico .so .s .S .c .cc .C .f .y .l .ln .m4 .sh

.ifdef BOOTXX_FFSV2
FILES=		bootxx_ffsv2
FSOBJS=		ffsv2.o
DEFS+=		-DBOOTXX_FFS_VERSION=2
.else
FILES=		bootxx_ffsv1 bootxx_fd
FSOBJS=		ufs.o ustarfs.o
DEFS+=		-DBOOTXX_FFS_VERSION=1
.endif

BINDIR=/usr/mdec

COBJS = main.o console.o xd.o bzero.o gets.o
COBJS+=  lseek.o open.o vers.o read.o close.o dev.o errno.o
COBJS+=  ${FSOBJS} panic.o files.o

SOBJS = alloc.o ashrdi3.o ashldi3.o bcopy.o muldi3.o printf.o startit.o
SOBJS += strlen.o strcmp.o fstat.o
SOBJS += libstubs.o memcmp.o memmove.o memset.o strncmp.o

OBJS+=	$(SOBJS) $(COBJS)

DEFS+= -D_STANDALONE -DSA_EXEC_ANYOWNER -D_PRIMARY_BOOT -DSERCONSOLE

.NOPATH: ${OBJS} x.out f.out libboot.a xxstart.o

### main target: ###

realall: ${FILES}

CLEANFILES += ${FILES} x.out f.out xxstart.o fdstart.o libboot.a

VERSIONFILE=${.CURDIR}/../boot/version
VERSIONFLAGS+=-n
.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>
.include <bsd.klinks.mk>

### special  rules for bootblocks ###

INCPATH = -nostdinc -I${S} -I${S}/lib/libsa -I${.CURDIR}
INCPATH += -I${.CURDIR}/../boot -I${.CURDIR}/../../..
INCPATH += -I${.CURDIR}/../elf2bb -I${.OBJDIR}

AFLAGS += -m68030 -l
CAFLAGS += -Wa,-l -Wa,-march=68030 -Wa,-mcpu=68030 ${INCPATH} -D_PRIMARY_BOOT

COPTIM= -Os -fomit-frame-pointer -fcse-follow-jumps -fcse-skip-blocks
COPTIM+= -Wa,-l -m68060 -Wa,-mcpu=68030 -fno-unwind-tables
CFLAGS= -ffreestanding ${COPTIM} ${INCPATH} ${DEFS} -Wall #-Wstrict-prototypes

NETBSD_VERS!=	${HOST_SH} ${.CURDIR}/../../../../../conf/osrelease.sh
DEFS+=	-DNETBSD_VERS='"${NETBSD_VERS}"'

# Use small daddr_t to avoid code bloat
DEFS+=	-D__daddr_t=int32_t

.c.o:
	${_MKTARGET_COMPILE}
	${CC} ${CFLAGS} -S $< -o $*.s
	${TXLT} < $*.s | ${AS} ${AFLAGS} -o $*.o
	rm $*.s

.s.o:
	${_MKTARGET_COMPILE}
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

.S.o:
	${_MKTARGET_COMPILE}
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp -o $@ -c $<

bootxx_ffsv1: x.out
	${_MKTARGET_CREATE}
	${RELOC2BB} x.out $@ || (${NM} -u x.out && false)

bootxx_ffsv2: x.out
	${_MKTARGET_CREATE}
	${RELOC2BB} x.out $@ || (${NM} -u x.out && false)

bootxx_fd: f.out
	${_MKTARGET_CREATE}
	${RELOC2BB} -F f.out $@ || (${NM} -u f.out && false)

x.out: xxstart.o libboot.a
	${_MKTARGET_LINK}
	${LD} ${LDFLAGS} -r -dc -e start -o $@ $>
	${SIZE} $@
	${NM} -u $@

f.out: fdstart.o libboot.a
	${_MKTARGET_LINK}
	${LD} ${LDFLAGS} -r -dc -e start -o $@ $>
	${SIZE} $@
	${NM} -u $@

xxstart.o: ${.CURDIR}/../boot/bbstart.s
	${_MKTARGET_COMPILE}
	${CC} ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

fdstart.o: ${.CURDIR}/../boot//bbstart.s
	${_MKTARGET_COMPILE}
	${CC} -DAUTOLOAD=8192 ${CAFLAGS} ${COPTS} -x assembler-with-cpp \
		-o $@ -c $>

libboot.a: ${OBJS}
	${_MKTARGET_BUILD}
	${AR} crs $@ $> && ${RANLIB} $@

# make sure these are built:

${COBJS}: ${TXLT}
${FILES}: ${RELOC2BB}

.include "${.CURDIR}/../Makefile.booters"
