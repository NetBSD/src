#	$Id: Makefile,v 1.5 1993/12/05 14:51:56 cgd Exp $

.if exists(${.CURDIR}/${MACHINE})
.PATH:	${.CURDIR}/${MACHINE}
.include "${.CURDIR}/${MACHINE}/Makefile.inc"
AINC+= -I${.CURDIR}/${MACHINE}
.endif

.if exists(${.CURDIR}/${MACHINE_ARCH}) && (${MACHINE} != ${MACHINE_ARCH})
.PATH:	${.CURDIR}/${MACHINE_ARCH}
.include "${.CURDIR}/${MACHINE_ARCH}/Makefile.inc"
AINC+= -I${.CURDIR}/${MACHINE_ARCH}
.endif

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

.if exists(${.CURDIR}/${MACHINE}) || exists(${.CURDIR}/${MACHINE_ARCH})
LIB=	${MACHINE}

.include <bsd.lib.mk>
.else
all:
clean:
cleandir:
depend:
install:
obj:
.endif
