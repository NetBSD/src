#	$NetBSD: Makefile.sgimips.inc,v 1.1.2.2 2000/12/08 09:30:25 bouyer Exp $

SYSTEM_LD_TAIL=	@echo ${LD} -T ${MIPS}/conf/kern.ldscript.be -Ttext 0x88002000 \
		-e start ${GP} -x -o $@.high '$${SYSTEM_OBJ}' vers.o ; \
		${LD} -T ${MIPS}/conf/kern.ldscript.be -Ttext 0x88002000 \
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
