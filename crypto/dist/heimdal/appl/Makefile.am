# $Heimdal: Makefile.am,v 1.24 2001/01/27 18:34:39 assar Exp $
# $NetBSD: Makefile.am,v 1.1.1.4 2002/09/12 12:41:33 joda Exp $

include $(top_srcdir)/Makefile.am.common

if OTP
dir_otp = otp
endif
if DCE
dir_dce = dceutils
endif
SUBDIRS = 					\
	  afsutil				\
	  ftp					\
	  login					\
	  $(dir_otp)				\
	  popper				\
	  push					\
	  rsh					\
	  rcp					\
	  su					\
	  xnlock				\
	  telnet				\
	  test					\
	  kx					\
	  kf					\
	  $(dir_dce)
