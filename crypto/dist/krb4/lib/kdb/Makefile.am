# $KTH-KRB: Makefile.am,v 1.8 2002/09/09 19:17:05 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:09 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += -DROKEN_RENAME $(INCLUDE_des)

include_HEADERS =  krb_db.h kdc.h

build_HEADERZ = $(include_HEADERS)

lib_LTLIBRARIES = libkdb.la
libkdb_la_LDFLAGS = -version-info 1:11:0
libkdb_la_LIBADD  = ../krb/libkrb.la $(LIB_des)

if HAVE_DB3
ndbm_wrap = ndbm_wrap.c ndbm_wrap.h
else
ndbm_wrap =
endif
CLEANFILES = $(ndbm_wrap) roken_rename.h

libkdb_la_SOURCES =				\
	copykey.c				\
	krb_cache.c				\
	krb_dbm.c				\
	krb_kdb_utils.c				\
	krb_lib.c				\
	print_princ.c				\
	$(ndbm_wrap)

$(libkdb_la_OBJECTS): ../../include/config.h roken_rename.h $(ndbm_wrap)

roken_rename.h:
	$(LN_S) $(srcdir)/../krb/roken_rename.h .

ndbm_wrap.c:
	$(LN_S) $(srcdir)/../roken/ndbm_wrap.c .
ndbm_wrap.h:
	(echo '#define dbm_rename(X) __krb_kdb_ ## X'; cat $(srcdir)/../roken/ndbm_wrap.h) > ndbm_wrap.h
