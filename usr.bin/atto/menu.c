/*	$NetBSD: menu.c,v 1.1 2026/10/30 16:14:35 nia Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *			 (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * Copyright (c) 2026 Nia Alarie <nia@NetBSD.org>
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
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
 */

#include "header.h"
#include <menu.h>

int menu_mode = 0;

static int menu_ready;
static int col_width;

static WINDOW *top_win;

static MENU *top_menu;
static ITEM *top_items[8];

static MENU *file_menu;
static ITEM *file_items[8];

static MENU *edit_menu;
static ITEM *edit_items[8];

static MENU *view_menu;
static ITEM *view_items[8];

static WINDOW *create_menu_popup(MENU *, MENU *, int, int, int, int);
static void handle_menu_popup(MENU *, WINDOW *, const char *);
static int file_item_init(void);
static int edit_item_init(void);
static int view_item_init(void);
static void quit_item_callback(MENU *);
static void file_item_callback(MENU *);
static void edit_item_callback(MENU *);
static void view_item_callback(MENU *);

static void
open_item_callback(MENU *parent) { (void)parent; readfile(); }

static void
save_item_callback(MENU *parent) { (void)parent; savebuffer(); }

static void
saveas_item_callback(MENU *parent) { (void)parent; writefile(); }

static void
find_item_callback(MENU *parent) { (void)parent; search(); }

static void
replace_item_callback(MENU *parent) { (void)parent; query_replace(); }

static void
paste_item_callback(MENU *parent) { (void)parent; paste(); }

static void
cut_item_callback(MENU *parent) { (void)parent; killtoeol(); }

static void
goto_item_callback(MENU *parent) { (void)parent; gotoline(); }

static void
quit_item_callback(MENU *parent) { (void)parent; quit_ask(); }

static void
next_item_callback(MENU *parent) { (void)parent; next_buffer(); }

static void
status_item_callback(MENU *parent) { (void)parent; showpos(); }

/*
 * Create and return an appropriately sized and positioned window for
 * the given menu.  If x is less than 0 then the function uses the
 * parent menu position to calculate the correct location of the
 * popup.  The parameter y must always be a valid location.  If either
 * drows or dcols are less than 0 then the child menu dimensions are
 * used to set the window size. The parent parameter may be NULL iff x
 * is greater than or equal to 0.  Returns NULL on error.
 */
static WINDOW *
create_menu_popup(MENU *parent, MENU *child, int y, int x,
		  int drows, int dcols)
{
	int lhs, index, rows, cols;
	WINDOW *popup;

	assert(child != NULL);

	scale_menu(child, &rows, &cols);
	if (drows > 0)
		rows = drows;
	if (dcols > 0)
		cols = dcols;

	if (x >= 0) {
		lhs = x;
	} else {
		if (parent == NULL) {
			fatal("%s: Bad parent in create_menu_popup\n");
		}

		index = item_index(current_item(parent));
		lhs = index * col_width;
		if (lhs > COLS)
			lhs = COLS - cols;
	}

	if ((popup = newwin(rows, cols, y, lhs)) == NULL) {
		fatal("%s: Could not create popup win\n");
	}

	assert(popup != NULL);

	keypad(popup, TRUE);
	nodelay(popup, FALSE);
	set_menu_sub(child, popup);
	return popup;
}

/*
 * Do a generic menu handling for the given menu in the given popup window.
 * This includes posting the menu, handling the key event loop and then
 * unposting the menu when done.  The name parameter is used when reporting
 * errors.
 */
static void
handle_menu_popup(MENU *menu, WINDOW *popup, const char *name)
{
	int c, isdone = 0, status;
	Menu_Hook callback;
	ITEM *current;

	(void)name;
	if ((status = post_menu(menu)) < 0) {
		fatal("%s: Post of popup menu failed\n");
		return;
	}

	wrefresh(popup);
	do {
		c = wgetch(popup);
		switch (c) {
		case '\e':
			isdone = 1;
			break;

		case '\t':
		case KEY_DOWN:
			menu_driver(menu, REQ_DOWN_ITEM);
			break;

		case KEY_UP:
			menu_driver(menu, REQ_UP_ITEM);
			break;

		case '\n':
		case '\r':
			if ((current = current_item(menu)) != NULL) {
				callback = (Menu_Hook) item_userptr(current);
				if (callback != NULL) {
					callback(menu);
					menu_mode = 0;
					isdone = 1;
				}
			}

			break;

		case KEY_LEFT:
			menu_driver(top_menu, REQ_LEFT_ITEM);
			isdone = 1;
			break;

		case KEY_RIGHT:
			menu_driver(top_menu, REQ_RIGHT_ITEM);
			isdone = 1;
			break;

		default:
			menu_driver(menu, c);
			break;
		}

		wrefresh(popup);
	} while (isdone == 0);

	unpost_menu(menu);
	return;
}

/*
 * Handle the menu mode keystrokes.
 */
void
handle_menus(int c)
{
	Menu_Hook cb;
	ITEM *current;

	post_menu(top_menu);
	touchwin(top_win);
	wrefresh(top_win);

	switch (c) {
	case '\e':
		menu_mode = 0;
		break;
  
	case KEY_UP:
		beep();
		break;

	case '\r':
	case '\n':
	case KEY_DOWN:
		if ((current = current_item(top_menu)) != NULL) {
			cb = (Menu_Hook) item_userptr(current);
			if (cb != NULL) {
				cb(top_menu);
				//refresh_screen();
			}
		}
		break;

	case KEY_LEFT:
		menu_driver(top_menu, REQ_LEFT_ITEM);
		break;

	case '\t':
	case KEY_RIGHT:
		menu_driver(top_menu, REQ_RIGHT_ITEM);
		break;

	default:
		menu_driver(top_menu, c); /* this makes item search work */
		break;
	}

	wrefresh(top_win);
}

static int
file_item_init(void)
{
	if ((file_items[0] = new_item(__UNCONST("Open"),
	    __UNCONST("C-x c-f"))) == NULL) {
		return -1;
	}
	if ((file_items[1] = new_item(__UNCONST("Save"),
	    __UNCONST("C-x c-s"))) == NULL) {
		return -1;
	}
	if ((file_items[2] = new_item(__UNCONST("Save As..."),
	    __UNCONST("C-x c-w"))) == NULL) {
		return -1;
	}
	if ((file_items[3] = new_item(__UNCONST("Quit"),
	    __UNCONST("C-x c-c"))) == NULL) {
		return -1;
	}

	file_items[4] = NULL;

	set_item_userptr(file_items[0], (char *) &open_item_callback);
	set_item_userptr(file_items[1], (char *) &save_item_callback);
	set_item_userptr(file_items[2], (char *) &saveas_item_callback);
	set_item_userptr(file_items[3], (char *) &quit_item_callback);

	if ((file_menu = new_menu(file_items)) == NULL) {
		return -1;
	}

	set_menu_format(file_menu, 4, 1);
	menu_opts_on(file_menu, O_SHOWDESC);

	return 0;
}

static int
edit_item_init(void)
{
	if ((edit_items[0] = new_item(__UNCONST("Cut line"),
	    __UNCONST("Ctrl-K"))) == NULL) {
		return -1;
	}
	if ((edit_items[1] = new_item(__UNCONST("Paste"),
	    __UNCONST("Ctrl-Y"))) == NULL) {
		return -1;
	}
	if ((edit_items[2] = new_item(__UNCONST("Find..."),
	    __UNCONST("Ctrl-S"))) == NULL) {
		return -1;
	}
	if ((edit_items[3] = new_item(__UNCONST("Find & Replace"),
	    __UNCONST("Ctrl-R"))) == NULL) {
		return -1;
	}
	if ((edit_items[4] = new_item(__UNCONST("Go To..."),
	    __UNCONST("Esc-G"))) == NULL) {
		return -1;
	}

	set_item_userptr(edit_items[0], (char *) &cut_item_callback);
	set_item_userptr(edit_items[1], (char *) &paste_item_callback);
	set_item_userptr(edit_items[2], (char *) &find_item_callback);
	set_item_userptr(edit_items[3], (char *) &replace_item_callback);
	set_item_userptr(edit_items[4], (char *) &goto_item_callback);

	edit_items[5] = NULL;

	if ((edit_menu = new_menu(edit_items)) == NULL)
		fatal("%s: Failed to create edit menu\n");

	set_menu_format(edit_menu, 5, 1);
	menu_opts_on(edit_menu, O_SHOWDESC);
	return 0;
}

static int
view_item_init(void)
{
	if ((view_items[0] = new_item(__UNCONST("Next buffer"),
	    __UNCONST("C-x n"))) == NULL) {
		return -1;
	}
	if ((view_items[1] = new_item(__UNCONST("Status line"),
	    __UNCONST("C-x ="))) == NULL) {
		return -1;
	}

	set_item_userptr(view_items[0], (char *) &next_item_callback);
	set_item_userptr(view_items[1], (char *) &status_item_callback);

	view_items[2] = NULL;

	if ((view_menu = new_menu(view_items)) == NULL)
		fatal("%s: Failed to create view menu\n");

	set_menu_format(view_menu, 2, 1);
	menu_opts_on(view_menu, O_SHOWDESC);
	return 0;
}


static void
file_item_callback(MENU *parent)
{
	WINDOW *file_popup;
 
	if ((file_popup = create_menu_popup(parent, file_menu, 1, -1, -1, -1))
	    == NULL) {
		fatal("%s: File menu popup failed.\n");
		return;
	}
 
	handle_menu_popup(file_menu, file_popup, "file");
}

static void
edit_item_callback(MENU *parent)
{
	WINDOW *edit_popup;
 
	if ((edit_popup = create_menu_popup(parent, edit_menu, 1, -1, -1, -1))
	    == NULL) {
		fatal("%s: Edit menu popup failed.\n");
		return;
	}
 
	handle_menu_popup(edit_menu, edit_popup, "edit");
}

static void
view_item_callback(MENU *parent)
{
	WINDOW *view_popup;
 
	if ((view_popup = create_menu_popup(parent, view_menu, 1, -1, -1, -1))
	    == NULL) {
		fatal("%s: View menu popup failed.\n");
		return;
	}
 
	handle_menu_popup(view_menu, view_popup, "view");
}

int
create_menu(void)
{
	int top_rows, top_cols;
	int status;

	/* set default menu attributes - all menus will inherit these */
	set_menu_back(NULL, A_REVERSE);
	set_menu_fore(NULL, A_NORMAL);

	/* ignore case and show matches */
	menu_opts_on(NULL, O_IGNORECASE | O_SHOWMATCH);

	if ((top_items[0] = new_item(__UNCONST("File"),
	    __UNCONST("File Menu"))) == NULL) {
		return -1;
	}

	if ((top_items[1] = new_item(__UNCONST("Edit"),
	    __UNCONST("Edit Menu"))) == NULL) {
		return -1;
	}

	if ((top_items[2] = new_item(__UNCONST("View"),
	    __UNCONST("View Menu"))) == NULL) {
		return -1;
	}

	top_items[3] = NULL;

	set_item_userptr(top_items[0], (char *) file_item_callback);
	set_item_userptr(top_items[1], (char *) edit_item_callback);
	set_item_userptr(top_items[2], (char *) view_item_callback);

	if (file_item_init() != 0) return -1;
	if (edit_item_init() != 0) return -1;
	if (view_item_init() != 0) return -1;

	top_menu = new_menu(top_items);
	if (top_menu == NULL) {
		return -1;
	}

	menu_opts_off(top_menu, O_ROWMAJOR);      
	set_menu_format(top_menu, 1, 4);

	scale_menu(top_menu, &top_rows, &top_cols);

	if ((top_win = newwin(1, top_cols, 0,0)) == NULL) {
		return -1;
	}

	keypad(top_win, TRUE);
	nodelay(top_win, FALSE);

	set_menu_sub(top_menu, top_win);
 
	col_width = top_cols / 4;

	if ((status = post_menu(top_menu)) < 0) {
		return -1;
	}

	wrefresh(top_win);

	menu_ready = 1;
	return 0;
}

void
menu_escape(void)
{
	if (!menu_ready) return;
	menu_mode = !menu_mode;
	touchwin(top_win);
	wrefresh(top_win);
}
