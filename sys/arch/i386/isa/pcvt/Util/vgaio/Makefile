
.if !exists(${.CURDIR}/../Makefile.inc)
error:
	@echo
	@echo "                      You MUST link/copy"
	@echo
	@echo "       ../Makefile.inc.NetBSD    or   ../Makefile.inc.FreeBSD"
	@echo
	@echo "             to ../Makefile.inc before you can proceed !"
	@echo
.else

PROG=	vgaio

SRCS=	vgaio.y lex.l
YACC=	bison
YFLAGS+= -yd
#YFLAGS+= -v	# verbose
LFLAGS+= -I

MAN8= vgaio.0

CFLAGS+= -I ${.CURDIR}/obj -I ${.CURDIR} -g

CLEANFILES+= y.tab.h
CLEANFILES+= y.output	# comment file from bison

install:	$(PROG) $(MAN8)
	@echo "vgaio is not installed automatically ...."

.include <bsd.prog.mk>

.endif
