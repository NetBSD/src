# $Id: Makefile.am,v 1.1.1.1.2.1 2001/04/05 23:22:45 he Exp $

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
