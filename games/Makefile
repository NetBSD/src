#	$NetBSD: Makefile,v 1.19 2002/06/16 10:21:40 martin Exp $
#	@(#)Makefile	8.3 (Berkeley) 7/24/94

# Missing: ching dungeon warp
# Moved: chess
# Don't belong: xneko xroach

# For MKCRYPTO
.include <bsd.own.mk>

SUBDIR=	adventure arithmetic atc backgammon banner battlestar bcd boggle \
	caesar canfield countmail cribbage dm fish fortune gomoku hack \
	hangman hunt larn mille monop morse number phantasia pig pom ppt \
	primes quiz rain random robots rogue sail snake tetris trek wargames \
	worm worms wtf wump

.if (${MKCRYPTO} != "no")
SUBDIR+= factor
.endif

.include <bsd.subdir.mk>
