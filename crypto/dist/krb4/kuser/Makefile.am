# $KTH-KRB: Makefile.am,v 1.5 2002/09/09 13:24:13 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:08 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_des)

bin_PROGRAMS = kinit kdestroy klist

man_MANS = kinit.1 kdestroy.1 klist.1

LDADD = \
	$(LIB_krb4)	\
	$(LIB_des)	\
	$(LIB_roken)

klist_LDADD = \
	$(LIB_kafs)	\
	$(LDADD)

kdestroy_LDADD = \
	$(LIB_kafs)	\
	$(LDADD)
