#	$NetBSD: Makefile.zaurus.inc,v 1.2.76.1 2011/05/31 03:04:25 rmind Exp $

MACHINE_ARCH=		arm
CPPFLAGS+=		-D${MACHINE}

SYSTEM_FIRST_OBJ=	zaurus_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/zaurus/zaurus_start.S

LINKFLAGS=		-T ${THISARM}/conf/ldscript

EXTRA_CLEAN+= netbsd.map assym.d

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "Checking kernel size..."; \
	size=`wc -c "$@" | ${TOOL_AWK} '{ print $$1 }'`; \
	maxsize=5242880; \
	if [ $$size -gt $$maxsize ]; \
	then \
		echo "Fatal: kernel size must be less than 5MB."; \
		echo "Fatal: kernel size: $$size, max kernel size: $$maxsize"; \
		false; \
	fi
