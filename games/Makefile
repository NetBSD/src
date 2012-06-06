#	$NetBSD: Makefile,v 1.27 2012/06/06 00:16:01 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 7/24/94

# Missing: dungeon warp
# Moved: chess
# Don't belong: xneko xroach

.include <bsd.own.mk>

SUBDIR=	adventure arithmetic atc \
	backgammon banner battlestar bcd boggle \
	caesar canfield ching colorbars countmail cribbage \
	dm factor fish fortune gomoku \
	hack hangman hunt larn mille monop morse number \
	phantasia pig pom ppt primes quiz \
	rain random robots rogue sail snake tetris trek \
	wargames worm worms wtf wump

.if ${MKCXX} != "no"
SUBDIR+=	dab 
.endif

.include <bsd.subdir.mk>
