#	$NetBSD: Makefile,v 1.1 2001/06/05 07:20:42 simonb Exp $

MKMAN=  no

DIST=	   ${.CURDIR}/../../../dist/bzip2

regress:
	bzip2 -1  < ${DIST}/sample1.ref > sample1.rb2
	bzip2 -2  < ${DIST}/sample2.ref > sample2.rb2
	bzip2 -3  < ${DIST}/sample3.ref > sample3.rb2
	bzip2 -d  < ${DIST}/sample1.bz2 > sample1.tst
	bzip2 -d  < ${DIST}/sample2.bz2 > sample2.tst
	bzip2 -ds < ${DIST}/sample3.bz2 > sample3.tst
	cmp ${DIST}/sample1.bz2 sample1.rb2 
	cmp ${DIST}/sample2.bz2 sample2.rb2
	cmp ${DIST}/sample3.bz2 sample3.rb2
	cmp sample1.tst ${DIST}/sample1.ref
	cmp sample2.tst ${DIST}/sample2.ref
	cmp sample3.tst ${DIST}/sample3.ref
	rm -f sample1.rb2 sample2.rb2 sample3.rb2 sample1.tst sample2.tst sample3.tst

.include <bsd.prog.mk>
