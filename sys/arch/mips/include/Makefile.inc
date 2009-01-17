# $NetBSD: Makefile.inc,v 1.6.60.1 2009/01/17 13:28:15 mjf Exp $

CFLAGS+=	-G 0 -mno-abicalls -ffixed-23

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
CFLAGS+=	-msym32
.endif

AFLAGS+=	-mno-abicalls -x assembler-with-cpp -traditional-cpp ${AOPTS}
