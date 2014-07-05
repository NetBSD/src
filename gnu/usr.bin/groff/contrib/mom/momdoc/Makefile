# $NetBSD: Makefile,v 1.4 2014/07/05 19:22:42 dholland Exp $

DIST_SUBDIR=    contrib/mom/momdoc

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=       /usr/share/doc/reference/ref1/roff/mom
FILES=          appendices.html \
  color.html \
  cover.html \
  definitions.html \
  docelement.html \
  docprocessing.html \
  goodies.html \
  headfootpage.html \
  inlines.html \
  intro.html \
  letters.html \
  macrolist.html \
  rectoverso.html \
  refer.html \
  reserved.html \
  toc.html \
  typemacdoc.html \
  typesetting.html \
  using.html
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
