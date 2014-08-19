/*	$NetBSD: hunt.h,v 1.19.12.1 2014/08/20 00:00:23 tls Exp $	*/

/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/poll.h>

#include "hunt_common.h"

extern const int shot_req[];
extern const int shot_type[];
#ifdef OOZE
extern const int slime_req[];
#endif

typedef struct bullet_def	BULLET;
typedef struct expl_def		EXPL;
typedef struct player_def	PLAYER;
typedef struct ident_def	IDENT;
typedef struct regen_def	REGEN;

struct ident_def {
	char i_name[WIRE_NAMELEN];
	char i_team;
	uint32_t i_machine;
	uint32_t i_uid;
	float i_kills;
	int i_entries;
	float i_score;
	int i_absorbed;
	int i_faced;
	int i_shot;
	int i_robbed;
	int i_slime;
	int i_missed;
	int i_ducked;
	int i_gkills, i_bkills, i_deaths, i_stillb, i_saved;
	IDENT *i_next;
};

struct player_def {
	IDENT *p_ident;
	char p_over;
	int p_face;
	int p_undershot;
#ifdef FLY
	int p_flying;
	int p_flyx, p_flyy;
#endif
#ifdef BOOTS
	int p_nboots;
#endif
	FILE *p_output;
	int p_fd;
	int p_mask;
	int p_damage;
	int p_damcap;
	int p_ammo;
	int p_ncshot;
	int p_scan;
	int p_cloak;
	int p_x, p_y;
	int p_ncount;
	int p_nexec;
	long p_nchar;
	char p_death[MSGLEN];
	char p_maze[HEIGHT][WIDTH2];
	int p_curx, p_cury;
	int p_lastx, p_lasty;
	char p_cbuf[BUFSIZ];
};

struct bullet_def {
	int b_x, b_y;
	int b_face;
	int b_charge;
	char b_type;
	char b_size;
	char b_over;
	PLAYER *b_owner;
	IDENT *b_score;
	bool b_expl;
	BULLET *b_next;
};

struct expl_def {
	int e_x, e_y;
	char e_char;
	EXPL *e_next;
};

struct regen_def {
	int r_x, r_y;
	REGEN *r_next;
};

/*
 * external variables
 */

extern char Buf[BUFSIZ], Maze[HEIGHT][WIDTH2], Orig_maze[HEIGHT][WIDTH2];

extern int Nplayer;
extern int huntsock;
extern struct pollfd fdset[];

extern int See_over[NASCII];

extern BULLET *Bullets;

extern IDENT *Scores;

extern PLAYER Player[MAXPL], *End_player;
#ifdef BOOTS
extern PLAYER Boot[NBOOTS];
#endif

#ifdef MONITOR
extern PLAYER Monitor[MAXMON], *End_monitor;
#endif

/*
 * function types
 */

/* in answer.c */
bool answer(void);
int rand_dir(void);

/* in draw.c */
void drawmaze(PLAYER *);
void look(PLAYER *);
void check(PLAYER *, int, int);
void showstat(PLAYER *);
void drawplayer(PLAYER *, bool);
void message(PLAYER *, const char *);

/* in driver.c */
void checkdam(PLAYER *, PLAYER *, IDENT *, int, char);
int rand_num(int);
__dead void cleanup(int);

/* in execute.c */
void mon_execute(PLAYER *);	/* ifdef MONITOR only */
void execute(PLAYER *);
void add_shot(int, int, int, char, int, PLAYER *, int, char);
BULLET *create_shot(int, int, int, char, int, int, PLAYER *,
		    IDENT *, int, char);

/* in expl.c */
void showexpl(int, int, char);
void rollexpl(void);
void clearwalls(void);

/* in makemaze.c */
void makemaze(void);

/* in shots.c */
void moveshots(void);
PLAYER *play_at(int, int);
bool opposite(int, char);
BULLET *is_bullet(int, int);
void fixshots(int, int, char);

/* in support.c */
__printflike(2, 3) void complain(int level, const char *fmt, ...);

/* in terminal.c */
void cgoto(PLAYER *, int, int);
void outch(PLAYER *, int);
void outstr(PLAYER *, const char *, int);
void clrscr(PLAYER *);
void ce(PLAYER *);
void sendcom(PLAYER *, int, ...);
