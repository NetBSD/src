# $KTH-KRB: Makefile.am,v 1.3 2002/04/02 07:06:43 joda Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:08 joda Exp $

include $(top_srcdir)/Makefile.am.common

AUTOMAKE_OPTIONS += no-texinfo.tex
TEXI2HTML = texi2html

info_TEXINFOS = kth-krb.texi

kth_krb_TEXINFOS = \
	ack.texi				\
	index.texi				\
	install.texi				\
	intro.texi				\
	otp.texi				\
	problems.texi				\
	setup.texi				\
	whatis.texi

.PHONY: html
html: kth-krb.html

kth-krb.html: $(kth_krb_TEXINFOS)
	$(TEXI2HTML) $(srcdir)/kth-krb.texi
