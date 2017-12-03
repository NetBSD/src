#	$NetBSD: Makefile.zaurus.inc,v 1.4.6.2 2017/12/03 11:36:52 jdolecek Exp $

MACHINE_ARCH?=		arm
CPPFLAGS+=		-D${MACHINE}

SYSTEM_FIRST_OBJ=	zaurus_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/zaurus/zaurus_start.S

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

KERNEL_BASE_VIRT=	$(LOADADDRESS)

KERNLDSCRIPT=		ldscript

EXTRA_CLEAN+=		netbsd.map assym.d ldscript tmp

# generate ldscript from common template
ldscript: ${THISARM}/conf/ldscript.zaurus ${THISARM}/conf/Makefile.zaurus.inc Makefile
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.zaurus > tmp && mv tmp $@
