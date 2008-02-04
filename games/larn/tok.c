/*	$NetBSD: tok.c,v 1.10 2008/02/04 01:07:01 dholland Exp $	*/

/* tok.c		Larn is copyrighted 1986 by Noah Morgan. */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: tok.c,v 1.10 2008/02/04 01:07:01 dholland Exp $");
#endif				/* not lint */

#include <sys/types.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include "header.h"
#include "extern.h"

/* Keystrokes (roughly) between checkpoints */
#define CHECKPOINT_INTERVAL	400

static char     lastok = 0;
int             yrepcount = 0;
#ifndef FLUSHNO
#define FLUSHNO 5
#endif	/* FLUSHNO */
static int      flushno = FLUSHNO;	/* input queue flushing threshold */
#define MAXUM 52		/* maximum number of user re-named monsters */
#define MAXMNAME 40		/* max length of a monster re-name */
static char     usermonster[MAXUM][MAXMNAME];	/* the user named monster
						 * name goes here */
static u_char     usermpoint = 0;	/* the user monster pointer */

/*
	lexical analyzer for larn
 */
int
yylex()
{
	char            cc;
	int             ic;
	if (hit2flag) {
		hit2flag = 0;
		yrepcount = 0;
		return (' ');
	}
	if (yrepcount > 0) {
		--yrepcount;
		return (lastok);
	} else
		yrepcount = 0;
	if (yrepcount == 0) {
		bottomdo();
		showplayer();
	}			/* show where the player is	 */
	lflush();
	while (1) {
		c[BYTESIN]++;
		/* check for periodic checkpointing */
		if (ckpflag)
			if ((c[BYTESIN] % CHECKPOINT_INTERVAL) == 0) {
#ifndef DOCHECKPOINTS
				savegame(ckpfile);
#else
				wait(0);	/* wait for other forks to
						 * finish */
				if (fork() == 0) {
					savegame(ckpfile);
					exit();
				}
#endif
			}
		do {		/* if keyboard input buffer is too big, flush
				 * some of it */
			ioctl(0, FIONREAD, &ic);
			if (ic > flushno)
				read(0, &cc, 1);
		}
		while (ic > flushno);

		if (read(0, &cc, 1) != 1)
			return (lastok = -1);

		if (cc == 'Y' - 64) {	/* control Y -- shell escape */
			resetscroll();
			clear();/* scrolling region, home, clear, no
				 * attributes */
			if ((ic = fork()) == 0) {	/* child */
				execl("/bin/csh", "/bin/csh", NULL);
				exit(1);
			}
			wait(0);
			if (ic < 0) {	/* error */
				write(2, "Can't fork off a shell!\n", 25);
				sleep(2);
			}
			setscroll();
			return (lastok = 'L' - 64);	/* redisplay screen */
		}
		if ((cc <= '9') && (cc >= '0')) {
			yrepcount = yrepcount * 10 + cc - '0';
		} else {
			if (yrepcount > 0)
				--yrepcount;
			return (lastok = cc);
		}
	}
}

/*
 *	flushall()		Function to flush all type-ahead in the input buffer
 */
void
flushall()
{
	char            cc;
	int             ic;
	for (;;) {		/* if keyboard input buffer is too big, flush
				 * some of it */
		ioctl(0, FIONREAD, &ic);
		if (ic <= 0)
			return;
		while (ic > 0) {
			read(0, &cc, 1);
			--ic;
		}		/* gobble up the byte */
	}
}

/*
	function to set the desired hardness
	enter with hard= -1 for default hardness, else any desired hardness
 */
void
sethard(hard)
	int             hard;
{
	int    j, k, i;
	struct monst *mp;

	j = c[HARDGAME];
	hashewon();
	if (restorflag == 0) {	/* don't set c[HARDGAME] if restoring game */
		if (hard >= 0)
			c[HARDGAME] = hard;
	} else
		c[HARDGAME] = j;/* set c[HARDGAME] to proper value if
				 * restoring game */

	if ((k = c[HARDGAME]) != 0)
		for (j = 0; j <= MAXMONST + 8; j++) {
			mp = &monster[j];
			i = ((6 + k) * mp->hitpoints + 1) / 6;
			mp->hitpoints = (i < 0) ? 32767 : i;
			i = ((6 + k) * mp->damage + 1) / 5;
			mp->damage = (i > 127) ? 127 : i;
			i = (10 * mp->gold) / (10 + k);
			mp->gold = (i > 32767) ? 32767 : i;
			i = mp->armorclass - k;
			mp->armorclass = (i < -127) ? -127 : i;
			i = (7 * mp->experience) / (7 + k) + 1;
			mp->experience = (i <= 0) ? 1 : i;
		}
}

/*
	function to read and process the larn options file
 */
void
readopts()
{
	const char  *i;
	int    j, k;
	int             flag;

	flag = 1;		/* set to 0 if a name is specified */

	if (lopen(optsfile) < 0) {
		strcpy(logname, loginname);
		return;		/* user name if no character name */
	}
	i = " ";
	while (*i) {
		if ((i = lgetw()) == NULL)
			break;	/* check for EOF */
		while ((*i == ' ') || (*i == '\t'))
			i++;	/* eat leading whitespace */

		if (strcmp(i, "bold-objects") == 0)
			boldon = 1;
		else if (strcmp(i, "enable-checkpointing") == 0)
			ckpflag = 1;
		else if (strcmp(i, "inverse-objects") == 0)
			boldon = 0;
		else if (strcmp(i, "female") == 0)
			sex = 0;	/* male or female */
		else if (strcmp(i, "monster:") == 0) {	/* name favorite monster */
			if ((i = lgetw()) == 0)
				break;
			strlcpy(usermonster[usermpoint], i, MAXMNAME);
			if (usermpoint >= MAXUM)
				continue;	/* defined all of em */
			if (isalpha(j = usermonster[usermpoint][0])) {
				for (k = 1; k < MAXMONST + 8; k++)	/* find monster */
					if (monstnamelist[k] == j) {
						monster[k].name = &usermonster[usermpoint++][0];
						break;
					}
			}
		} else if (strcmp(i, "male") == 0)
			sex = 1;
		else if (strcmp(i, "name:") == 0) {	/* defining players name */
			if ((i = lgetw()) == 0)
				break;
			strlcpy(logname, i, LOGNAMESIZE);
			flag = 0;
		} else if (strcmp(i, "no-introduction") == 0)
			nowelcome = 1;
		else if (strcmp(i, "no-beep") == 0)
			nobeep = 1;
		else if (strcmp(i, "process-name:") == 0) {
			if ((i = lgetw()) == 0)
				break;
			strlcpy(psname, i, PSNAMESIZE);
		} else if (strcmp(i, "play-day-play") == 0) {
			/* bypass time restrictions: ignored */
		} else if (strcmp(i, "savefile:") == 0) {	/* defining savefilename */
			if ((i = lgetw()) == 0)
				break;
			strcpy(savefilename, i);
			flag = 0;
		}
	}
	if (flag)
		strcpy(logname, loginname);
}
