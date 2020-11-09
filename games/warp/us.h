/* $Header: /cvsroot/src/games/warp/us.h,v 1.1 2020/11/09 23:37:05 kamil Exp $ */

/* $Log: us.h,v $
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
 * Revision 7.0.1.1  86/10/16  10:53:58  lwall
 * Added Damage.  Fixed random bugs.
 * 
 * Revision 7.0  86/10/08  15:14:27  lwall
 * Split into separate files.  Added amoebas and pirates.
 * 
 */

EXT bool cloaking;
EXT bool cloaked;

EXT int status;
EXT int entmode;

EXT int evely;
EXT int evelx;
EXT int bvely;
EXT int bvelx;

#define MAXDAMAGE 9
#define NOWARP 0
#define NOIMPULSE 1
#define NOPHASERS 2
#define NOTORPS 3
#define NOCLOAKING 4
#define NOSHIELDS 5
#define NOZAPPER 6
#define NODESTRUCT 7
#define NOTRACTORS 8

EXT int dam INIT(0);
EXT int lastdam INIT(-1);
EXT int damage INIT(0);
EXT int olddamage INIT(-1);

#ifdef DOINIT
char *dammess[MAXDAMAGE] = {
    "WARP",
    "IMPULSE",
    "PHASERS",
    "TORPS",
    "CLOAKING",
    "SHIELDS",
    "ZAPPER",
    "DESTRUCT",
    "TRACTORS"
};
char damflag[MAXDAMAGE] = {0,0,0,0,0,0,0,0,0};
#else
extern char *dammess[];
extern char damflag[];
#endif

void do_direction();
void ctrl_direction();
void shift_direction();
void get_commands();
void us_init();
