# $Id: Makefile.am,v 1.1.1.1 2001/09/17 12:09:42 assar Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_des)

sbin_PROGRAMS = ext_srvtab kdb_destroy kdb_edit kdb_init kdb_util kstash

man_MANS = ext_srvtab.8 kdb_destroy.8 kdb_edit.8 kdb_init.8 kdb_util.8 kstash.8

LDADD = \
	$(top_builddir)/lib/kdb/libkdb.la	\
	$(top_builddir)/lib/krb/libkrb.la	\
	$(LIB_des)	\
	$(LIB_roken)	\
	$(DBLIB)
