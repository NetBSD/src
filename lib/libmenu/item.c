/*	$NetBSD: item.c,v 1.5 1999/12/22 14:38:12 kleink Exp $	*/

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

#include <menu.h>
#include <stdlib.h>
#include <string.h>

/* the following is defined in menu.c - it is the default menu struct */
extern MENU _menui_default_menu;

/* keep default item options for setting in new_item */
ITEM _menui_default_item = {
	{NULL, 0}, /* item name struct */
	{NULL, 0}, /* item description struct */
	NULL, /* user pointer */
	0, /* is item visible? */
	0, /* is item selected? */
	0, /* row item is on */
	0, /* column item is on */
	O_SELECTABLE, /* item options */
	NULL, /* parent menu item is bound to */
	-1, /* index number if item attached to a menu */
	NULL, /* left neighbour */
	NULL, /* right neighbour */
	NULL, /* up neighbour */
	NULL /* down neighbour */
};

/*
 * Return the item visibility flag
 */
int
item_visible(item)
        ITEM *item;
{
	if (item == NULL)
		return E_BAD_ARGUMENT;
	if (item->parent == NULL)
		return E_NOT_CONNECTED;
	
        return item->visible;
}

/*
 * Return the pointer to the item name
 */
char *
item_name(item)
        ITEM *item;
{
	if (item == NULL)
		return NULL;
	
        return item->name.string;
}

/*
 * Return the pointer to the item description
 */
char *
item_description(item)
        ITEM *item;
{
	if (item == NULL)
		return NULL;
	
        return item->description.string;
}

/*
 * Set the application defined function called when the menu is posted or
 * just after the current item changes.
 */
int
set_item_init(menu, func)
        MENU *menu;
	Menu_Hook func;
{
	if (menu == NULL)
		_menui_default_menu.item_init = func;
	else
		menu->item_init = func;
        return E_OK;
}


/*
 * Return a pointer to the item initialisation routine.
 */
Menu_Hook
item_init(menu)
        MENU *menu;
{
	if (menu == NULL)
		return _menui_default_menu.item_init;
	else
		return menu->item_init;
}

/*
 * Set the user defined function to be called when menu is unposted or just
 * before the current item changes.
 */
int
set_item_term(menu, func)
        MENU *menu;
        Menu_Hook func;
{
	if (menu == NULL)
		_menui_default_menu.item_term = func;
	else
		menu->item_term = func;
        return E_OK;
}

/*
 * Return a pointer to the termination function
 */
Menu_Hook
item_term(menu)
        MENU *menu;
{
	if (menu == NULL)
		return _menui_default_menu.item_term;
	else
		return menu->item_term;
}

/*
 * Set the item options.  We keep a global copy of the current item options
 * as subsequent new_item calls will use the updated options as their
 * defaults.
 */
int
set_item_opts(item, opts)
        ITEM *item;
        OPTIONS opts;
{
          /* selectable seems to be the only allowable item opt! */
        if (opts != O_SELECTABLE)
                return E_SYSTEM_ERROR;

	if (item == NULL)
		_menui_default_item.opts = opts;
	else
		item->opts = opts;
        return E_OK;
}

/*
 * Set item options on.
 */
int
item_opts_on(item, opts)
        ITEM *item;
        OPTIONS opts;
{
        if (opts != O_SELECTABLE)
                return E_SYSTEM_ERROR;

        if (item == NULL)
		_menui_default_item.opts |= opts;
	else
		item->opts |= opts;
        return E_OK;
}

/*
 * Turn off the named options.
 */
int
item_opts_off(item, opts)
        ITEM *item;
        OPTIONS opts;
{
        if (opts != O_SELECTABLE)
                return E_SYSTEM_ERROR;

	if (item == NULL)
		_menui_default_item.opts &= ~(opts);
	else
		item->opts &= ~(opts);
        return E_OK;
}

/*
 * Return the current options set in item.
 */
OPTIONS
item_opts(item)
        ITEM *item;
{
	if (item == NULL)
		return _menui_default_item.opts;
	else
		return item->opts;
}

/*
 * Set the selected flag of the item iff the menu options allow it.
 */
int
set_item_value(param_item, flag)
        ITEM *param_item;
        int flag;
{
	ITEM *item = (param_item != NULL) ? param_item : &_menui_default_item;
	
          /* not bound to a menu */
        if (item->parent == NULL)
                return E_NOT_CONNECTED;

          /* menu options do not allow multi-selection */
        if ((item->parent->opts & O_ONEVALUE) == O_ONEVALUE)
                return E_REQUEST_DENIED;

        item->selected = flag;
        return E_OK;
}

/*
 * Return the item value of the item.
 */
int
item_value(item)
        ITEM *item;
{
	if (item == NULL)
		return _menui_default_item.selected;
	else
		return item->selected;
}

/*
 * Allocate a new item and return the pointer to the newly allocated
 * structure.
 */
ITEM *
new_item(name, description)
        char *name;
        char *description;
{
        ITEM *new_one;

	  /* allocate a new item structure for ourselves */
        if ((new_one = (ITEM *)malloc(sizeof(ITEM))) == NULL)
                return NULL;

	  /* copy in the defaults for the item */
	(void)memcpy(new_one, &_menui_default_item, sizeof(ITEM));
	
	  /* fill in the name structure - first the length and then
	     allocate room for the string & copy that. */
	new_one->name.length = strlen(name);
        if ((new_one->name.string = (char *)
             malloc(sizeof(char) * new_one->name.length + 1)) == NULL) {
		  /* uh oh malloc failed - clean up & exit */
		free(new_one);
                return NULL;
        }
        
        strcpy(new_one->name.string, name);

	  /* fill in the description structure, stash the length then
	     allocate room for description string and copy it in */
        new_one->description.length = strlen(description);
        if ((new_one->description.string = (char *)
             malloc(sizeof(char) * new_one->description.length + 1)) == NULL) {
		  /* malloc has failed - free up allocated memory and return */
		free(new_one->name.string);
		free(new_one);
		return NULL;
	}
	
	strcpy(new_one->description.string, description);

	return new_one;
}

/*
 * Free the allocated storage associated with item.
 */
int
free_item(item)
	ITEM *item;
{
	if (item == NULL)
		return E_BAD_ARGUMENT;
	
	  /* check for connection to menu */
	if (item->parent != NULL)
		return E_CONNECTED;

	  /* no connections, so free storage starting with the strings */
	free(item->name.string);
	free(item->description.string);
	free(item);
	return E_OK;
}

/*
 * Set the menu's current item to the one given.
 */
int
set_current_item(param_menu, item)
	MENU *param_menu;
	ITEM *item;
{
	MENU *menu = (param_menu != NULL) ? param_menu : &_menui_default_menu;
	int i = 0;

	  /* check if we have been called from an init type function */
	if (menu->in_init == 1)
		return E_BAD_STATE;

	  /* check we have items in the menu */
	if (menu->items == NULL)
		return E_NOT_CONNECTED;

	if ((i = item_index(item)) < 0)
		  /* item must not be a part of this menu */
		return E_BAD_ARGUMENT;
	
	menu->cur_item = i;
	return E_OK;
}

/*
 * Return a pointer to the current item for the menu
 */
ITEM *
current_item(menu)
	MENU *menu;
{
	if (menu == NULL)
		return NULL;
	
	if (menu->items == NULL)
		return NULL;
	
	return menu->items[menu->cur_item];
}

/*
 * Return the index into the item array that matches item.
 */
int
item_index(item)
	ITEM *item;
{
	if (item == NULL)
		return _menui_default_item.index;
	else
		return item->index;
}
