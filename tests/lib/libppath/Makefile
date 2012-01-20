# $Id: Makefile,v 1.3 2012/01/20 15:06:47 tsutsui Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libppath

#LIBPPATH != make -V .OBJDIR -C $(.CURDIR)/../lib

TESTS_C=t_ppath
SRCS.t_ppath=t_ppath.c personnel.c personnel.h
CPPFLAGS+=-I$(.OBJDIR)

.SUFFIXES: .plist .h

.plist.h:
	echo "extern const char " ${.TARGET:S,.h$,,} "[];" > ${.TARGET}

.plist.c:
	${HOST_SH} ${.CURDIR}/plist_to_c ${.TARGET:S,.c$,,} < ${.IMPSRC} > ${.TARGET}

CLEANFILES+=personnel.c personnel.h

#LDADD+=-L$(LIBPPATH)
LDADD+=-lppath -lprop
#DPADD+=$(LIBPPATH)/libppath.a

.include <bsd.test.mk>
#.include "../mk/tags.mk"
