# $Id: Makefile,v 1.1 2011/08/25 19:09:46 dyoung Exp $

.include <bsd.own.mk>

LIBPPATH != make -V .OBJDIR -C $(.CURDIR)/../lib

TESTS_C=t_ppath t_proplib
SRCS.t_proplib=t_proplib.c personnel.c personnel.h
SRCS.t_ppath=t_ppath.c personnel.c personnel.h
CPPFLAGS+=-I$(.OBJDIR)

.SUFFIXES: .plist .h

.plist.h:
	echo "extern const char " ${.TARGET:S,.h$,,} "[];" > ${.TARGET}

.plist.c:
	${.CURDIR}/plist_to_c ${.TARGET:S,.c$,,} < ${.IMPSRC} > ${.TARGET}

CLEANFILES+=personnel.c personnel.h

LDADD+=-L$(LIBPPATH) -lppath -lprop
DPADD+=$(LIBPPATH)/libppath.a

.include <bsd.test.mk>
.include "../mk/tags.mk"
