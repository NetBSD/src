# $Id: Makefile.am,v 1.1.1.1 2001/09/17 12:10:06 assar Exp $

include $(top_srcdir)/Makefile.am.common

libexec_PROGRAMS = kprop kpropd

man_MANS = kprop.8 kpropd.8

LDADD = \
	$(top_builddir)/lib/krb/libkrb.la	\
	$(LIB_des)	\
	$(LIB_roken)

INCLUDES += -DSBINDIR=\"$(sbindir)\"
