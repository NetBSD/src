#	$NetBSD: Makefile.inc,v 1.1 2001/10/25 02:23:21 thorpej Exp $

HOST_CPPFLAGS+=-I${.CURDIR}/../lint1/arch/${MACHINE_CPU}

.if ${OBJECT_FMT} == "a.out"
HOST_CPPFLAGS+=-DTARGET_OBJFMT_AOUT
.elif ${OBJECT_FMT} == "COFF"
HOST_CPPFLAGS+=-DTARGET_OBJFMT_COFF
.elif ${OBJECT_FMT} == "ELF"
HOST_CPPFLAGS+=-DTARGET_OBJFMT_ELF
.else
.BEGIN:
	@echo "Unrecognized OBJECT_FMT ${OBJECT_FMT}"
	@false
.endif
