# $NetBSD: Makefile.inc,v 1.4 1997/10/17 14:53:18 lukem Exp $
# Original from Freebsd, no rcs id.

LIBINSTALL != cd ${.CURDIR}/../lib;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
CFLAGS+=-I${.CURDIR}/../lib
CPPFLAGS+=${DEBUG}
DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
