#	Makefile,v 1.6 1993/09/10 23:32:17 jtc Exp

ARCH?=${MACHINE}

.if exists (${.CURDIR}/${MACHINE}/Makefile.inc)
.PATH:	${.CURDIR}/${MACHINE}
.include "${.CURDIR}/${MACHINE}/Makefile.inc"
AINC+= -I${.CURDIR}/${MACHINE}
.endif

.if defined(ARCH) && !empty(ARCH)
.if exists (${.CURDIR}/${ARCH}/Makefile.inc)
.PATH:	${.CURDIR}/${ARCH}
.include "${.CURDIR}/${ARCH}/Makefile.inc"
AINC+= -I${.CURDIR}/${ARCH}
.endif
.endif

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

.if exists (${.CURDIR}/${ARCH})
LIB=	${ARCH}

.include <bsd.lib.mk>
.else
all:
clean:
cleandir:
obj:
depend:
.endif
