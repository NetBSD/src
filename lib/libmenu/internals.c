/*      $NetBSD: internals.c,v 1.2 1999/11/24 12:43:15 kleink Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn (blymn@baea.com.au, brett_lymn@yahoo.com)
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

#include <menu.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "internals.h"

/* internal function prototypes */
static void
__menui_calc_neighbours(MENU *, int, int, int, int, ITEM **, ITEM **,
			ITEM **, ITEM **);
static void __menui_redraw_menu __P((MENU *, int, int));

  /*
   * Link all the menu items together to speed up navigation.  We need
   * to calculate the widest item entry, then work out how many columns
   * of items the window will accomodate and then how many rows there will
   * be.  Once the layout is determined the neighbours of each item is
   * calculated and the item structures updated.
   */
int
__menui_stitch_items(menu)
	MENU *menu;
{
	int i, cycle, row_major;

	cycle = ((menu->opts & O_NONCYCLIC) != O_NONCYCLIC);
	row_major = ((menu->opts & O_ROWMAJOR) == O_ROWMAJOR);

	if (menu->posted == 1)
		return E_POSTED;
	if (menu->items == NULL)
		return E_BAD_ARGUMENT;

	if (row_major) {
		menu->item_rows = menu->item_count / menu->cols;
		menu->item_cols = menu->cols;
		if (menu->item_count > (menu->item_rows * menu->item_cols))
			menu->item_rows += 1;
	} else {
		menu->item_cols = menu->item_count / menu->rows;
		menu->item_rows = menu->rows;
		if (menu->item_count > (menu->item_rows * menu->item_cols))
			menu->item_cols += 1;
	}
	

	__menui_max_item_size(menu);

	for (i = 0; i < menu->item_count; i++) {
		  /* Calculate the neighbours.  The ugliness here deals with
		   * the differing menu layout styles.  The layout affects
		   * the neighbour calculation so we change the arguments
		   * around depending on the layout style.
		   */
		__menui_calc_neighbours(menu, i, cycle,
					(row_major) ? menu->item_rows
					: menu->item_cols,
					(row_major) ? menu->item_cols
					: menu->item_rows,
					(row_major) ? &menu->items[i]->right
					: &menu->items[i]->down,
					(row_major) ? &menu->items[i]->left
					: &menu->items[i]->up,
					(row_major) ? &menu->items[i]->down
					: &menu->items[i]->right,
					(row_major) ? &menu->items[i]->up
					: &menu->items[i]->left);
		
		  /* fill in the row and column value of the item */
		if (row_major) {
			menu->items[i]->row = i / menu->item_cols;
			menu->items[i]->col = i % menu->item_cols;
		} else {
			menu->items[i]->row = i % menu->item_rows;
			menu->items[i]->col = i / menu->item_rows;
		}
	}
	
	return E_OK;
}

  /*
   * Calculate the neighbours for an item in menu.  This routine deliberately
   * does not refer to up/down/left/right as these concepts depend on the menu
   * layout style (row major or not).  By arranging the arguments in the right
   * order the caller can generate the the neighbours for either menu layout
   * style.
   */
static void
__menui_calc_neighbours(menu, index, cycle, item_rows, item_cols, next, prev,
			major_next, major_prev)
	MENU *menu;
	int index;
	int cycle;
	int item_rows;
	int item_cols;
	ITEM **next;
	ITEM **prev;
	ITEM **major_next;
	ITEM **major_prev;
{
	int neighbour;

	if (item_rows < 2) {
		if (cycle) {
			*major_next = menu->items[index];
			*major_prev = menu->items[index];
		} else {
			*major_next = NULL;
			*major_prev = NULL;
		}
	} else {
		neighbour = index + item_cols;
		if (neighbour >= menu->item_count) {
			if (cycle) {
				if (item_rows == 2) {
					neighbour = index - item_cols;
					if (neighbour < 0)
						neighbour = index;
					*major_next = menu->items[neighbour];
				} else {
					*major_next =
						menu->items[index % item_cols];
				}
			} else
				*major_next = NULL;
		} else
			*major_next = menu->items[neighbour];
		
		
		neighbour = index - item_cols;
		if (neighbour < 0) {
			if (cycle) {
				if (item_rows == 2) {
					neighbour = index + item_cols;
					if (neighbour >= menu->item_count)
						neighbour = index;
					*major_prev = menu->items[neighbour];
				} else {
					neighbour = index +
						(item_rows - 1) * item_cols;

					if (neighbour >= menu->item_count)
						neighbour = index +
							(item_rows - 2)
							* item_cols;
					
					*major_prev = menu->items[neighbour];
				}
			} else
				*major_prev = NULL;
		} else
			*major_prev = menu->items[neighbour];
	}
	
	if ((index % item_cols) == 0) {
		if (cycle) {
			if (item_cols  < 2) {
				*prev = menu->items[index];
			} else {
				neighbour = index + item_cols - 1;
				if (neighbour >= menu->item_count) {
					if (item_cols == 2) {
						*prev = menu->items[index];
					} else {
						*prev = menu->items[menu->item_count - 1];
					}
				} else
					*prev = menu->items[neighbour];
			}
		} else
			*prev = NULL;
	} else
		*prev = menu->items[index - 1];
	
	if ((index % item_cols) == (item_cols - 1)) {
		if (cycle) {
			if (item_cols  < 2) {
				*next = menu->items[index];
			} else {
				neighbour = index - item_cols + 1;
				if (neighbour >= menu->item_count) {
					if (item_cols == 2) {
						*next = menu->items[index];
					} else {
						neighbour = item_cols * index / item_cols;
						
						*next = menu->items[neighbour];
					}
				} else
					*next = menu->items[neighbour];
			}
		} else
			*next = NULL;
	} else {
		neighbour = index + 1;
		if (neighbour >= menu->item_count) {
			if (cycle) {
				neighbour = item_cols * (item_rows - 1);
				*next = menu->items[neighbour];
			} else
				*next = NULL;
		} else
			*next = menu->items[neighbour];
	}
}

/*
 * Goto the item pointed to by item and adjust the menu structure
 * accordingly.  Call the term and init functions if required.
 */
int
__menui_goto_item(menu, item, top_row)
	MENU *menu;
	ITEM *item;
	int top_row;
{
	int old_top_row = menu->top_row, old_cur_item = menu->cur_item;
	
	  /* If we get a null then the menu is not cyclic so deny request */
	if (item == NULL)
		return E_REQUEST_DENIED;

	menu->in_init = 1;
	if (menu->top_row != top_row) {
		if ((menu->posted == 1) && (menu->menu_term != NULL))
			menu->menu_term(menu);
		menu->top_row = top_row;

		if ((menu->posted == 1) && (menu->menu_init != NULL))
			menu->menu_init(menu);
	}
	
	  /* this looks like wasted effort but it can happen.... */
	if (menu->cur_item != item->index) {

		if ((menu->posted == 1) && (menu->item_term != NULL))
			menu->item_term(menu);

		menu->cur_item = item->index;
		menu->cur_row = item->row;
		menu->cur_col = item->col;

		if (menu->posted == 1)
			__menui_redraw_menu(menu, old_top_row, old_cur_item);
		
		if ((menu->posted == 1) && (menu->item_init != NULL))
			menu->item_init(menu);
		
	}

	menu->in_init = 0;
	return E_OK;
}

/*
 * Attempt to match items with the pattern buffer in the direction given
 * by iterating over the menu items.  If a match is found return E_OK
 * otherwise return E_NO_MATCH
 */
int
__menui_match_items(menu, direction, item_matched)
	MENU *menu;
	int direction;
	int *item_matched;
{
	int i, caseless;

	caseless = ((menu->opts & O_IGNORECASE) == O_IGNORECASE);
	
	i = menu->cur_item;
	if (direction == MATCH_NEXT_FORWARD) {
		if (++i >= menu->item_count) i = 0;
	} else if (direction == MATCH_NEXT_REVERSE) {
		if (--i < 0) i = menu->item_count - 1;
	}

	
	do {
		if (menu->items[i]->name.length >= menu->plen) {
			  /* no chance if pattern is longer */
			if (caseless) {
				if (strncasecmp(menu->items[i]->name.string,
						menu->pattern,
						menu->plen) == 0) {
					*item_matched = i;
					menu->match_len = menu->plen;
					return E_OK;
				}
			} else {
				if (strncmp(menu->items[i]->name.string,
					    menu->pattern, menu->plen) == 0) {
					*item_matched = i;
					menu->match_len = menu->plen;
					return E_OK;
				}
			}
		}
	
		if ((direction == MATCH_FORWARD) ||
		    (direction == MATCH_NEXT_FORWARD)) {
			if (++i >= menu->item_count) i = 0;
		} else {
			if (--i <= 0) i = menu->item_count - 1;
		}
	} while (i != menu->cur_item);

	menu->match_len = 0; /* match did not succeed - kill the match len. */
	return E_NO_MATCH;
}

/*
 * Attempt to match the pattern buffer against the items.  If c is a 
 * printable character then add it to the pattern buffer prior to
 * performing the match.  Direction determines the direction of matching.
 * If the match is successful update the item_matched variable with the
 * index of the item that matched the pattern.
 */
int
__menui_match_pattern(menu, c, direction, item_matched)
	MENU *menu;
	char c;
	int direction;
	int *item_matched;
{
	if (menu == NULL)
		return E_BAD_ARGUMENT;
	if (menu->items == NULL)
		return E_BAD_ARGUMENT;
	if (*menu->items == NULL)
		return E_BAD_ARGUMENT;

	if (isprint(c)) {
		  /* add char to buffer - first allocate room for it */
		if ((menu->pattern = (char *)
		     realloc(menu->pattern,
			     menu->plen + sizeof(char) +
			     ((menu->plen > 0)? 0 : 1)))
		    == NULL)
			return E_SYSTEM_ERROR;
		menu->pattern[menu->plen] = c;
		menu->pattern[++menu->plen] = '\0';

		  /* there is no chance of a match if pattern is longer
		     than all the items */
		if (menu->plen >= menu->max_item_width) {
			menu->pattern[--menu->plen] = '\0';
			return E_NO_MATCH;
		}
		
		if (__menui_match_items(menu, direction,
					item_matched) == E_NO_MATCH) {
			menu->pattern[--menu->plen] = '\0';
			return E_NO_MATCH;
		} else
			return E_OK;
	} else {
		if (__menui_match_items(menu, direction,
					item_matched) == E_OK) {
			return E_OK;
		} else {
			return E_NO_MATCH;
		}
	}
}

/*
 * Draw an item in the subwindow complete with appropriate highlighting.
 */
void
__menui_draw_item(menu, item)
	MENU *menu;
	int item;
{
	int j, pad_len, mark_len;
	
	mark_len = max(menu->mark.length, menu->unmark.length);
	
	wmove(menu->menu_subwin,
	      menu->items[item]->row - menu->top_row,
	      menu->items[item]->col * (menu->col_width + 1));
			
	if ((menu->cur_item == item) || (menu->items[item]->selected == 1))
		wattron(menu->menu_subwin, menu->fore);
	if ((menu->items[item]->opts & O_SELECTABLE) != O_SELECTABLE)
		wattron(menu->menu_subwin, menu->grey);

	  /* deal with the menu mark, if  one is set.
	   * We mark the selected items and write blanks for
	   * all others unless the menu unmark string is set in which
	   * case the unmark string is written.
	   */
	if (menu->items[item]->selected == 1) {
		if (menu->mark.string != NULL) {
			for (j = 0; j < menu->mark.length; j++) {
				waddch(menu->menu_subwin,
				       menu->mark.string[j]);
			}
		}
		  /* blank any length difference between mark & unmark */
		for (j = menu->mark.length; j < mark_len; j++)
			waddch(menu->menu_subwin, ' ');
	} else {
		if (menu->unmark.string != NULL) {
			for (j = 0; j < menu->unmark.length; j++) {
				waddch(menu->menu_subwin,
				       menu->unmark.string[j]);
			}
		}
		  /* blank any length difference between mark & unmark */
		for (j = menu->unmark.length; j < mark_len; j++)
			waddch(menu->menu_subwin, ' ');
	}
	
	  /* add the menu name */
	for (j=0; j < menu->items[item]->name.length; j++)
		waddch(menu->menu_subwin,
		       menu->items[item]->name.string[j]);
	
	pad_len = menu->col_width - menu->items[item]->name.length
		- mark_len - 1;
	if ((menu->opts & O_SHOWDESC) == O_SHOWDESC) {
		pad_len -= menu->items[item]->description.length - 1;
		for (j = 0; j < pad_len; j++)
			waddch(menu->menu_subwin, menu->pad);
		for (j = 0; j < menu->items[item]->description.length; j++) {
			waddch(menu->menu_subwin,
			       menu->items[item]->description.string[j]);
		}
	} else {
		for (j = 0; j < pad_len; j++)
			waddch(menu->menu_subwin, ' ');
	}
	menu->items[item]->visible = 1;
	  /* kill any special attributes... */
	wattrset(menu->menu_subwin, menu->back);

	  /* and position the cursor nicely */
	pos_menu_cursor(menu);
}

/*
 * Draw the menu in the subwindow provided.
 */
int
__menui_draw_menu(menu)
	MENU *menu;
{
	int rowmajor, i, j, max_items, last_item, row = -1, col = -1;
	
	rowmajor = ((menu->opts & O_ROWMAJOR) == O_ROWMAJOR);

	for (i = 0;  i < menu->item_count; i++) {
		if (menu->items[i]->row == menu->top_row)
			break;
		menu->items[i]->visible = 0;
	}

	wmove(menu->menu_subwin, 0, 0);

	menu->col_width = getmaxx(menu->menu_subwin) / menu->cols;

	  /*if ((menu->opts & O_SHOWDESC) == O_SHOWDESC)
	    menu->col_width++;*/
		
	max_items = menu->rows * menu->cols;
	last_item = ((max_items + i) > menu->item_count) ? menu->item_count :
		(max_items + i);
	
	for (; i < last_item; i++) {
		if (i > menu->item_count) {
			  /* no more items to draw, write background blanks */
			wattron(menu->menu_subwin, menu->back);
			if (row < 0) {
				row = menu->items[menu->item_count - 1]->row;
				col = menu->items[menu->item_count - 1]->col;
			}

			if (rowmajor) {
				col++;
				if (col > menu->cols) {
					col = 0;
					row++;
				}
			} else {
				row++;
				if (row > menu->rows) {
					row = 0;
					col++;
				}
			}
			wmove(menu->menu_subwin, row,
			      col * (menu->col_width + 1));
			for (j = 0; j < menu->col_width; j++)
				waddch(menu->menu_subwin, ' ');
		} else {
			__menui_draw_item(menu, i);
			
		}

	}

	if (last_item < menu->item_count) {
		for (j = last_item; j < menu->item_count; j++)
			menu->items[j]->visible = 0;
	}
	
	return E_OK;
}

	
/*
 * Calculate the widest menu item and stash it in the menu struct.
 *
 */
void
__menui_max_item_size(menu)
	MENU *menu;
{
	int i, with_desc, width;

	with_desc = ((menu->opts & O_SHOWDESC) == O_SHOWDESC);
	
	for (i = 0; i < menu->item_count; i++) {
		width = menu->items[i]->name.length
			+ max(menu->mark.length, menu->unmark.length);
		if (with_desc)
			width += menu->items[i]->description.length + 1;

		menu->max_item_width = max(menu->max_item_width, width);
	}
}


/*
 * Redraw the menu on the screen.  If the current item has changed then
 * unhighlight the old item and highlight the new one.
 */
static void
__menui_redraw_menu(menu, old_top_row, old_cur_item)
	MENU *menu;
	int old_top_row;
	int old_cur_item;
{

	if (menu->top_row != old_top_row) {
		  /* top row changed - redo the whole menu
		   * XXXX this could be improved if we had wscrl implemented.

		   * XXXX we could scroll the window and just fill in the
		   * XXXX changed lines.
		   */
		wclear(menu->menu_subwin);
		__menui_draw_menu(menu);
	} else {
		if (menu->cur_item != old_cur_item) {
			  /* redo the old item as a normal one. */
			__menui_draw_item(menu, old_cur_item);
		}
		  /* and then redraw the current item */
		__menui_draw_item(menu, menu->cur_item);
	}
}
