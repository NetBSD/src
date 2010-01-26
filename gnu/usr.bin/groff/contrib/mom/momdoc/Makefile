# $NetBSD: Makefile,v 1.3 2010/01/26 15:11:56 wiz Exp $

DIST_SUBDIR=    contrib/mom/momdoc

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=       ${HTMLDOCDIR}/groff/mom
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
