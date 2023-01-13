#	$NetBSD: Makefile,v 1.4 2023/01/13 18:43:43 martin Exp $

NODEBUG=yes
PROG=	xxboot
NOMAN=	# defined
SRCS=	start.S main.c readufs.c readufs_ffs.c readufs_lfs.c milli_tiny.S

.PATH:	${.CURDIR}/../common

CPPFLAGS+=	-I${.CURDIR}/../../../.. -I. -D_STANDALONE
# configuration for readufs module
CPPFLAGS+=	-DUSE_LFS -DUSE_FFS -DUSE_UFS1 -DUSE_UFS2
# IODC can handle only 2GB, so this is enough
CPPFLAGS+=	-D__daddr_t=int32_t -D__ino_t=uint32_t
# ANSI C feature prevents from being relocatable
#CPPFLAGS+=	-traditional	# would be best
CPPFLAGS+=	-Dconst=
COPTS+=		-Os -fno-delete-null-pointer-checks -funsigned-char -mdisable-fpregs -mpa-risc-1-0

.include <bsd.own.mk>

LINKS=	${BINDIR}/${PROG} ${BINDIR}/sdboot

BINDIR=		/usr/mdec
STRIPFLAG=
BINMODE=	444

# standalone program
LIBCRTBEGIN=
LIBCRT0=
LIBCRTI=
LIBCRTEND=
LIBC=

S=		${.CURDIR}/../../../..

${PROG}: iplsum ${OBJS}
	${_MKTARGET_LINK}
	${LD} -Ttext 0 -Tdata 0 -e '$$START$$' -N -o $@1 $(OBJS)
	${LD} -Ttext 0x100 -Tdata 0x23456780 -e '$$START$$' -N -o $@2 $(OBJS)
	${SIZE} $@1
	${OBJCOPY} -O binary -j .data $@1 $@1.bin
	${OBJCOPY} -O binary -j .data $@2 $@2.bin
	cmp $@1.bin $@2.bin	# should be same
	${OBJCOPY} -O binary -j .text $@1 $@2.bin
	test ! -s $@2.bin	# text section must be empty
	${_MKMSG} " iplsum " ${.TARGET}
	./iplsum $@1.bin $@

iplsum: iplsum.c
	${_MKTARGET_LINK}
	${HOST_CC} -o $@ ${.CURDIR}/iplsum.c

CLEANFILES+=	${PROG}1 ${PROG}2 ${PROG}1.bin ${PROG}2.bin ${PROG}.bin iplsum
CLEANFILES+=	${SRCS:M*.c:S/.c$/.o.S/}

.include <bsd.prog.mk>
.include <bsd.klinks.mk>

# override default rules

# Place code to data section.
.S.o:
	${_MKTARGET_COMPILE}
	${TOOL_SED} -e 's/\.code/.data/' \
	-e 's/\.bss/.section .bss,"aw",@nobits/' \
	-e 's/\.allow$$/.level	1.0/' -e 's/\.allow/.level/' \
	 ${.IMPSRC} | ${AS} -o ${.TARGET}

# Place code to data section, and make sure all address calculations
# are relative to $global$.
.c.o:
	${_MKTARGET_COMPILE}
	${CC} ${CFLAGS} ${CPPFLAGS} -o $@.S -S ${.IMPSRC}
	grep -i 'ldil' $@.S | egrep -v "ldil L'-?[0-9]*," > /dev/null 2>&1; \
		if [ $$? = 0 ]; then \
			echo 'found non-relocatable code' >&2 && exit 1; \
		fi
	${TOOL_SED} -e 's/\.text/.data/' $@.S | ${AS} -o ${.TARGET}
