# $KTH-KRB: Makefile.am,v 2.5 2002/04/26 12:22:41 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:14 joda Exp $

include $(top_srcdir)/Makefile.am.common

libexec_PROGRAMS = kprop kpropd

man_MANS = kprop.8 kpropd.8

LDADD = \
	$(top_builddir)/lib/krb/libkrb.la	\
	$(LIB_des)	\
	$(LIB_roken)

INCLUDES += $(INCLUDE_des)
