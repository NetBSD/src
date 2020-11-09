/* $Header: /cvsroot/src/games/warp/score.h,v 1.1 2020/11/09 23:37:05 kamil Exp $ */

/* $Log: score.h,v $
/* Revision 1.1  2020/11/09 23:37:05  kamil
/* Add Warp Kit, Version 7.0 by Larry Wall
/*
/* Warp is a real-time space war game that doesn't get boring very quickly.
/* Read warp.doc and the manual page for more information.
/*
/* games/warp originally distributed with 4.3BSD-Reno, is back to the BSD
/* world via NetBSD. Its remnants were still mentioned in games/Makefile.
/*
/* Larry Wall, the original author and the copyright holder, generously
/* donated the game and copyright to The NetBSD Foundation, Inc.
/*
/* Import the game sources as-is from 4.3BSD-Reno, with the cession
/* of the copyright and license to BSD-2-clause NetBSD-style.
/*
/* Signed-off-by: Larry Wall <larry@wall.org>
/* Signed-off-by: Kamil Rytarowski <kamil@netbsd.org>
/*
 * Revision 7.0  86/10/08  15:13:21  lwall
 * Split into separate files.  Added amoebas and pirates.
 * 
 */

#define ENTBOUNDARY 100000	/*  point boundary across which a new E is
					awarded */

#define BASEBOUNDARY 250000	/*  point boundary across which a new B is
					awarded */

EXT int oldstatus;
EXT int oldetorp;
EXT int oldbtorp;
EXT int oldstrs;
EXT int oldenemies;

EXT long totalscore;
EXT long lastscore INIT(0);
EXT long curscore;
EXT long possiblescore;
EXT long oldeenergy;
EXT long oldbenergy;
EXT long oldcurscore;

EXT char savefilename[40];

#ifdef SCOREFULL
#define COMPOFF 0
#define COMPNAME longlognam
#define COMPLEN 24
#else
#define COMPOFF 24
#define COMPNAME longlognam
#define COMPLEN 8
#endif
EXT char longlognam[128];

EXT char c INIT(' ');

void score_init();
void wscore();
void display_status();
void wavescore();
void score();
void save_game();
