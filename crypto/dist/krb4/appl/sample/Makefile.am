# $Id: Makefile.am,v 1.1.1.1 2001/09/17 12:09:44 assar Exp $

include $(top_srcdir)/Makefile.am.common

bin_PROGRAMS = sample_client simple_client
libexec_PROGRAMS = sample_server simple_server

LDADD = \
	$(top_builddir)/lib/krb/libkrb.la	\
	$(LIB_des)	\
	$(LIB_roken)
