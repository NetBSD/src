# $NetBSD: Makefile.inc,v 1.5 1997/10/25 06:58:29 lukem Exp $
# Original from Freebsd, no rcs id.

LIBINSTALL != cd ${.CURDIR}/../lib;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../lib ${DEBUG}
DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
