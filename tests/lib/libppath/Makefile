# $Id: Makefile,v 1.2 2011/08/27 18:55:52 dyoung Exp $

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
	${.CURDIR}/plist_to_c ${.TARGET:S,.c$,,} < ${.IMPSRC} > ${.TARGET}

CLEANFILES+=personnel.c personnel.h

#LDADD+=-L$(LIBPPATH)
LDADD+=-lppath -lprop
#DPADD+=$(LIBPPATH)/libppath.a

.include <bsd.test.mk>
#.include "../mk/tags.mk"
