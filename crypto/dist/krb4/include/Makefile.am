# $KTH-KRB: Makefile.am,v 1.4 2002/05/02 16:41:01 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:08 joda Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIR = sys

noinst_PROGRAMS = bits

INCLUDES += -DHOST=\"$(CANONICAL_HOST)\"

include_HEADERS = ktypes.h protos.h $(des_h)

if HAVE_OPENSSL
des_h = des.h
des.h:
	echo '#include <@DES_H@>' > des.h
else
des_h = 
endif


ktypes.h: bits$(EXEEXT)
	./bits$(EXEEXT) ktypes.h

CLEANFILES =					\
	acl.h					\
	base64.h				\
	com_err.h				\
	com_right.h				\
	des.h					\
	editline.h				\
	err.h					\
	getarg.h				\
	glob.h					\
	kadm.h					\
	kadm_err.h				\
	kafs.h					\
	kdc.h					\
	klog.h					\
	krb-archaeology.h			\
	krb-protos.h				\
	krb.h					\
	krb_db.h				\
	krb_err.h				\
	krb_log.h				\
	ktypes.h				\
	md4.h					\
	md5.h					\
	otp.h					\
	parse_bytes.h				\
	parse_time.h				\
	parse_units.h				\
	prot.h					\
	protos.h				\
	roken.h					\
	rc4.h					\
	resolve.h				\
	roken-common.h				\
	sha.h					\
	sl.h					\
	xdbm.h

protos.h: $(srcdir)/protos.hin
	$(LN_S) $(srcdir)/protos.hin protos.h
