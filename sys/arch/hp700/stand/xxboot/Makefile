#	$NetBSD: Makefile,v 1.2 2004/08/09 16:01:37 chs Exp $

PROG=	xxboot
SRCS=	start.s main.c readufs.c readufs_ffs.c readufs_lfs.c milli_tiny.s

CPPFLAGS+=	-mpa-risc-1-0 -I${.CURDIR}/../../../.. -I. -D_STANDALONE
# configuration for readufs module
CPPFLAGS+=	-DUSE_LFS -DUSE_FFS -DUSE_UFS1 -DUSE_UFS2
# IODC can handle only 2GB, so this is enough
CPPFLAGS+=	-D__daddr_t=int32_t
# ANSI C feature prevents from being relocatable
#CPPFLAGS+=	-traditional	# would be best
CPPFLAGS+=	-Dconst=
COPTS+=	-fwritable-strings -funsigned-char

LINKS=	${BINDIR}/${PROG} ${BINDIR}/sdboot

BINDIR=		/usr/mdec
STRIPFLAG=
BINMODE=	444
MKMAN=		no

# standalone program
LIBCRTBEGIN=
LIBCRT0=
LIBCRTEND=
LIBC=

${PROG}: iplsum
	${LD} -Ttext 0 -Tdata 0 -e '$$START$$' -N -o $@1 $(OBJS)
	${LD} -Ttext 0x100 -Tdata 0x23456780 -e '$$START$$' -N -o $@2 $(OBJS)
	${SIZE} $@1
	${OBJCOPY} -O binary -j .data $@1 $@1.bin
	${OBJCOPY} -O binary -j .data $@2 $@2.bin
	cmp $@1.bin $@2.bin	# should be same
	${OBJCOPY} -O binary -j .text $@1 $@2.bin
	test ! -s $@2.bin	# text section must be empty
	./iplsum $@1.bin $@

iplsum: iplsum.c
	${HOST_CC} -o $@ ${.CURDIR}/iplsum.c

CLEANFILES+=	${PROG}1 ${PROG}2 ${PROG}1.bin ${PROG}2.bin ${PROG}.bin iplsum
CLEANFILES+=	${SRCS:M*.c:S/.c$/.o.s/}

.include <bsd.prog.mk>

# override default rules

# Place code to data section.
.s.o:
	sed -e 's/\.code/.data/' \
	-e 's/\.bss/.section .bss,"aw",@nobits/' \
	-e 's/\.allow$$/.level	1.0/' -e 's/\.allow/.level/' \
	 $< | ${AS} -o $@

# Place code to data section, and make sure all address calculations
# are relative to $global$.
.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -o $@.s -S $<
	@grep -i 'ldil' $@.s | egrep -v "ldil L'-?[0-9]*," || exit 0; \
		echo 'found non-relocatable code' >&2; exit 1
	sed -e 's/\.text/.data/' $@.s | ${AS} -o $@
