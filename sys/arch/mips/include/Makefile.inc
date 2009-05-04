# $NetBSD: Makefile.inc,v 1.6.64.1 2009/05/04 08:11:31 yamt Exp $

CFLAGS+=	-G 0 -mno-abicalls -ffixed-23

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
CFLAGS+=	-msym32
.endif

AFLAGS+=	-mno-abicalls -x assembler-with-cpp -traditional-cpp ${AOPTS}
