#	$NetBSD: Makefile.sgimips.inc,v 1.3 2001/10/05 14:57:18 pooka Exp $

SYSTEM_LD_TAIL=	@echo ${LD} -N -T ${MIPS}/conf/kern.ldscript -Ttext 0x88002000 \
		-e start ${GP} -x -o $@.high '$${SYSTEM_OBJ}' vers.o ; \
		${LD} -N -T ${MIPS}/conf/kern.ldscript -Ttext 0x88002000 \
		-e start ${GP} -x -o $@.high ${SYSTEM_OBJ} vers.o swapnetbsd.o
# XXX
SYSTEM_LD_TAIL+=; \
		${SIZE} $@; chmod 755 $@ ; \
		${SIZE} $@.high; chmod 755 $@.high

# Used if DEBUG != ""
DEBUG_SYSTEM_LD_TAIL+=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb
