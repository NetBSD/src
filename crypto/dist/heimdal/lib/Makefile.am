# $Id: Makefile.am,v 1.1.1.1.4.2 2000/06/16 18:32:20 thorpej Exp $

include $(top_srcdir)/Makefile.am.common

if KRB4
dir_45 = 45
endif
if OTP
dir_otp = otp
endif

SUBDIRS = roken editline com_err sl asn1 des krb5 \
	kafs hdb kadm5 gssapi auth $(dir_45) $(dir_otp)
