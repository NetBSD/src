# $Id: Makefile.am,v 1.1.1.3 2001/02/11 13:51:10 assar Exp $

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
