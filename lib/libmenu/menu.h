/*      $Id: menu.h,v 1.5.2.2 1999/12/27 18:30:03 wrstuden Exp $ */

/*-
 * Copyright (c) 1998-1999 Brett Lymn (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#ifndef	_MENU_H_
#define	_MENU_H_

#include <curses.h>
#include <eti.h>

/* the following is a hack to define attr_t until the curses lib
   does it officially */
#ifndef CURSES_V3
typedef char attr_t;
#endif

/* requests for the menu_driver call */
#define REQ_BASE_NUM      (0x100)
#define REQ_LEFT_ITEM     (0x101)
#define REQ_RIGHT_ITEM    (0x102)
#define REQ_UP_ITEM       (0x103)
#define REQ_DOWN_ITEM     (0x104)
#define REQ_SCR_ULINE     (0x105)
#define REQ_SCR_DLINE     (0x106)
#define REQ_SCR_DPAGE     (0x107)
#define REQ_SCR_UPAGE     (0x108)
#define REQ_FIRST_ITEM    (0x109)
#define REQ_LAST_ITEM     (0x10a)
#define REQ_NEXT_ITEM     (0x10b)
#define REQ_PREV_ITEM     (0x10c)
#define REQ_TOGGLE_ITEM   (0x10d)
#define REQ_CLEAR_PATTERN (0x10e)
#define REQ_BACK_PATTERN  (0x10f)
#define REQ_NEXT_MATCH    (0x110)
#define REQ_PREV_MATCH    (0x111)

#define MAX_COMMAND       (0x111) /* last menu driver request - for application
				     defined commands */

/* Menu options */
typedef unsigned int OPTIONS;

/* and the values they can have */
#define O_ONEVALUE   (0x1)
#define O_SHOWDESC   (0x2)
#define O_ROWMAJOR   (0x4)
#define O_IGNORECASE (0x8)
#define O_SHOWMATCH  (0x10)
#define O_NONCYCLIC  (0x20)
#define O_SELECTABLE (0x40)

typedef struct __menu_str {
        char *string;
        int length;
} MENU_STR;

typedef struct __menu MENU;
typedef struct __item ITEM;

typedef void (*Menu_Hook) (MENU *);

struct __item {
        MENU_STR name;
        MENU_STR description;
        char *userptr;
        int visible;  /* set if item is visible */
        int selected; /* set if item has been selected */
	int row; /* menu row this item is on */
	int col; /* menu column this item is on */
        OPTIONS opts;
        MENU *parent; /* menu this item is bound to */
	int index; /* index number for this item, if attached */
	  /* The following are the item's neighbours - makes menu
	     navigation easier */
	ITEM *left;
	ITEM *right;
	ITEM *up;
	ITEM *down;
};

struct __menu {
        int rows; /* max number of rows to be displayed */
        int cols; /* max number of columns to be displayed */
	int item_rows; /* number of item rows we have */
	int item_cols; /* number of item columns we have */
        int cur_row; /* current cursor row */
        int cur_col; /* current cursor column */
        MENU_STR mark; /* menu mark string */
        MENU_STR unmark; /* menu unmark string */
        OPTIONS opts; /* options for the menu */
        char *pattern; /* the pattern buffer */
	int plen;  /* pattern buffer length */
	int match_len; /* length of pattern matched */
        int posted; /* set if menu is posted */
        attr_t fore; /* menu foreground */
        attr_t back; /* menu background */
        attr_t grey; /* greyed out (nonselectable) menu item */
        int pad;  /* filler char between name and description */
        char *userptr;
	int top_row; /* the row that is at the top of the menu */
	int max_item_width; /* widest item */
	int col_width; /* width of the menu columns - this is not always
			  the same as the widest item */
        int item_count; /* number of items attached */
        ITEM **items; /* items associated with this menu */
        int  cur_item; /* item cursor is currently positioned at */
        int in_init; /* set when processing an init or term function call */
        Menu_Hook menu_init; /* call this when menu is posted */
        Menu_Hook menu_term; /* call this when menu is unposted */
        Menu_Hook item_init; /* call this when menu posted & after
				       current item changes */
        Menu_Hook item_term; /* call this when menu unposted & just
				       before current item changes */
        WINDOW *menu_win; /* the menu window */
        WINDOW *menu_subwin; /* the menu subwindow */
	int we_created;
};


/* Public function prototypes. */
__BEGIN_DECLS
int  menu_driver __P((MENU *, int));
int scale_menu __P((MENU *, int *, int *));
int set_top_row __P((MENU *, int));
int pos_menu_cursor __P((MENU *));
int top_row __P((MENU *));

int  free_menu __P((MENU *));
char menu_back __P((MENU *));
char menu_fore __P((MENU *));
void menu_format __P((MENU *, int *, int *));
char menu_grey __P((MENU *));
Menu_Hook menu_init __P((MENU *));
char *menu_mark __P((MENU *));
OPTIONS menu_opts __P((MENU *));
int menu_opts_off __P((MENU *, OPTIONS));
int menu_opts_on __P((MENU *, OPTIONS));
int menu_pad __P((MENU *));
char *menu_pattern __P((MENU *));
WINDOW *menu_sub __P((MENU *));
Menu_Hook menu_term __P((MENU *));
char *menu_unmark __P((MENU *));
char *menu_userptr __P((MENU *));
WINDOW *menu_win __P((MENU *));
MENU *new_menu __P((ITEM **));
int post_menu __P((MENU *));
int set_menu_back __P((MENU *, attr_t));
int set_menu_fore __P((MENU *, attr_t));
int set_menu_format __P((MENU *, int, int));
int set_menu_grey __P((MENU *, attr_t));
int set_menu_init __P((MENU *, Menu_Hook));
int set_menu_items __P((MENU *, ITEM **));
int set_menu_mark __P((MENU *, char *));
int set_menu_opts __P((MENU *, OPTIONS));
int set_menu_pad __P((MENU *, int));
int set_menu_pattern __P((MENU *, char *));
int  set_menu_sub __P((MENU *, WINDOW *));
int set_menu_term __P((MENU *, Menu_Hook));
int set_menu_unmark __P((MENU *, char *));
int set_menu_userptr __P((MENU *, char *));
int  set_menu_win __P((MENU *, WINDOW *));
int unpost_menu __P((MENU *));

ITEM *current_item __P((MENU *));
int free_item __P((ITEM *));
int item_count __P((MENU *));
char *item_description __P((ITEM *));
int item_index __P((ITEM *));
Menu_Hook item_init __P((MENU *));
char *item_name __P((ITEM *));
OPTIONS item_opts __P((ITEM *));
int item_opts_off __P((ITEM *, OPTIONS));
int item_opts_on __P((ITEM *, OPTIONS));
Menu_Hook item_term __P((MENU *));
char *item_userptr __P((ITEM *));
int item_value __P((ITEM *));
int item_visible __P((ITEM *));
ITEM **menu_items __P((MENU *));
ITEM *new_item __P((char *, char *));
int set_current_item __P((MENU *, ITEM *));
int set_item_init __P((MENU *, Menu_Hook));
int set_item_opts __P((ITEM *, OPTIONS));
int set_item_term __P((MENU *, Menu_Hook));
int set_item_userptr __P((ITEM *, char *));
int set_item_value __P((ITEM *, int));

__END_DECLS

#endif /* !_MENU_H_ */
