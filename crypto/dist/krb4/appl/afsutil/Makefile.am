# $Id: Makefile.am,v 1.1.1.1 2001/09/17 12:09:42 assar Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_des)

bin_PROGRAMS = pagsh afslog kstring2key

afslog_SOURCES = aklog.c

man_MANS = afslog.1 pagsh.1

LDADD = \
	$(LIB_kafs) \
	$(LIB_krb4) \
	$(LIB_des)	\
	$(LIB_roken)
