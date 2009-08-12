/*	$NetBSD: hunt.h,v 1.19 2009/08/12 07:42:11 dholland Exp $	*/

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

#include "bsd.h"

#include <stdio.h>
#include <string.h>

#ifdef LOG
#include <syslog.h>
#endif

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/poll.h>

#ifdef INTERNET
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#else
#include <sys/un.h>
#endif

#ifdef INTERNET
#define SOCK_FAMILY	AF_INET
#else
#define SOCK_FAMILY	AF_UNIX
#define AF_UNIX_HACK			/* 4.2 hack; leaves files around */
#endif

/*
 * Preprocessor define dependencies
 */
#if defined(VOLCANO) && !defined(OOZE)
#define OOZE
#endif
#if defined(BOOTS) && !defined(FLY)
#define FLY
#endif
#if !defined(REFLECT) && !defined(RANDOM)
#define RANDOM
#endif

/* decrement version number for each change in startup protocol */
#define HUNT_VERSION		-1

#define ADDCH		('a' | 0200)
#define MOVE		('m' | 0200)
#define REFRESH		('r' | 0200)
#define CLRTOEOL	('c' | 0200)
#define ENDWIN		('e' | 0200)
#define CLEAR		('C' | 0200)
#define REDRAW		('R' | 0200)
#define LAST_PLAYER	('l' | 0200)
#define BELL		('b' | 0200)
#define READY		('g' | 0200)

/*
 * Choose MAXPL and MAXMON carefully.  The screen is assumed to be
 * 23 lines high and will only tolerate (MAXPL == 17 && MAXMON == 0)
 * or (MAXPL + MAXMON <= 16).
 */
#ifdef MONITOR
#define MAXPL		15
#define MAXMON		1
#else
#define MAXPL		17
#define MAXMON		0
#endif
#define SHORTLEN	2		/* sizeof (network short) */
#define LONGLEN		4		/* sizeof (network long) */
#define NAMELEN		20
#define MSGLEN		SCREEN_WIDTH
#define DECAY		50.0

#define NASCII		128

#define WIDTH		51
#define WIDTH2		64	/* Next power of 2 >= WIDTH (for fast access) */
#define HEIGHT		23
#define UBOUND		1
#define DBOUND		(HEIGHT - 1)
#define LBOUND		1
#define RBOUND		(WIDTH - 1)

#define SCREEN_HEIGHT	24
#define SCREEN_WIDTH	80
#define SCREEN_WIDTH2	128	/* Next power of 2 >= SCREEN_WIDTH */

#define STAT_LABEL_COL	60
#define STAT_VALUE_COL	74
#define STAT_NAME_COL	61
#define STAT_SCAN_COL	(STAT_NAME_COL + 5)
#define STAT_AMMO_ROW	0
#define STAT_GUN_ROW	1
#define STAT_DAM_ROW	2
#define STAT_KILL_ROW	3
#define STAT_PLAY_ROW	5
#ifdef MONITOR
#define STAT_MON_ROW	(STAT_PLAY_ROW + MAXPL + 1)
#endif
#define STAT_NAME_LEN	18

#define DOOR		'#'
#define WALL1		'-'
#define WALL2		'|'
#define WALL3		'+'
#ifdef REFLECT
#define WALL4		'/'
#define WALL5		'\\'
#endif
#define KNIFE		'K'
#define SHOT		':'
#define GRENADE		'o'
#define SATCHEL		'O'
#define BOMB		'@'
#define MINE		';'
#define GMINE		'g'
#ifdef OOZE
#define SLIME		'$'
#endif
#ifdef VOLCANO
#define LAVA		'~'
#endif
#ifdef DRONE
#define DSHOT		'?'
#endif
#ifdef FLY
#define FALL		'F'
#endif
#ifdef BOOTS
#define NBOOTS		2
#define BOOT		'b'
#define BOOT_PAIR	'B'
#endif
#define SPACE		' '

#define ABOVE		'i'
#define BELOW		'!'
#define RIGHT		'}'
#define LEFTS		'{'
#ifdef FLY
#define FLYER		'&'
#define isplayer(c)	(c == LEFTS || c == RIGHT ||\
			 c == ABOVE || c == BELOW || c == FLYER)
#else
#define	isplayer(c)	(c == LEFTS || c == RIGHT ||\
			 c == ABOVE || c == BELOW)
#endif

#define NORTH	01
#define SOUTH	02
#define EAST	010
#define WEST	020

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#undef CTRL
#define CTRL(x) ((x) & 037)

#define BULSPD		5		/* bullets movement speed */
#define ISHOTS		15
#define NSHOTS		5
#define MAXNCSHOT	2
#define MAXDAM		10
#define MINDAM		5
#define STABDAM		2

#define BULREQ		1
#define GRENREQ		9
#define SATREQ		25
#define BOMB7REQ	49
#define BOMB9REQ	81
#define BOMB11REQ	121
#define BOMB13REQ	169
#define BOMB15REQ	225
#define BOMB17REQ	289
#define BOMB19REQ	361
#define BOMB21REQ	441
#define MAXBOMB		11
#ifdef DRONE
#define MINDSHOT	2	/* At least a satchel bomb */
#endif
extern int shot_req[];
extern int shot_type[];
#ifdef OOZE
#define SLIME_FACTOR	3
#define SLIMEREQ	5
#define SSLIMEREQ	10
#define SLIME2REQ	15
#define SLIME3REQ	20
#define MAXSLIME	4
#define SLIMESPEED	5
extern int slime_req[];
#endif
#ifdef VOLCANO
#define LAVASPEED	1
#endif

#define CLOAKLEN	20
#define SCANLEN		(Nplayer * 20)
#define EXPLEN		4

#define Q_QUIT		0
#define Q_CLOAK		1
#define Q_FLY		2
#define Q_SCAN		3
#define Q_MESSAGE	4

#define C_PLAYER	0
#define C_MONITOR	1
#define C_MESSAGE	2
#define C_SCORES	3

#ifdef MONITOR
#define C_TESTMSG()	(Query_driver ? C_MESSAGE :\
			(Show_scores ? C_SCORES :\
			(Am_monitor ? C_MONITOR :\
			C_PLAYER)))
#else
#define	C_TESTMSG()	(Show_scores ? C_SCORES :\
			(Query_driver ? C_MESSAGE :\
			C_PLAYER))
#endif

#ifdef FLY
#define _scan_char(pp)	(((pp)->p_scan < 0) ? ' ' : '*')
#define _cloak_char(pp)	(((pp)->p_cloak < 0) ? _scan_char(pp) : '+')
#define stat_char(pp)	(((pp)->p_flying < 0) ? _cloak_char(pp) : FLYER)
#else
#define _scan_char(pp)	(((pp)->p_scan < 0) ? ' ' : '*')
#define stat_char(pp)	(((pp)->p_cloak < 0) ? _scan_char(pp) : '+')
#endif

typedef int			FLAG;
typedef struct bullet_def	BULLET;
typedef struct expl_def		EXPL;
typedef struct player_def	PLAYER;
typedef struct ident_def	IDENT;
typedef struct regen_def	REGEN;
#ifdef INTERNET
typedef struct sockaddr_in	SOCKET;
#else
typedef struct sockaddr_un	SOCKET;
#endif

struct ident_def {
	char i_name[NAMELEN];
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
	FLAG b_expl;
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

extern FLAG Last_player;

extern char Buf[BUFSIZ], Maze[HEIGHT][WIDTH2], Orig_maze[HEIGHT][WIDTH2];

extern const char *Driver;

extern int Nplayer, Socket, Status;
extern struct pollfd fdset[];

#ifdef INTERNET
extern u_short Test_port;
#else
extern char *Sock_name, *Stat_name;
#endif

#ifdef VOLCANO
extern int volcano;
#endif

extern int See_over[NASCII];

extern BULLET *Bullets;

extern EXPL *Expl[EXPLEN];
extern EXPL *Last_expl;

extern IDENT *Scores;

extern PLAYER Player[MAXPL], *End_player;
#ifdef BOOTS
extern PLAYER Boot[NBOOTS];
#endif

#ifdef MONITOR
extern FLAG Am_monitor;
extern PLAYER Monitor[MAXMON], *End_monitor;
#endif

#ifdef INTERNET
extern char *Send_message;
#endif

extern char map_key[256];
extern FLAG no_beep;

/*
 * function types
 */

void add_shot(int, int, int, char, int, PLAYER *, int, char);
int answer(void);
void bad_con(void) __dead;
void bad_ver(void) __dead;
void ce(PLAYER *);
void cgoto(PLAYER *, int, int);
void check(PLAYER *, int, int);
void checkdam(PLAYER *, PLAYER *, IDENT *, int, char);
void clearwalls(void);
void clear_eol(void);
void clear_the_screen(void);
void clrscr(PLAYER *);
BULLET *create_shot(int, int, int, char, int, int, PLAYER *,
		    IDENT *, int, char);
void do_connect(char *, char, long);
void do_message(void);
void drawmaze(PLAYER *);
void drawplayer(PLAYER *, FLAG);
void execute(PLAYER *);
void faketalk(void);
void fixshots(int, int, char);
void get_local_name(char *);
int get_remote_name(char *);
BULLET *is_bullet(int, int);
void look(PLAYER *);
void makemaze(void);
void message(PLAYER *, const char *);
void mon_execute(PLAYER *);
void moveshots(void);
void open_ctl(void);
int opposite(int, char);
void otto(int, int, char);
void outch(PLAYER *, int);
void outstr(PLAYER *, const char *, int);
PLAYER *play_at(int, int);
void playit(void);
void put_ch(char);
void put_str(char *);
int quit(int);
int rand_dir(void);
int rand_num(int);
void rollexpl(void);
void sendcom(PLAYER *, int, ...);
void showexpl(int, int, char);
void showstat(PLAYER *);
void cleanup(int) __dead;
void intr(int);
void tstp(int);
