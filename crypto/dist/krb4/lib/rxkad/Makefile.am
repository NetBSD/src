# $Id: Makefile.am,v 1.1.1.1 2001/09/17 12:09:55 assar Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_des)

lib_LIBRARIES = librxkad.a

noinst_PROGRAMS = fc_test

# This should really be set from configure
AFSWS = @AFSWS@
KRB5 = /usr/heimdal

librxkad_a_SOURCES = rxk_locl.c rxk_clnt.c rxk_serv.c rxk_crpt.c \
	rxk_info.c osi_alloc.c compat.c

INCLUDES += -I$(AFSWS)/include -I$(KRB5)/include

fc_test_SOURCES = 

fc_test.o: rxk_crpt.c
	$(COMPILE) -DTEST -c rxk_crpt.c

rxincludedir = $(includedir)/rx

rxinclude_HEADERS = rxkad.h

