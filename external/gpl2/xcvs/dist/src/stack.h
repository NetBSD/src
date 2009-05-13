/*
 * Copyright (c) 2004-2005 The Free Software Foundation,
 *                         Derek Price, and Ximbiot <http://ximbiot.com>.
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 */

void push (List *_stack, void *_elem);
void *pop (List *_stack);
void unshift (List *_stack, void *_elem);
void *shift (List *_stack);
void push_string (List *_stack, char *_elem);
char *pop_string (List *_stack);
void unshift_string (List *_stack, char *_elem);
char *shift_string (List *_stack);
int isempty (List *_stack);
