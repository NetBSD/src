# $Id: Makefile.am,v 1.1.1.1 2001/09/17 12:09:53 assar Exp $

include $(top_srcdir)/Makefile.am.common

if OTP
dir_otp = otp
endif

SUBDIRS	= @DIR_roken@ vers com_err @DIR_des@ krb kdb kadm acl \
	kafs auth editline sl $(dir_otp)
