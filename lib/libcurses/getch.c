/*	$NetBSD: getch.c,v 1.11 1999/06/06 20:43:00 pk Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)getch.c	8.2 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: getch.c,v 1.11 1999/06/06 20:43:00 pk Exp $");
#endif
#endif					/* not lint */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "curses.h"

#define DEFAULT_DELAY 2			/* default delay for timeout() */

/*
 * Keyboard input handler.  Do this by snarfing
 * all the info we can out of the termcap entry for TERM and putting it
 * into a set of keymaps.  A keymap is an array the size of all the possible
 * single characters we can get, the contents of the array is a structure
 * that contains the type of entry this character is (i.e. part/end of a
 * multi-char sequence or a plain char) and either a pointer which will point
 * to another keymap (in the case of a multi-char sequence) OR the data value
 * that this key should return.
 *
 */

/* private data structures for holding the key definitions */
typedef struct keymap keymap_t;
typedef struct key_entry key_entry_t;

struct key_entry {
	short   type;		/* type of key this is */
	union {
		keymap_t *next;	/* next keymap is key is multi-key sequence */
		int     symbol;	/* key symbol if key is a leaf entry */
	}       value;
};
/* Types of key structures we can have */
#define KEYMAP_MULTI  1		/* part of a multi char sequence */
#define KEYMAP_LEAF   2		/* key has a symbol associated with it, either
				 * it is the end of a multi-char sequence or a
				 * single char key that generates a symbol */

/* The max number of different chars we can receive */
#define MAX_CHAR 256

struct keymap {
	int     count;		/* count of number of key structs allocated */
	short   mapping[MAX_CHAR];	/* mapping of key to allocated structs */
	key_entry_t **key;	/* dynamic array of keys */};


/* Key buffer */
#define INBUF_SZ 16		/* size of key buffer - must be larger than
				 * longest multi-key sequence */
char    inbuf[INBUF_SZ];
int     start, end, working;	/* pointers for manipulating inbuf data */

#define INC_POINTER(ptr)  do {                                            \
        (ptr)++;                                                          \
        ptr %= INBUF_SZ;                                                  \
} while(/*CONSTCOND*/0)

short   state;			/* state of the inkey function */

#define INKEY_NORM       0	/* no key backlog to process */
#define INKEY_ASSEMBLING 1	/* assembling a multi-key sequence */
#define INKEY_BACKOUT    2	/* recovering from an unrecognised key */
#define INKEY_TIMEOUT    3	/* multi-key sequence timeout */

/* The termcap data we are interested in and the symbols they map to */
struct tcdata {
	char   *name;		/* name of termcap entry */
	int     symbol;		/* the symbol associated with it */
};

const struct tcdata tc[] = {
	{"K1", KEY_A1},
	{"K2", KEY_B2},
	{"K3", KEY_A3},
	{"K4", KEY_C1},
	{"K5", KEY_C3},
	{"k0", KEY_F0},
	{"k1", KEY_F(1)},
	{"k2", KEY_F(2)},
	{"k3", KEY_F(3)},
	{"k4", KEY_F(4)},
	{"k5", KEY_F(5)},
	{"k6", KEY_F(6)},
	{"k7", KEY_F(7)},
	{"k8", KEY_F(8)},
	{"k9", KEY_F(9)},
	{"kA", KEY_IL},
	{"ka", KEY_CATAB},
	{"kb", KEY_BACKSPACE},
	{"kC", KEY_CLEAR},
	{"kD", KEY_DC},
	{"kd", KEY_DOWN},
	{"kE", KEY_EOL},
	{"kF", KEY_SF},
	{"kH", KEY_LL},
	{"kh", KEY_HOME},
	{"kI", KEY_IC},
	{"kL", KEY_DL},
	{"kl", KEY_LEFT},
	{"kN", KEY_NPAGE},
	{"kP", KEY_PPAGE},
	{"kR", KEY_SR},
	{"kr", KEY_RIGHT},
	{"kS", KEY_EOS},
	{"kT", KEY_STAB},
	{"kt", KEY_CTAB},
	{"ku", KEY_UP}
};
/* Number of TC entries .... */
const int num_tcs = (sizeof(tc) / sizeof(struct tcdata));

/* The root keymap */

keymap_t *base_keymap;

/* prototypes for private functions */
keymap_t *
new_keymap(void);	/* create a new keymap */

key_entry_t *
new_key(void);		/* create a new key entry */

unsigned
inkey(int, int);

/*
 * Init_getch - initialise all the pointers & structures needed to make
 * getch work in keypad mode.
 *
 */
void
__init_getch(sp)
	char   *sp;
{
	int     i, j, length;
	keymap_t *current;
static	char    termcap[1024];
	char    entry[1024], termname[1024], *p;
	key_entry_t *the_key;

	/* init the inkey state variable */
	state = INKEY_NORM;

	/* init the base keymap */
	base_keymap = new_keymap();

	/* key input buffer pointers */
	start = end = working = 0;

	/* now do the termcap snarfing ... */
	strncpy(termname, sp, 1022);
	termname[1023] = 0;

	if (tgetent(termcap, termname) > 0) {
		for (i = 0; i < num_tcs; i++) {
			p = entry;
			if (tgetstr(tc[i].name, &p) != NULL) {
				current = base_keymap;	/* always start with
							 * base keymap. */
				length = strlen(entry);

				for (j = 0; j < length - 1; j++) {
					if (current->mapping[(unsigned) entry[j]] < 0) {
						/* first time for this char */
						current->mapping[(unsigned) entry[j]] = current->count;	/* map new entry */
						the_key = new_key();
						/* multikey coz we are here */
						the_key->type = KEYMAP_MULTI;

						/* need for next key */
						the_key->value.next
							= new_keymap();
						
						/* put into key array */
						if ((current->key = realloc(current->key, (current->count + 1) * sizeof(key_entry_t *))) == NULL) {
							fprintf(stderr,
								"Could not malloc for key entry\n");
							exit(1);
						}
						
						current->key[current->count++]
							= the_key;

					}
					/* next key uses this map... */
					current = current->key[current->mapping[(unsigned) entry[j]]]->value.next;
				}

				/* this is the last key in the sequence (it
				 * may have been the only one but that does
				 * not matter) this means it is a leaf key and
				 * should have a symbol associated with it */
				if (current->count > 0) {
					  /* if there were other keys then
					     we need to extend the mapping
					     array */
					if ((current->key =
					     realloc(current->key,
						     (current->count + 1) *
						     sizeof(key_entry_t *)))
					    == NULL) {
						fprintf(stderr,
							"Could not malloc for key entry\n");
						exit(1);
					}
				}
				current->mapping[(unsigned) entry[length - 1]]
					= current->count;
				the_key = new_key();
				the_key->type = KEYMAP_LEAF;	/* leaf key */

				/* the associated symbol */
				the_key->value.symbol = tc[i].symbol;
				current->key[current->count++] = the_key;
			}
		}
	}
}


/*
 * new_keymap - allocates & initialises a new keymap structure.  This
 * function returns a pointer to the new keymap.
 *
 */
keymap_t *
new_keymap(void)
{
	int     i;
	keymap_t *new_map;

	if ((new_map = malloc(sizeof(keymap_t))) == NULL) {
		perror("Inkey: Cannot allocate new keymap");
		exit(2);
	}
	/* initialise the new map */
	new_map->count = 0;
	for (i = 0; i < MAX_CHAR; i++) {
		new_map->mapping[i] = -1;	/* no mapping for char */
	}

	  /* one does assume there will be at least one key mapped.... */
	if ((new_map->key = malloc(sizeof(key_entry_t *))) == NULL) {
		perror("Could not malloc first key ent");
		exit(1);
	}
							
	return new_map;
}

/*
 * new_key - allocates & initialises a new key entry.  This function returns
 * a pointer to the newly allocated key entry.
 *
 */
key_entry_t *
new_key(void)
{
	key_entry_t *new_one;

	if ((new_one = malloc(sizeof(key_entry_t))) == NULL) {
		perror("inkey: Cannot allocate new key entry");
		exit(2);
	}
	new_one->type = 0;
	new_one->value.next = NULL;

	return new_one;
}

/*
 * inkey - do the work to process keyboard input, check for multi-key
 * sequences and return the appropriate symbol if we get a match.
 *
 */

unsigned
inkey(to, delay)
	int     to, delay;
{
	int     k, nchar;
	char    c;
	keymap_t *current = base_keymap;

	for (;;) {		/* loop until we get a complete key sequence */
reread:
		if (state == INKEY_NORM) {
			if (delay && __timeout(delay) == ERR)
				return ERR;
			if ((nchar = read(STDIN_FILENO, &c, sizeof(char))) < 0)
				return ERR;
			if (delay && (__notimeout() == ERR))
				return ERR;
			if (nchar == 0)
				return ERR;	/* just in case we are nodelay
						 * mode */
			k = (unsigned int) c;
#ifdef DEBUG
			__CTRACE("inkey (state normal) got '%s'\n", unctrl(k));
#endif

			working = start;
			inbuf[working] = k;
			INC_POINTER(working);
			end = working;
			state = INKEY_ASSEMBLING;	/* go to the assembling
							 * state now */
		} else
			if (state == INKEY_BACKOUT) {
				k = inbuf[working];
				INC_POINTER(working);
				if (working == end) {	/* see if we have run
							 * out of keys in the
							 * backlog */

					/* if we have then switch to
					   assembling */
					state = INKEY_ASSEMBLING;
				}
			} else if (state == INKEY_ASSEMBLING) {
				/* assembling a key sequence */
				if (delay)
				{
					if (__timeout(to ? DEFAULT_DELAY : delay) == ERR)
						return ERR;
				} else {
					if (to && (__timeout(DEFAULT_DELAY) == ERR))
						return ERR;
				}
				if ((nchar = read(STDIN_FILENO, &c,
						  sizeof(char))) < 0)
					return ERR;
				if ((to || delay) && (__notimeout() == ERR))
					return ERR;
				
				k = (unsigned int) c;
#ifdef DEBUG
				__CTRACE("inkey (state assembling) got '%s'\n", unctrl(k));
#endif
				if (nchar == 0) {	/* inter-char timeout,
							 * start backing out */
					if (start == end)
						goto reread; /* no chars in the
							      * buffer, restart */
					k = inbuf[start];
					state = INKEY_TIMEOUT;
				} else {
					inbuf[working] = k;
					INC_POINTER(working);
					end = working;
				}
			} else {
				fprintf(stderr,
					"Inkey state screwed - exiting!!!");
				exit(2);
			}

		/* Check key has no special meaning and we have not timed out */
		if ((current->mapping[k] < 0) || (state == INKEY_TIMEOUT)) {
			k = inbuf[start];	/* return the first key we
						 * know about */

			INC_POINTER(start);
			working = start;

			if (start == end) {	/* only one char processed */
				state = INKEY_NORM;
			} else {/* otherwise we must have more than one char
				 * to backout */
				state = INKEY_BACKOUT;
			}
			return k;
		} else {	/* must be part of a multikey sequence */
			/* check for completed key sequence */
			if (current->key[current->mapping[k]]->type == KEYMAP_LEAF) {
				start = working;	/* eat the key sequence
							 * in inbuf */

				if (start == end) {	/* check if inbuf empty
							 * now */
					state = INKEY_NORM; /* if it is go
							       back to normal */
				} else {	/* otherwise go to backout
						 * state */
					state = INKEY_BACKOUT;
				}

				/* return the symbol */
				return current->key[current->mapping[k]]->value.symbol;

			} else {/* step on to next part of the multi-key
				 * sequence */
				current = current->key[current->mapping[k]]->value.next;
			}
		}
	}
}

/*
 * wgetch --
 *	Read in a character from the window.
 */
int
wgetch(win)
	WINDOW *win;
{
	int     inp, weset;
	int	nchar;
	char    c;

	if (!(win->flags & __SCROLLOK) && (win->flags & __FULLWIN)
	    && win->curx == win->maxx - 1 && win->cury == win->maxy - 1
	    && __echoit)
		return (ERR);
#ifdef DEBUG
	__CTRACE("wgetch: __echoit = %d, __rawmode = %d\n",
	    __echoit, __rawmode);
#endif
	if (__echoit && !__rawmode) {
		cbreak();
		weset = 1;
	} else
		weset = 0;

	__save_termios();

	if (win->flags & __KEYPAD) {
		switch (win->delay)
		{
		case -1:
			inp = inkey (win->flags & __NOTIMEOUT ? 0 : 1, 0);
			break;
		case 0:
			if (__nodelay() == ERR) return ERR;
			inp = inkey(0, 0);
			break;
		default:
			inp = inkey(win->flags & __NOTIMEOUT ? 0 : 1, win->delay);
			break;
		}
	} else {
		switch (win->delay)
		{
		case -1:
			break;
		case 0:
			if (__nodelay() == ERR) {
				__restore_termios();
				return ERR;
			}
			break;
		default:
			if (__timeout(win->delay) == ERR) {
				__restore_termios();
				return ERR;
			}
			break;
		}
		if ((nchar = read(STDIN_FILENO, &c, sizeof(char))) < 0)
			inp = ERR;
		else {
			if (nchar == 0) {
				__restore_termios();
				return ERR;	/* we have timed out */
			}
			inp = (unsigned int) c;
		}
	}
#ifdef DEBUG
	__CTRACE("wgetch got '%s'\n", unctrl(inp));
#endif
	if (win->delay > -1)
		if (__delay() == ERR) {
			__restore_termios();
			return ERR;
		}
	__restore_termios();
	if (__echoit) {
		mvwaddch(curscr,
		    (int) (win->cury + win->begy), (int) (win->curx + win->begx), inp);
		waddch(win, inp);
	}
	if (weset)
		nocbreak();
	return ((inp < 0) || (inp == ERR) ? ERR : inp);
}
