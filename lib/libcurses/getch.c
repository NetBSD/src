/*	$NetBSD: getch.c,v 1.42 2003/08/07 16:44:21 agc Exp $	*/

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
static char sccsid[] = "@(#)getch.c	8.2 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: getch.c,v 1.42 2003/08/07 16:44:21 agc Exp $");
#endif
#endif					/* not lint */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "curses.h"
#include "curses_private.h"

int	ESCDELAY=300;		/* Delay in ms between keys for esc seq's */

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
typedef struct key_entry key_entry_t;

struct key_entry {
	short   type;		/* type of key this is */
	bool    enable;         /* true if the key is active */
	union {
		keymap_t *next;	/* next keymap is key is multi-key sequence */
		wchar_t   symbol;	/* key symbol if key is a leaf entry */
	} value;
};
/* Types of key structures we can have */
#define KEYMAP_MULTI  1		/* part of a multi char sequence */
#define KEYMAP_LEAF   2		/* key has a symbol associated with it, either
				 * it is the end of a multi-char sequence or a
				 * single char key that generates a symbol */

/* allocate this many key_entry structs at once to speed start up must
 * be a power of 2.
 */
#define KEYMAP_ALLOC_CHUNK 4

/* The max number of different chars we can receive */
#define MAX_CHAR 256

/*
 * Unused mapping flag.
 */
#define MAPPING_UNUSED (0 - MAX_CHAR) /* never been used */

struct keymap {
	int	count;	       /* count of number of key structs allocated */
	short	mapping[MAX_CHAR]; /* mapping of key to allocated structs */
	key_entry_t **key;     /* dynamic array of keys */
};


/* Key buffer */
#define INBUF_SZ 16		/* size of key buffer - must be larger than
				 * longest multi-key sequence */
static wchar_t  inbuf[INBUF_SZ];
static int     start, end, working; /* pointers for manipulating inbuf data */

#define INC_POINTER(ptr)  do {	\
	(ptr)++;		\
	ptr %= INBUF_SZ;	\
} while(/*CONSTCOND*/0)

static short	state;		/* state of the inkey function */

#define INKEY_NORM	 0	/* no key backlog to process */
#define INKEY_ASSEMBLING 1	/* assembling a multi-key sequence */
#define INKEY_BACKOUT	 2	/* recovering from an unrecognised key */
#define INKEY_TIMEOUT	 3	/* multi-key sequence timeout */

/* The termcap data we are interested in and the symbols they map to */
struct tcdata {
	const char	*name;	/* name of termcap entry */
	wchar_t	symbol;		/* the symbol associated with it */
};

static const struct tcdata tc[] = {
	{"!1", KEY_SSAVE},
	{"!2", KEY_SSUSPEND},
	{"!3", KEY_SUNDO},
	{"#1", KEY_SHELP},
	{"#2", KEY_SHOME},
	{"#3", KEY_SIC},
	{"#4", KEY_SLEFT},
	{"%0", KEY_REDO},
	{"%1", KEY_HELP},
	{"%2", KEY_MARK},
	{"%3", KEY_MESSAGE},
	{"%4", KEY_MOVE},
	{"%5", KEY_NEXT},
	{"%6", KEY_OPEN},
	{"%7", KEY_OPTIONS},
	{"%8", KEY_PREVIOUS},
	{"%9", KEY_PRINT},
	{"%a", KEY_SMESSAGE},
	{"%b", KEY_SMOVE},
	{"%c", KEY_SNEXT},
	{"%d", KEY_SOPTIONS},
	{"%e", KEY_SPREVIOUS},
	{"%f", KEY_SPRINT},
	{"%g", KEY_SREDO},
	{"%h", KEY_SREPLACE},
	{"%i", KEY_SRIGHT},
	{"%j", KEY_SRSUME},
	{"&0", KEY_SCANCEL},
	{"&1", KEY_REFERENCE},
	{"&2", KEY_REFRESH},
	{"&3", KEY_REPLACE},
	{"&4", KEY_RESTART},
	{"&5", KEY_RESUME},
	{"&6", KEY_SAVE},
	{"&7", KEY_SUSPEND},
	{"&8", KEY_UNDO},
	{"&9", KEY_SBEG},
	{"*0", KEY_SFIND},
	{"*1", KEY_SCOMMAND},
	{"*2", KEY_SCOPY},
	{"*3", KEY_SCREATE},
	{"*4", KEY_SDC},
	{"*5", KEY_SDL},
	{"*6", KEY_SELECT},
	{"*7", KEY_SEND},
	{"*8", KEY_SEOL},
	{"*9", KEY_SEXIT},
	{"@0", KEY_FIND},
	{"@1", KEY_BEG},
	{"@2", KEY_CANCEL},
	{"@3", KEY_CLOSE},
	{"@4", KEY_COMMAND},
	{"@5", KEY_COPY},
	{"@6", KEY_CREATE},
	{"@7", KEY_END},
	{"@8", KEY_ENTER},
	{"@9", KEY_EXIT},
	{"F1", KEY_F(11)},
	{"F2", KEY_F(12)},
	{"F3", KEY_F(13)},
	{"F4", KEY_F(14)},
	{"F5", KEY_F(15)},
	{"F6", KEY_F(16)},
	{"F7", KEY_F(17)},
	{"F8", KEY_F(18)},
	{"F9", KEY_F(19)},
	{"FA", KEY_F(20)},
	{"FB", KEY_F(21)},
	{"FC", KEY_F(22)},
	{"FD", KEY_F(23)},
	{"FE", KEY_F(24)},
	{"FF", KEY_F(25)},
	{"FG", KEY_F(26)},
	{"FH", KEY_F(27)},
	{"FI", KEY_F(28)},
	{"FJ", KEY_F(29)},
	{"FK", KEY_F(30)},
	{"FL", KEY_F(31)},
	{"FM", KEY_F(32)},
	{"FN", KEY_F(33)},
	{"FO", KEY_F(34)},
	{"FP", KEY_F(35)},
	{"FQ", KEY_F(36)},
	{"FR", KEY_F(37)},
	{"FS", KEY_F(38)},
	{"FT", KEY_F(39)},
	{"FU", KEY_F(40)},
	{"FV", KEY_F(41)},
	{"FW", KEY_F(42)},
	{"FX", KEY_F(43)},
	{"FY", KEY_F(44)},
	{"FZ", KEY_F(45)},
	{"Fa", KEY_F(46)},
	{"Fb", KEY_F(47)},
	{"Fc", KEY_F(48)},
	{"Fd", KEY_F(49)},
	{"Fe", KEY_F(50)},
	{"Ff", KEY_F(51)},
	{"Fg", KEY_F(52)},
	{"Fh", KEY_F(53)},
	{"Fi", KEY_F(54)},
	{"Fj", KEY_F(55)},
	{"Fk", KEY_F(56)},
	{"Fl", KEY_F(57)},
	{"Fm", KEY_F(58)},
	{"Fn", KEY_F(59)},
	{"Fo", KEY_F(60)},
	{"Fp", KEY_F(61)},
	{"Fq", KEY_F(62)},
	{"Fr", KEY_F(63)},
	{"K1", KEY_A1},
	{"K2", KEY_B2},
	{"K3", KEY_A3},
	{"K4", KEY_C1},
	{"K5", KEY_C3},
	{"Km", KEY_MOUSE},
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
	{"k;", KEY_F(10)},
	{"kA", KEY_IL},
	{"ka", KEY_CATAB},
	{"kB", KEY_BTAB},
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
	{"kM", KEY_EIC},
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
static const int num_tcs = (sizeof(tc) / sizeof(struct tcdata));

/* prototypes for private functions */
static void add_key_sequence(SCREEN *screen, char *sequence, int key_type);
static key_entry_t *add_new_key(keymap_t *current, char chr, int key_type,
				int symbol);
static void delete_key_sequence(keymap_t *current, int key_type);
static void do_keyok(keymap_t *current, int key_type, bool flag, int *retval);
static keymap_t		*new_keymap(void);	/* create a new keymap */
static key_entry_t	*new_key(void);		/* create a new key entry */
static wchar_t		inkey(int to, int delay);

/*
 * Free the storage associated with the given keymap
 */
void
_cursesi_free_keymap(keymap_t *map)
{
	int i;

	  /* check for, and free, child keymaps */
	for (i = 0; i < MAX_CHAR; i++) {
		if (map->mapping[i] >= 0) {
			if (map->key[map->mapping[i]]->type == KEYMAP_MULTI)
				_cursesi_free_keymap(
					map->key[map->mapping[i]]->value.next);
		}
	}

	  /* now free any allocated keymap structs */
	for (i = 0; i < map->count; i += KEYMAP_ALLOC_CHUNK) {
		free(map->key[i]);
	}

	free(map->key);
	free(map);
}

				
/*
 * Add a new key entry to the keymap pointed to by current.  Entry
 * contains the character to add to the keymap, type is the type of
 * entry to add (either multikey or leaf) and symbol is the symbolic
 * value for a leaf type entry.  The function returns a pointer to the
 * new keymap entry.
 */
static key_entry_t *
add_new_key(keymap_t *current, char chr, int key_type, int symbol)
{
	key_entry_t *the_key;
        int i, ki;

#ifdef DEBUG
	__CTRACE("Adding character %s of type %d, symbol 0x%x\n", unctrl(chr),
		 key_type, symbol);
#endif
	if (current->mapping[(unsigned char) chr] < 0) {
		if (current->mapping[(unsigned char) chr] == MAPPING_UNUSED) {
			  /* first time for this char */
			current->mapping[(unsigned char) chr] =
				current->count;	/* map new entry */
			ki = current->count;
			
			  /* make sure we have room in the key array first */
			if ((current->count & (KEYMAP_ALLOC_CHUNK - 1)) == 0)
			{
				if ((current->key =
				     realloc(current->key,
					     ki * sizeof(key_entry_t *)
					     + KEYMAP_ALLOC_CHUNK * sizeof(key_entry_t *))) == NULL) {
					fprintf(stderr,
					  "Could not malloc for key entry\n");
					exit(1);
				}
			
				the_key = new_key();
				for (i = 0; i < KEYMAP_ALLOC_CHUNK; i++) {
					current->key[ki + i] = &the_key[i];
				}
			}
                } else {
			  /* the mapping was used but freed, reuse it */
			ki = - current->mapping[(unsigned char) chr];
			current->mapping[(unsigned char) chr] = ki;
		}
		
		current->count++;
		
		  /* point at the current key array element to use */
		the_key = current->key[ki];
                                                
		the_key->type = key_type;

		switch (key_type) {
		  case KEYMAP_MULTI:
			    /* need for next key */
#ifdef DEBUG
			  __CTRACE("Creating new keymap\n");
#endif
			  the_key->value.next = new_keymap();
			  the_key->enable = TRUE;
			  break;

		  case KEYMAP_LEAF:
				/* the associated symbol for the key */
#ifdef DEBUG
			  __CTRACE("Adding leaf key\n");
#endif
			  the_key->value.symbol = symbol;
			  the_key->enable = TRUE;
			  break;

		  default:
			  fprintf(stderr, "add_new_key: bad type passed\n");
			  exit(1);
		}
	} else {
		  /* the key is already known - just return the address. */
#ifdef DEBUG
		__CTRACE("Keymap already known\n");
#endif
		the_key = current->key[current->mapping[(unsigned char) chr]];
	}

        return the_key;
}

/*
 * Delete the given key symbol from the key mappings for the screen.
 *
 */
void
delete_key_sequence(keymap_t *current, int key_type)
{
	key_entry_t *key;
	int i;

	  /*
	   * we need to iterate over all the keys as there may be
	   * multiple instances of the leaf symbol.
	   */
	for (i = 0; i < MAX_CHAR; i++) {
		if (current->mapping[i] < 0)
			continue; /* no mapping for the key, next! */

		key = current->key[current->mapping[i]];

		if (key->type == KEYMAP_MULTI) {
			  /* have not found the leaf, recurse down */
			delete_key_sequence(key->value.next, key_type);
			  /* if we deleted the last key in the map, free */
			if (key->value.next->count == 0)
				_cursesi_free_keymap(key->value.next);
		} else if ((key->type == KEYMAP_LEAF)
			   && (key->value.symbol == key_type)) {
			  /*
			   * delete the mapping by negating the current
			   * index - this "holds" the position in the
			   * allocation just in case we later re-add
			   * the key for that mapping.
			   */
			current->mapping[i] = - current->mapping[i];
			current->count--;
		}
	}
}
	
/*
 * Add the sequence of characters given in sequence as the key mapping
 * for the given key symbol.
 */
void
add_key_sequence(SCREEN *screen, char *sequence, int key_type)
{
	key_entry_t *tmp_key;
	keymap_t *current;
	int length, j, key_ent;

	current = screen->base_keymap;	/* always start with
					 * base keymap. */
	length = (int) strlen(sequence);

	for (j = 0; j < length - 1; j++) {
		  /* add the entry to the struct */
		tmp_key = add_new_key(current, sequence[j], KEYMAP_MULTI, 0);
					
		  /* index into the key array - it's
		     clearer if we stash this */
		key_ent = current->mapping[(unsigned char) sequence[j]];

		current->key[key_ent] = tmp_key;
				
		  /* next key uses this map... */
		current = current->key[key_ent]->value.next;
	}

	/*
	 * This is the last key in the sequence (it may have been the
	 * only one but that does not matter) this means it is a leaf
	 * key and should have a symbol associated with it.
	 */
	tmp_key = add_new_key(current, sequence[length - 1], KEYMAP_LEAF,
			      key_type);
	current->key[current->mapping[(int)sequence[length - 1]]] = tmp_key;
}

/*
 * Init_getch - initialise all the pointers & structures needed to make
 * getch work in keypad mode.
 *
 */
void
__init_getch(SCREEN *screen)
{
	char entry[1024], *p;
	int     i;
	size_t limit;
#ifdef DEBUG
	int k, length;
#endif

	/* init the inkey state variable */
	state = INKEY_NORM;

	/* init the base keymap */
	screen->base_keymap = new_keymap();

	/* key input buffer pointers */
	start = end = working = 0;

	/* now do the termcap snarfing ... */

	for (i = 0; i < num_tcs; i++) {
		p = entry;
		limit = 1023;
		if (t_getstr(screen->cursesi_genbuf, tc[i].name,
			     &p, &limit) != NULL) {
#ifdef DEBUG
			__CTRACE("Processing termcap entry %s, sequence ",
				 tc[i].name);
			length = (int) strlen(entry);
			for (k = 0; k <= length -1; k++)
				__CTRACE("%s", unctrl(entry[k]));
			__CTRACE("\n");
#endif
			add_key_sequence(screen, entry, tc[i].symbol);
		}
		
	}
}


/*
 * new_keymap - allocates & initialises a new keymap structure.  This
 * function returns a pointer to the new keymap.
 *
 */
static keymap_t *
new_keymap(void)
{
	int     i;
	keymap_t *new_map;

	if ((new_map = malloc(sizeof(keymap_t))) == NULL) {
		perror("Inkey: Cannot allocate new keymap");
		exit(2);
	}

	/* Initialise the new map */
	new_map->count = 0;
	for (i = 0; i < MAX_CHAR; i++) {
		new_map->mapping[i] = MAPPING_UNUSED; /* no mapping for char */
	}

	/* key array will be allocated when first key is added */
	new_map->key = NULL;

	return new_map;
}

/*
 * new_key - allocates & initialises a new key entry.  This function returns
 * a pointer to the newly allocated key entry.
 *
 */
static key_entry_t *
new_key(void)
{
	key_entry_t *new_one;
	int i;

	if ((new_one = malloc(KEYMAP_ALLOC_CHUNK * sizeof(key_entry_t)))
	    == NULL) {
		perror("inkey: Cannot allocate new key entry chunk");
		exit(2);
	}

	for (i = 0; i < KEYMAP_ALLOC_CHUNK; i++) {
		new_one[i].type = 0;
		new_one[i].value.next = NULL;
	}

	return new_one;
}

/*
 * inkey - do the work to process keyboard input, check for multi-key
 * sequences and return the appropriate symbol if we get a match.
 *
 */

wchar_t
inkey(int to, int delay)
{
	wchar_t		 k;
	int              c, mapping;
	keymap_t	*current = _cursesi_screen->base_keymap;
	FILE            *infd = _cursesi_screen->infd;

	k = 0;		/* XXX gcc -Wuninitialized */

	for (;;) {		/* loop until we get a complete key sequence */
reread:
		if (state == INKEY_NORM) {
			if (delay && __timeout(delay) == ERR)
				return ERR;
			if ((c = getchar()) == EOF) {
				clearerr(infd);
				return ERR;
			}
			
			if (delay && (__notimeout() == ERR))
				return ERR;

			k = (wchar_t) c;
#ifdef DEBUG
			__CTRACE("inkey (state normal) got '%s'\n", unctrl(k));
#endif

			working = start;
			inbuf[working] = k;
			INC_POINTER(working);
			end = working;
			state = INKEY_ASSEMBLING;	/* go to the assembling
							 * state now */
		} else if (state == INKEY_BACKOUT) {
			k = inbuf[working];
			INC_POINTER(working);
			if (working == end) {	/* see if we have run
						 * out of keys in the
						 * backlog */

				/* if we have then switch to assembling */
				state = INKEY_ASSEMBLING;
			}
		} else if (state == INKEY_ASSEMBLING) {
			/* assembling a key sequence */
			if (delay) {
				if (__timeout(to ? (ESCDELAY / 100) : delay)
				    == ERR)
					return ERR;
			} else {
				if (to && (__timeout(ESCDELAY / 100) == ERR))
					return ERR;
			}

			c = getchar();
			if (ferror(infd)) {
				clearerr(infd);
				return ERR;
			}
			
			if ((to || delay) && (__notimeout() == ERR))
					return ERR;

			k = (wchar_t) c;
#ifdef DEBUG
			__CTRACE("inkey (state assembling) got '%s'\n", unctrl(k));
#endif
			if (feof(infd)) {	/* inter-char timeout,
						 * start backing out */
				clearerr(infd);
				if (start == end)
					/* no chars in the buffer, restart */
					goto reread;

				k = inbuf[start];
				state = INKEY_TIMEOUT;
			} else {
				inbuf[working] = k;
				INC_POINTER(working);
				end = working;
			}
		} else {
			fprintf(stderr, "Inkey state screwed - exiting!!!");
			exit(2);
		}

		  /*
		   * Check key has no special meaning and we have not
		   * timed out and the key has not been disabled
		   */
		mapping = current->mapping[k];
		if (((state == INKEY_TIMEOUT) || (mapping < 0))
			|| ((current->key[mapping]->type == KEYMAP_LEAF)
			    && (current->key[mapping]->enable == FALSE))) {
			/* return the first key we know about */
			k = inbuf[start];

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

				/* check if inbuf empty now */
				if (start == end) {
					/* if it is go back to normal */
					state = INKEY_NORM;
				} else {
					/* otherwise go to backout state */
					state = INKEY_BACKOUT;
				}

				/* return the symbol */
				return current->key[current->mapping[k]]->value.symbol;

			} else {
				/*
				 * Step on to next part of the multi-key
				 * sequence.
				 */
				current = current->key[current->mapping[k]]->value.next;
			}
		}
	}
}

#ifndef _CURSES_USE_MACROS
/*
 * getch --
 *	Read in a character from stdscr.
 */
int
getch(void)
{
	return wgetch(stdscr);
}

/*
 * mvgetch --
 *      Read in a character from stdscr at the given location.
 */
int
mvgetch(int y, int x)
{
	return mvwgetch(stdscr, y, x);
}

/*
 * mvwgetch --
 *      Read in a character from stdscr at the given location in the
 *      given window.
 */
int
mvwgetch(WINDOW *win, int y, int x)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return wgetch(win);
}

#endif

/*
 * keyok --
 *      Set the enable flag for a keysym, if the flag is false then
 * getch will not return this keysym even if the matching key sequence
 * is seen.
 */
int
keyok(int key_type, bool flag)
{
	int result = ERR;
	
	do_keyok(_cursesi_screen->base_keymap, key_type, flag, &result);
	return result;
}

/*
 * do_keyok --
 *       Does the actual work for keyok, we need to recurse through the
 * keymaps finding the passed key symbol.
 */
void
do_keyok(keymap_t *current, int key_type, bool flag, int *retval)
{
	key_entry_t *key;
	int i;

	  /*
	   * we need to iterate over all the keys as there may be
	   * multiple instances of the leaf symbol.
	   */
	for (i = 0; i < MAX_CHAR; i++) {
		if (current->mapping[i] < 0)
			continue; /* no mapping for the key, next! */

		key = current->key[current->mapping[i]];

		if (key->type == KEYMAP_MULTI)
			do_keyok(key->value.next, key_type, flag, retval);
		else if ((key->type == KEYMAP_LEAF)
			 && (key->value.symbol == key_type)) {
			key->enable = flag;
			*retval = OK; /* we found at least one instance, ok */
		}
	}
}

/*
 * define_key --
 *      Add a custom mapping of a key sequence to key symbol.
 *
 */
int
define_key(char *sequence, int symbol)
{

	if (symbol <= 0)
		return ERR;

	if (sequence == NULL)
		delete_key_sequence(_cursesi_screen->base_keymap, symbol);
	else
		add_key_sequence(_cursesi_screen, sequence, symbol);

	return OK;
}
	
/*
 * wgetch --
 *	Read in a character from the window.
 */
int
wgetch(WINDOW *win)
{
	int inp, weset;
	int c;
	FILE *infd = _cursesi_screen->infd;

	if (!(win->flags & __SCROLLOK) && (win->flags & __FULLWIN)
	    && win->curx == win->maxx - 1 && win->cury == win->maxy - 1
	    && __echoit)
		return (ERR);

	if (is_wintouched(win))
		wrefresh(win);
#ifdef DEBUG
	__CTRACE("wgetch: __echoit = %d, __rawmode = %d, __nl = %d, flags = %#.4x\n",
	    __echoit, __rawmode, _cursesi_screen->nl, win->flags);
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
			if (__nodelay() == ERR) {
				__restore_termios();
				return ERR;
			}
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

		c = getchar();
		if (feof(infd)) {
			clearerr(infd);
			__restore_termios();
			return ERR;	/* we have timed out */
		}
		
		if (ferror(infd)) {
			clearerr(infd);
			inp = ERR;
		} else {
			inp = c;
		}
	}
#ifdef DEBUG
	if (inp > 255)
		  /* we have a key symbol - treat it differently */
		  /* XXXX perhaps __unctrl should be expanded to include
		   * XXXX the keysyms in the table....
		   */
		__CTRACE("wgetch assembled keysym 0x%x\n", inp);
	else
		__CTRACE("wgetch got '%s'\n", unctrl(inp));
#endif
	if (win->delay > -1) {
		if (__delay() == ERR) {
			__restore_termios();
			return ERR;
		}
	}

	__restore_termios();

	if (__echoit)
		waddch(win, (chtype) inp);

	if (weset)
		nocbreak();

	if (_cursesi_screen->nl && inp == 13)
		inp = 10;

	return ((inp < 0) || (inp == ERR) ? ERR : inp);
}

/*
 * ungetch --
 *     Put the character back into the input queue.
 */
int
ungetch(int c)
{
	return ((ungetc(c, _cursesi_screen->infd) == EOF) ? ERR : OK);
}
