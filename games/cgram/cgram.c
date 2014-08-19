/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <err.h>
#include <assert.h>
#include <curses.h>
#include "pathnames.h"

////////////////////////////////////////////////////////////

static char *xstrdup(const char *s) {
   char *ret;

   ret = malloc(strlen(s) + 1);
   if (ret == NULL) {
      errx(1, "Out of memory");
   }
   strcpy(ret, s);
   return ret;
}

////////////////////////////////////////////////////////////

struct stringarray {
   char **v;
   int num;
};

static void stringarray_init(struct stringarray *a) {
   a->v = NULL;
   a->num = 0;
}

static void stringarray_cleanup(struct stringarray *a) {
   free(a->v);
}

static void stringarray_add(struct stringarray *a, const char *s) {
   a->v = realloc(a->v, (a->num + 1) * sizeof(a->v[0]));
   if (a->v == NULL) {
      errx(1, "Out of memory");
   }
   a->v[a->num] = xstrdup(s);
   a->num++;
}

////////////////////////////////////////////////////////////

static struct stringarray lines;
static struct stringarray sollines;
static bool hinting;
static int scrolldown;
static unsigned curx;
static int cury;

static void readquote(void) {
   FILE *f = popen(_PATH_FORTUNE, "r");
   if (!f) {
      err(1, "%s", _PATH_FORTUNE);
   }

   char buf[128], buf2[8*sizeof(buf)];
   while (fgets(buf, sizeof(buf), f)) {
      char *s = strrchr(buf, '\n');
      assert(s);
      assert(strlen(s)==1);
      *s = 0;

      int i,j;
      for (i=j=0; buf[i]; i++) {
	 if (buf[i]=='\t') {
	    buf2[j++] = ' ';
	    while (j%8) buf2[j++] = ' ';
	 }
	 else if (buf[i]=='\b') {
	    if (j>0) j--;
	 }
	 else {
	    buf2[j++] = buf[i];
	 }
      }
      buf2[j] = 0;

      stringarray_add(&lines, buf2);
      stringarray_add(&sollines, buf2);
   }

   pclose(f);
}

static void encode(void) {
   int used[26];
   for (int i=0; i<26; i++) used[i] = 0;

   int key[26];
   int keypos=0;
   while (keypos < 26) {
      int c = random()%26;
      if (used[c]) continue;
      key[keypos++] = c;
      used[c] = 1;
   }

   for (int y=0; y<lines.num; y++) {
      for (unsigned x=0; lines.v[y][x]; x++) {
	 if (islower((unsigned char)lines.v[y][x])) {
	    int q = lines.v[y][x]-'a';
	    lines.v[y][x] = 'a'+key[q];
	 }
	 if (isupper((unsigned char)lines.v[y][x])) {
	    int q = lines.v[y][x]-'A';
	    lines.v[y][x] = 'A'+key[q];
	 }
      }
   }
}

static int substitute(int ch) {
   assert(cury>=0 && cury<lines.num);
   if (curx >= strlen(lines.v[cury])) {
      beep();
      return -1;
   }

   int och = lines.v[cury][curx];
   if (!isalpha((unsigned char)och)) {
      beep();
      return -1;
   }

   int loch = tolower((unsigned char)och);
   int uoch = toupper((unsigned char)och);
   int lch = tolower((unsigned char)ch);
   int uch = toupper((unsigned char)ch);

   for (int y=0; y<lines.num; y++) {
      for (unsigned x=0; lines.v[y][x]; x++) {
	 if (lines.v[y][x]==loch) {
	    lines.v[y][x] = lch;
	 }
	 else if (lines.v[y][x]==uoch) {
	    lines.v[y][x] = uch;
	 }
	 else if (lines.v[y][x]==lch) {
	    lines.v[y][x] = loch;
	 }
	 else if (lines.v[y][x]==uch) {
	    lines.v[y][x] = uoch;
	 }
      }
   }
   return 0;
}

////////////////////////////////////////////////////////////

static void redraw(void) {
   erase();
   bool won = true;
   for (int i=0; i<LINES-1; i++) {
      move(i, 0);
      int ln = i+scrolldown;
      if (ln < lines.num) {
	 for (unsigned j=0; lines.v[i][j]; j++) {
	    int ch = lines.v[i][j];
	    if (ch != sollines.v[i][j] && isalpha((unsigned char)ch)) {
	       won = false;
	    }
	    bool bold=false;
	    if (hinting && ch==sollines.v[i][j] &&
		isalpha((unsigned char)ch)) {
	       bold = true;
	       attron(A_BOLD);
	    }
	    addch(lines.v[i][j]);
	    if (bold) {
	       attroff(A_BOLD);
	    }
	 }
      }
      clrtoeol();
   }

   move(LINES-1, 0);
   if (won) {
      addstr("*solved* ");
   }
   addstr("~ to quit, * to cheat, ^pnfb to move");

   move(LINES-1, 0);

   move(cury-scrolldown, curx);

   refresh();
}

static void opencurses(void) {
    initscr();
    cbreak();
    noecho();
}

static void closecurses(void) {
   endwin();
}

////////////////////////////////////////////////////////////

static void loop(void) {
   bool done=false;
   while (!done) {
      redraw();
      int ch = getch();
      switch (ch) {
       case 1: /* ^A */
	curx=0;
	break;
       case 2: /* ^B */
	if (curx > 0) {
	   curx--;
	}
	else if (cury > 0) {
	   cury--;
	   curx = strlen(lines.v[cury]);
	}
	break;
       case 5: /* ^E */
	curx = strlen(lines.v[cury]);
	break;
       case 6: /* ^F */
	if (curx < strlen(lines.v[cury])) {
	   curx++;
	}
	else if (cury < lines.num - 1) {
	   cury++;
	   curx = 0;
	}
	break;
       case 12: /* ^L */
	clear();
	break;
       case 14: /* ^N */
	if (cury < lines.num-1) {
	   cury++;
	}
	if (curx > strlen(lines.v[cury])) {
	   curx =  strlen(lines.v[cury]);
	}
	if (scrolldown < cury - (LINES-2)) {
	   scrolldown = cury - (LINES-2);
	}
	break;
       case 16: /* ^P */
	if (cury > 0) {
	   cury--;
	}
	if (curx > strlen(lines.v[cury])) {
	   curx = strlen(lines.v[cury]);
	}
	if (scrolldown > cury) {
	   scrolldown = cury;
	}
	break;
       case '*':
	hinting = !hinting;
	break;
       case '~':
	done = true;
	break;
       default:
	if (isalpha(ch)) {
	   if (!substitute(ch)) {
	      if (curx < strlen(lines.v[cury])) {
		 curx++;
	      }
	      if (curx==strlen(lines.v[cury]) && cury < lines.num-1) {
		 curx=0;
		 cury++;
	      }
	   }
	}
	else if (curx < strlen(lines.v[cury]) && ch==lines.v[cury][curx]) {
	   curx++;
	   if (curx==strlen(lines.v[cury]) && cury < lines.num-1) {
	      curx=0;
	      cury++;
	   }
	}
	else {
	   beep();
	}
	break;
      }
   }
}

////////////////////////////////////////////////////////////

int main(void) {
   stringarray_init(&lines);
   stringarray_init(&sollines);
   srandom(time(NULL));
   readquote();
   encode();
   opencurses();

   loop();

   closecurses();
   stringarray_cleanup(&sollines);
   stringarray_cleanup(&lines);
   return 0;
}
