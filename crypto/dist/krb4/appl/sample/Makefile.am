# $KTH-KRB: Makefile.am,v 1.4 2002/02/06 03:00:04 assar Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:04 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_krb4)

bin_PROGRAMS = sample_client simple_client
libexec_PROGRAMS = sample_server simple_server

LDADD = \
	$(top_builddir)/lib/krb/libkrb.la	\
	$(LIB_des)	\
	$(LIB_roken)
