/*	$NetBSD: sync.c,v 1.25.12.1 2009/05/13 19:18:06 jym Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)sync.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: sync.c,v 1.25.12.1 2009/05/13 19:18:06 jym Exp $");
#endif
#endif /* not lint */

#include <sys/stat.h>

#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "extern.h"
#include "pathnames.h"

#define BUFSIZE 4096

/* Message types */
#define W_CAPTAIN	1
#define W_CAPTURED	2
#define W_CLASS		3
#define W_CREW		4
#define W_DBP		5
#define W_DRIFT		6
#define W_EXPLODE	7
/*      W_FILE		8   not used */
#define W_FOUL		9
#define W_GUNL		10
#define W_GUNR		11
#define W_HULL		12
#define W_MOVE		13
#define W_OBP		14
#define W_PCREW		15
#define W_UNFOUL	16
#define W_POINTS	17
#define W_QUAL		18
#define W_UNGRAP	19
#define W_RIGG		20
#define W_COL		21
#define W_DIR		22
#define W_ROW		23
#define W_SIGNAL	24
#define W_SINK		25
#define W_STRUCK	26
#define W_TA		27
#define W_ALIVE		28
#define W_TURN		29
#define W_WIND		30
#define W_FS		31
#define W_GRAP		32
#define W_RIG1		33
#define W_RIG2		34
#define W_RIG3		35
#define W_RIG4		36
#define W_BEGIN		37
#define W_END		38
#define W_DDEAD		39


static void recv_captain(struct ship *ship, const char *astr);
static void recv_captured(struct ship *ship, long a);
static void recv_class(struct ship *ship, long a);
static void recv_crew(struct ship *ship, long a, long b, long c);
static void recv_dbp(struct ship *ship, long a, long b, long c, long d);
static void recv_drift(struct ship *ship, long a);
static void recv_explode(struct ship *ship, long a);
static void recv_foul(struct ship *ship, long a);
static void recv_gunl(struct ship *ship, long a, long b);
static void recv_gunr(struct ship *ship, long a, long b);
static void recv_hull(struct ship *ship, long a);
static void recv_move(struct ship *ship, const char *astr);
static void recv_obp(struct ship *ship, long a, long b, long c, long d);
static void recv_pcrew(struct ship *ship, long a);
static void recv_unfoul(struct ship *ship, long a, long b);
static void recv_points(struct ship *ship, long a);
static void recv_qual(struct ship *ship, long a);
static void recv_ungrap(struct ship *ship, long a, long b);
static void recv_rigg(struct ship *ship, long a, long b, long c, long d);
static void recv_col(struct ship *ship, long a);
static void recv_dir(struct ship *ship, long a);
static void recv_row(struct ship *ship, long a);
static void recv_signal(struct ship *ship, const char *astr);
static void recv_sink(struct ship *ship, long a);
static void recv_struck(struct ship *ship, long a);
static void recv_ta(struct ship *ship, long a);
static void recv_alive(void);
static void recv_turn(long a);
static void recv_wind(long a, long b);
static void recv_fs(struct ship *ship, long a);
static void recv_grap(struct ship *ship, long a);
static void recv_rig1(struct ship *ship, long a);
static void recv_rig2(struct ship *ship, long a);
static void recv_rig3(struct ship *ship, long a);
static void recv_rig4(struct ship *ship, long a);
static void recv_begin(struct ship *ship);
static void recv_end(struct ship *ship);
static void recv_ddead(void);

static void Write(int, struct ship *, long, long, long, long);
static void Writestr(int, struct ship *, const char *);

static int sync_update(int, struct ship *, const char *,
		       long, long, long, long);

static char sync_buf[BUFSIZE];
static char *sync_bp = sync_buf;
static long sync_seek;
static FILE *sync_fp;

static const char *
get_sync_file(int scenario_number)
{
	static char sync_file[NAME_MAX];

	snprintf(sync_file, sizeof(sync_file), _FILE_SYNC, scenario_number);
	return sync_file;
}

static const char *
get_lock_file(int scenario_number)
{
	static char sync_lock[NAME_MAX];

	snprintf(sync_lock, sizeof(sync_lock), _FILE_LOCK, scenario_number);
	return sync_lock;
}

void
fmtship(char *buf, size_t len, const char *fmt, struct ship *ship)
{
	while (*fmt) {
		if (len-- == 0) {
			*buf = '\0';
			return;
		}
		if (*fmt == '$' && fmt[1] == '$') {
			size_t l = snprintf(buf, len, "%s (%c%c)",
			    ship->shipname, colours(ship), sterncolour(ship));
			buf += l;
			len -= l - 1;
			fmt += 2;
		}
		else
			*buf++ = *fmt++;
	}

	if (len > 0)
		*buf = '\0';
}


/*VARARGS3*/
void
makesignal(struct ship *from, const char *fmt, struct ship *ship, ...)
{
	char message[BUFSIZ];
	char format[BUFSIZ];
	va_list ap;

	va_start(ap, ship);
	fmtship(format, sizeof(format), fmt, ship);
	vsnprintf(message, sizeof(message), format, ap);
	va_end(ap);
	send_signal(from, message);
}

/*VARARGS2*/
void
makemsg(struct ship *from, const char *fmt, ...)
{
	char message[BUFSIZ];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(message, sizeof(message), fmt, ap);
	va_end(ap);
	send_signal(from, message);
}

int
sync_exists(int gamenum)
{
	const char *path;
	struct stat s;
	time_t t;

	path = get_sync_file(gamenum);
	time(&t);
	setegid(egid);
	if (stat(path, &s) < 0) {
		setegid(gid);
		return 0;
	}
	if (s.st_mtime < t - 60*60*2) {		/* 2 hours */
		unlink(path);
		path = get_lock_file(gamenum);
		unlink(path);
		setegid(gid);
		return 0;
	} else {
		setegid(gid);
		return 1;
	}
}

int
sync_open(void)
{
	const char *sync_file;
	const char *sync_lock;
	struct stat tmp;

	if (sync_fp != NULL)
		fclose(sync_fp);
	sync_file = get_sync_file(game);
	sync_lock = get_lock_file(game);
	setegid(egid);
	if (stat(sync_file, &tmp) < 0) {
		mode_t omask = umask(002);
		sync_fp = fopen(sync_file, "w+");
		umask(omask);
	} else
		sync_fp = fopen(sync_file, "r+");
	setegid(gid);
	if (sync_fp == NULL)
		return -1;
	sync_seek = 0;
	return 0;
}

void
sync_close(int doremove)
{
	const char *sync_file;

	if (sync_fp != 0)
		fclose(sync_fp);
	if (doremove) {
		sync_file = get_sync_file(game);
		setegid(egid);
		unlink(sync_file);
		setegid(gid);
	}
}

static void
Write(int type, struct ship *ship, long a, long b, long c, long d)
{
	size_t max = sizeof(sync_buf) - (sync_bp - sync_buf);
	int shipindex = (ship == NULL) ? 0 : ship->file->index;

	snprintf(sync_bp, max, "%d %d 0 %ld %ld %ld %ld\n",
		       type, shipindex, a, b, c, d);
	while (*sync_bp++)
		;
	sync_bp--;
	if (sync_bp >= &sync_buf[sizeof sync_buf])
		abort();
	sync_update(type, ship, NULL, a, b, c, d);
}

static void
Writestr(int type, struct ship *ship, const char *a)
{
	size_t max = sizeof(sync_buf) - (sync_bp - sync_buf);
	int shipindex = (ship == NULL) ? 0 : ship->file->index;

	snprintf(sync_bp, max, "%d %d 1 %s\n", type, shipindex, a);
	while (*sync_bp++)
		;
	sync_bp--;
	if (sync_bp >= &sync_buf[sizeof sync_buf])
		abort();
	sync_update(type, ship, a, 0, 0, 0, 0);
}

int
Sync(void)
{
	sig_t sighup, sigint;
	int n;
	int type, shipnum, isstr;
	char *astr;
	long a, b, c, d;
	char buf[80];
	char erred = 0;
#ifndef LOCK_EX
	const char *sync_file;
	const char *sync_lock;
#endif

	sighup = signal(SIGHUP, SIG_IGN);
	sigint = signal(SIGINT, SIG_IGN);
	for (n = TIMEOUT; --n >= 0;) {
#ifdef LOCK_EX
		if (flock(fileno(sync_fp), LOCK_EX|LOCK_NB) >= 0)
			break;
		if (errno != EWOULDBLOCK)
			return -1;
#else
		sync_file = get_sync_file(game);
		sync_lock = get_lock_file(game);
		setegid(egid);
		if (link(sync_file, sync_lock) >= 0) {
			setegid(gid);
			break;
		}
		setegid(gid);
		if (errno != EEXIST)
			return -1;
#endif
		sleep(1);
	}
	if (n <= 0)
		return -1;
	fseek(sync_fp, sync_seek, SEEK_SET);
	for (;;) {
		switch (fscanf(sync_fp, "%d%d%d", &type, &shipnum, &isstr)) {
		case 3:
			break;
		case EOF:
			goto out;
		default:
			goto bad;
		}
		if (shipnum < 0 || shipnum >= cc->vessels)
			goto bad;
		if (isstr != 0 && isstr != 1)
			goto bad;
		if (isstr) {
			int ch;
			char *p;

			for (p = buf;;) {
				ch = getc(sync_fp);
				*p++ = ch;
				switch (ch) {
				case '\n':
					p--;
				case EOF:
					break;
				default:
					if (p >= buf + sizeof buf)
						p--;
					continue;
				}
				break;
			}
			*p = 0;
			for (p = buf; *p == ' '; p++)
				;
			astr = p;
			a = b = c = d = 0;
		} else {
			if (fscanf(sync_fp, "%ld%ld%ld%ld", &a, &b, &c, &d)
			    != 4)
				goto bad;
			astr = NULL;
		}
		if (sync_update(type, SHIP(shipnum), astr, a, b, c, d) < 0)
			goto bad;
	}
bad:
	erred++;
out:
	if (!erred && sync_bp != sync_buf) {
		fseek(sync_fp, 0L, SEEK_END);
		fwrite(sync_buf, sizeof *sync_buf, sync_bp - sync_buf,
			sync_fp);
		fflush(sync_fp);
		sync_bp = sync_buf;
	}
	sync_seek = ftell(sync_fp);
#ifdef LOCK_EX
	flock(fileno(sync_fp), LOCK_UN);
#else
	setegid(egid);
	unlink(sync_lock);
	setegid(gid);
#endif
	signal(SIGHUP, sighup);
	signal(SIGINT, sigint);
	return erred ? -1 : 0;
}

static int
sync_update(int type, struct ship *ship, const char *astr,
	    long a, long b, long c, long d)
{
	switch (type) {
	case W_CAPTAIN:  recv_captain(ship, astr);    break;
	case W_CAPTURED: recv_captured(ship, a);      break;
	case W_CLASS:    recv_class(ship, a);         break;
	case W_CREW:     recv_crew(ship, a, b, c);    break;
	case W_DBP:      recv_dbp(ship, a, b, c, d);  break;
	case W_DRIFT:    recv_drift(ship, a);         break;
	case W_EXPLODE:  recv_explode(ship, a);       break;
	case W_FOUL:     recv_foul(ship, a);          break;
	case W_GUNL:     recv_gunl(ship, a, b);       break;
	case W_GUNR:     recv_gunr(ship, a, b);       break;
	case W_HULL:     recv_hull(ship, a);          break;
	case W_MOVE:     recv_move(ship, astr);       break;
	case W_OBP:      recv_obp(ship, a, b, c, d);  break;
	case W_PCREW:    recv_pcrew(ship, a);         break;
	case W_UNFOUL:   recv_unfoul(ship, a, b);     break;
	case W_POINTS:   recv_points(ship, a);        break;
	case W_QUAL:     recv_qual(ship, a);          break;
	case W_UNGRAP:   recv_ungrap(ship, a, b);     break;
	case W_RIGG:     recv_rigg(ship, a, b, c, d); break;
	case W_COL:      recv_col(ship, a);           break;
	case W_DIR:      recv_dir(ship, a);           break;
	case W_ROW:      recv_row(ship, a);           break;
	case W_SIGNAL:   recv_signal(ship, astr);     break;
	case W_SINK:     recv_sink(ship, a);          break;
	case W_STRUCK:   recv_struck(ship, a);        break;
	case W_TA:       recv_ta(ship, a);            break;
	case W_ALIVE:    recv_alive();                break;
	case W_TURN:     recv_turn(a);                break;
	case W_WIND:     recv_wind(a, b);             break;
	case W_FS:       recv_fs(ship, a);            break;
	case W_GRAP:     recv_grap(ship, a);          break;
	case W_RIG1:     recv_rig1(ship, a);          break;
	case W_RIG2:     recv_rig2(ship, a);          break;
	case W_RIG3:     recv_rig3(ship, a);          break;
	case W_RIG4:     recv_rig4(ship, a);          break;
	case W_BEGIN:    recv_begin(ship);            break;
	case W_END:      recv_end(ship);              break;
	case W_DDEAD:    recv_ddead();                break;
	default:
		fprintf(stderr, "sync_update: unknown type %d\r\n", type);
		return -1;
	}
	return 0;
}

/*
 * Messages to send
 */

void
send_captain(struct ship *ship, const char *astr)
{
	Writestr(W_CAPTAIN, ship, astr);
}

void
send_captured(struct ship *ship, long a)
{
	Write(W_CAPTURED, ship, a, 0, 0, 0);
}

void
send_class(struct ship *ship, long a)
{
	Write(W_CLASS, ship, a, 0, 0, 0);
}

void
send_crew(struct ship *ship, long a, long b, long c)
{
	Write(W_CREW, ship, a, b, c, 0);
}

void
send_dbp(struct ship *ship, long a, long b, long c, long d)
{
	Write(W_DBP, ship, a, b, c, d);
}

void
send_drift(struct ship *ship, long a)
{
	Write(W_DRIFT, ship, a, 0, 0, 0);
}

void
send_explode(struct ship *ship, long a)
{
	Write(W_EXPLODE, ship, a, 0, 0, 0);
}

void
send_foul(struct ship *ship, long a)
{
	Write(W_FOUL, ship, a, 0, 0, 0);
}

void
send_gunl(struct ship *ship, long a, long b)
{
	Write(W_GUNL, ship, a, b, 0, 0);
}

void
send_gunr(struct ship *ship, long a, long b)
{
	Write(W_GUNR, ship, a, b, 0, 0);
}

void
send_hull(struct ship *ship, long a)
{
	Write(W_HULL, ship, a, 0, 0, 0);
}

void
send_move(struct ship *ship, const char *astr)
{
	Writestr(W_MOVE, ship, astr);
}

void
send_obp(struct ship *ship, long a, long b, long c, long d)
{
	Write(W_OBP, ship, a, b, c, d);
}

void
send_pcrew(struct ship *ship, long a)
{
	Write(W_PCREW, ship, a, 0, 0, 0);
}

void
send_unfoul(struct ship *ship, long a, long b)
{
	Write(W_UNFOUL, ship, a, b, 0, 0);
}

void
send_points(struct ship *ship, long a)
{
	Write(W_POINTS, ship, a, 0, 0, 0);
}

void
send_qual(struct ship *ship, long a)
{
	Write(W_QUAL, ship, a, 0, 0, 0);
}

void
send_ungrap(struct ship *ship, long a, long b)
{
	Write(W_UNGRAP, ship, a, b, 0, 0);
}

void
send_rigg(struct ship *ship, long a, long b, long c, long d)
{
	Write(W_RIGG, ship, a, b, c, d);
}

void
send_col(struct ship *ship, long a)
{
	Write(W_COL, ship, a, 0, 0, 0);
}

void
send_dir(struct ship *ship, long a)
{
	Write(W_DIR, ship, a, 0, 0, 0);
}

void
send_row(struct ship *ship, long a)
{
	Write(W_ROW, ship, a, 0, 0, 0);
}

void
send_signal(struct ship *ship, const char *astr)
{
	Writestr(W_SIGNAL, ship, astr);
}

void
send_sink(struct ship *ship, long a)
{
	Write(W_SINK, ship, a, 0, 0, 0);
}

void
send_struck(struct ship *ship, long a)
{
	Write(W_STRUCK, ship, a, 0, 0, 0);
}

void
send_ta(struct ship *ship, long a)
{
	Write(W_TA, ship, a, 0, 0, 0);
}

void
send_alive(void)
{
	Write(W_ALIVE, NULL, 0, 0, 0, 0);
}

void
send_turn(long a)
{
	Write(W_TURN, NULL, a, 0, 0, 0);
}

void
send_wind(long a, long b)
{
	Write(W_WIND, NULL, a, b, 0, 0);
}

void
send_fs(struct ship *ship, long a)
{
	Write(W_FS, ship, a, 0, 0, 0);
}

void
send_grap(struct ship *ship, long a)
{
	Write(W_GRAP, ship, a, 0, 0, 0);
}

void
send_rig1(struct ship *ship, long a)
{
	Write(W_RIG1, ship, a, 0, 0, 0);
}

void
send_rig2(struct ship *ship, long a)
{
	Write(W_RIG2, ship, a, 0, 0, 0);
}

void
send_rig3(struct ship *ship, long a)
{
	Write(W_RIG3, ship, a, 0, 0, 0);
}

void
send_rig4(struct ship *ship, long a)
{
	Write(W_RIG4, ship, a, 0, 0, 0);
}

void
send_begin(struct ship *ship)
{
	Write(W_BEGIN, ship, 0, 0, 0, 0);
}

void
send_end(struct ship *ship)
{
	Write(W_END, ship, 0, 0, 0, 0);
}

void
send_ddead(void)
{
	Write(W_DDEAD, NULL, 0, 0, 0, 0);
}


/*
 * Actions upon message receipt
 */

static void
recv_captain(struct ship *ship, const char *astr)
{
	strlcpy(ship->file->captain, astr, sizeof ship->file->captain);
}

static void
recv_captured(struct ship *ship, long a)
{
	if (a < 0)
		ship->file->captured = 0;
	else
		ship->file->captured = SHIP(a);
}

static void
recv_class(struct ship *ship, long a)
{
	ship->specs->class = a;
}

static void
recv_crew(struct ship *ship, long a, long b, long c)
{
	struct shipspecs *s = ship->specs;

	s->crew1 = a;
	s->crew2 = b;
	s->crew3 = c;
}

static void
recv_dbp(struct ship *ship, long a, long b, long c, long d)
{
	struct BP *p = &ship->file->DBP[a];

	p->turnsent = b;
	p->toship = SHIP(c);
	p->mensent = d;
}

static void
recv_drift(struct ship *ship, long a)
{
	ship->file->drift = a;
}

static void
recv_explode(struct ship *ship, long a)
{
	if ((ship->file->explode = a) == 2)
		ship->file->dir = 0;
}

static void
recv_foul(struct ship *ship, long a)
{
	struct snag *p = &ship->file->foul[a];

	if (SHIP(a)->file->dir == 0)
		return;
	if (p->sn_count++ == 0)
		p->sn_turn = turn;
	ship->file->nfoul++;
}

static void
recv_gunl(struct ship *ship, long a, long b)
{
	struct shipspecs *s = ship->specs;

	s->gunL = a;
	s->carL = b;
}

static void
recv_gunr(struct ship *ship, long a, long b)
{
	struct shipspecs *s = ship->specs;

	s->gunR = a;
	s->carR = b;
}

static void
recv_hull(struct ship *ship, long a)
{
	ship->specs->hull = a;
}

static void
recv_move(struct ship *ship, const char *astr)
{
	strlcpy(ship->file->movebuf, astr, sizeof ship->file->movebuf);
}

static void
recv_obp(struct ship *ship, long a, long b, long c, long d)
{
	struct BP *p = &ship->file->OBP[a];

	p->turnsent = b;
	p->toship = SHIP(c);
	p->mensent = d;
}

static void
recv_pcrew(struct ship *ship, long a)
{
	ship->file->pcrew = a;
}

static void
recv_unfoul(struct ship *ship, long a, long b)
{
	struct snag *p = &ship->file->foul[a];

	if (p->sn_count > 0) {
		if (b) {
			ship->file->nfoul -= p->sn_count;
			p->sn_count = 0;
		} else {
			ship->file->nfoul--;
			p->sn_count--;
		}
	}
}

static void
recv_points(struct ship *ship, long a)
{
	ship->file->points = a;
}

static void
recv_qual(struct ship *ship, long a)
{
	ship->specs->qual = a;
}

static void
recv_ungrap(struct ship *ship, long a, long b)
{
	struct snag *p = &ship->file->grap[a];

	if (p->sn_count > 0) {
		if (b) {
			ship->file->ngrap -= p->sn_count;
			p->sn_count = 0;
		} else {
			ship->file->ngrap--;
			p->sn_count--;
		}
	}
}

static void
recv_rigg(struct ship *ship, long a, long b, long c, long d)
{
	struct shipspecs *s = ship->specs;

	s->rig1 = a;
	s->rig2 = b;
	s->rig3 = c;
	s->rig4 = d;
}

static void
recv_col(struct ship *ship, long a)
{
	ship->file->col = a;
}

static void
recv_dir(struct ship *ship, long a)
{
	ship->file->dir = a;
}

static void
recv_row(struct ship *ship, long a)
{
	ship->file->row = a;
}

static void
recv_signal(struct ship *ship, const char *astr)
{
	if (mode == MODE_PLAYER) {
		if (nobells)
			Signal("$$: %s", ship, astr);
		else
			Signal("\a$$: %s", ship, astr);
	}
}

static void
recv_sink(struct ship *ship, long a)
{
	if ((ship->file->sink = a) == 2)
		ship->file->dir = 0;
}

static void
recv_struck(struct ship *ship, long a)
{
	ship->file->struck = a;
}

static void
recv_ta(struct ship *ship, long a)
{
	ship->specs->ta = a;
}

static void
recv_alive(void)
{
	alive = 1;
}

static void
recv_turn(long a)
{
	turn = a;
}

static void
recv_wind(long a, long b)
{
	winddir = a;
	windspeed = b;
}

static void
recv_fs(struct ship *ship, long a)
{
	ship->file->FS = a;
}

static void
recv_grap(struct ship *ship, long a)
{
	struct snag *p = &ship->file->grap[a];

	if (SHIP(a)->file->dir == 0)
		return;
	if (p->sn_count++ == 0)
		p->sn_turn = turn;
	ship->file->ngrap++;
}

static void
recv_rig1(struct ship *ship, long a)
{
	ship->specs->rig1 = a;
}

static void
recv_rig2(struct ship *ship, long a)
{
	ship->specs->rig2 = a;
}

static void
recv_rig3(struct ship *ship, long a)
{
	ship->specs->rig3 = a;
}

static void
recv_rig4(struct ship *ship, long a)
{
	ship->specs->rig4 = a;
}

static void
recv_begin(struct ship *ship)
{
	strcpy(ship->file->captain, "begin");
	people++;
}

static void
recv_end(struct ship *ship)
{
	*ship->file->captain = 0;
	ship->file->points = 0;
	people--;
}

static void
recv_ddead(void)
{
	hasdriver = 0;
}
