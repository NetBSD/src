/*	$NetBSD: internals.c,v 1.15 2001/05/11 14:04:48 blymn Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
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
 *
 *
 */

#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include "internals.h"
#include "form.h"

#ifdef DEBUG
/*
 *  file handle to write debug info to, this will be initialised when
 *  the form is first posted.
 */
FILE *dbg = NULL;
#endif

/* define our own min function - this is not generic but will do here
 * (don't believe me?  think about what value you would get
 * from min(x++, y++)
 */
#define min(a,b) (((a) > (b))? (b) : (a))

/* for the line joining function... */
#define JOIN_NEXT    1
#define JOIN_NEXT_NW 2 /* next join, don't wrap the joined line */
#define JOIN_PREV    3
#define JOIN_PREV_NW 4 /* previous join, don't wrap the joined line */

/* for the bump_lines function... */
#define _FORMI_USE_CURRENT -1 /* indicates current cursor pos to be used */

static void
_formi_do_char_validation(FIELD *field, FIELDTYPE *type, char c, int *ret_val);
static void
_formi_do_validation(FIELD *field, FIELDTYPE *type, int *ret_val);
static int
_formi_join_line(FIELD *field, unsigned int pos, int direction);
void
_formi_hscroll_back(FIELD *field, unsigned int amt);
void
_formi_hscroll_fwd(FIELD *field, unsigned int amt);
static void
_formi_scroll_back(FIELD *field, unsigned int amt);
static void
_formi_scroll_fwd(FIELD *field, unsigned int amt);
static int
find_sow(char *str, unsigned int offset);
static int
find_cur_line(FIELD *cur, unsigned pos);
static int
split_line(FIELD *field, unsigned pos);
static void
bump_lines(FIELD *field, int pos, int amt);


/*
 * Open the debug file if it is not already open....
 */
#ifdef DEBUG
int
_formi_create_dbg_file(void)
{
	if (dbg == NULL) {
		dbg = fopen("___form_dbg.out", "w");
		if (dbg == NULL) {
			fprintf(stderr, "Cannot open debug file!\n");
			return E_SYSTEM_ERROR;
		}
	}

	return E_OK;
}
#endif

/*
 * Bump the lines array elements in the given field by the given amount.
 * The row to start acting on can either be inferred from the given position
 * or if the special value _FORMI_USE_CURRENT is set then the row will be
 * the row the cursor is currently on.
 */
static void
bump_lines(FIELD *field, int pos, int amt)
{
	int i, row;
#ifdef DEBUG
	int dbg_ok = FALSE;
#endif

	if (pos == _FORMI_USE_CURRENT)
		row = field->start_line + field->cursor_ypos;
	else
		row = find_cur_line(field, (unsigned) pos);

#ifdef DEBUG
	if (_formi_create_dbg_file() == E_OK) {
		dbg_ok = TRUE;
		fprintf(dbg, "bump_lines: bump starting at row %d\n", row);
		fprintf(dbg,
			"bump_lines: len from %d to %d, end from %d to %d\n",
			field->lines[row].length,
			field->lines[row].length + amt,
			field->lines[row].end, field->lines[row].end + amt);
	}
#endif
		
	field->lines[row].length += amt;
	if (field->lines[row].length > 1)
		field->lines[row].end += amt;
	for (i = row + 1; i < field->row_count; i++) {
#ifdef DEBUG
		if (dbg_ok) {
			fprintf(dbg,
		"bump_lines: row %d: len from %d to %d, end from %d to %d\n",
				i, field->lines[i].start,
				field->lines[i].start + amt,
				field->lines[i].end,
				field->lines[i].end + amt);
		}
		fflush(dbg);
#endif
		field->lines[i].start += amt;
		field->lines[i].end += amt;
	}
}

/*
 * Set the form's current field to the first valid field on the page.
 * Assume the fields have been sorted and stitched.
 */
int
_formi_pos_first_field(FORM *form)
{
	FIELD *cur;
	int old_page;

	old_page = form->page;

	  /* scan forward for an active page....*/
	while (form->page_starts[form->page].in_use == 0) {
		form->page++;
		if (form->page > form->max_page) {
			form->page = old_page;
			return E_REQUEST_DENIED;
		}
	}

	  /* then scan for a field we can use */
	cur = form->fields[form->page_starts[form->page].first];
	while ((cur->opts & (O_VISIBLE | O_ACTIVE))
	       != (O_VISIBLE | O_ACTIVE)) {
		cur = CIRCLEQ_NEXT(cur, glue);
		if (cur == (void *) &form->sorted_fields) {
			form->page = old_page;
			return E_REQUEST_DENIED;
		}
	}

	form->cur_field = cur->index;
	return E_OK;
}

/*
 * Set the field to the next active and visible field, the fields are
 * traversed in index order in the direction given.  If the parameter
 * use_sorted is TRUE then the sorted field list will be traversed instead
 * of using the field index.
 */
int
_formi_pos_new_field(FORM *form, unsigned direction, unsigned use_sorted)
{
	FIELD *cur;
	int i;

	i = form->cur_field;
	cur = form->fields[i];
	
	do {
		if (direction == _FORMI_FORWARD) {
			if (use_sorted == TRUE) {
				if ((form->wrap == FALSE) &&
				    (cur == CIRCLEQ_LAST(&form->sorted_fields)))
					return E_REQUEST_DENIED;
				cur = CIRCLEQ_NEXT(cur, glue);
				i = cur->index;
			} else {
				if ((form->wrap == FALSE) &&
				    ((i + 1) >= form->field_count))
					return E_REQUEST_DENIED;
				i++;
				if (i >= form->field_count)
					i = 0;
			}
		} else {
			if (use_sorted == TRUE) {
				if ((form->wrap == FALSE) &&
				    (cur == CIRCLEQ_FIRST(&form->sorted_fields)))
					return E_REQUEST_DENIED;
				cur = CIRCLEQ_PREV(cur, glue);
				i = cur->index;
			} else {
				if ((form->wrap == FALSE) && (i <= 0))
					return E_REQUEST_DENIED;
				i--;
				if (i < 0)
					i = form->field_count - 1;
			}
		}
		
		if ((form->fields[i]->opts & (O_VISIBLE | O_ACTIVE))
			== (O_VISIBLE | O_ACTIVE)) {
			form->cur_field = i;
			return E_OK;
		}
	}
	while (i != form->cur_field);

	return E_REQUEST_DENIED;
}

/*
 * Find the line in a field that the cursor is currently on.
 */
static int
find_cur_line(FIELD *cur, unsigned pos)
{
	unsigned row;

	  /* first check if pos is at the end of the string, if this
	   * is true then just return the last row since the pos may
	   * not have been added to the lines array yet.
	   */
	if (pos == (cur->buffers[0].length - 1))
		return (cur->row_count - 1);
	
	for (row = 0; row < cur->row_count; row++) {
		if ((pos >= cur->lines[row].start)
		    && (pos <= cur->lines[row].end))
			return row;
	}

#ifdef DEBUG
	  /* barf if we get here, this should not be possible */
	assert((row != row));
#endif
	return 0;
}

	
/*
 * Word wrap the contents of the field's buffer 0 if this is allowed.
 * If the wrap is successful, that is, the row count nor the buffer
 * size is exceeded then the function will return E_OK, otherwise it
 * will return E_REQUEST_DENIED.
 */
int
_formi_wrap_field(FIELD *field, unsigned int pos)
{
	char *str;
	int width, row;
	
	str = field->buffers[0].string;

	  /* Don't bother if the field string is too short. */
	if (field->buffers[0].length < field->cols)
		return E_OK;
	
	if ((field->opts & O_STATIC) == O_STATIC) {
		if ((field->rows + field->nrows) == 1) {
			return E_OK; /* cannot wrap a single line */
		}
		width = field->cols;
	} else {
		  /* if we are limited to one line then don't try to wrap */
		if ((field->drows + field->nrows) == 1) {
			return E_OK;
		}
		
		  /*
		   * hueristic - if a dynamic field has more than one line
		   * on the screen then the field grows rows, otherwise
		   * it grows columns, effectively a single line field.
		   * This is documented AT&T behaviour.
		   */
		if (field->rows > 1) {
			width = field->cols;
		} else {
			return E_OK;
		}
	}
	
	for (row = 0; row < field->row_count; row++) {
	  AGAIN:
		pos = field->lines[row].end;
		if (field->lines[row].length <= width) {
			  /* line may be too short, try joining some lines */

			if (((((int) field->row_count) - 1) == row) ||
			    (field->lines[row].length == width)) {
				/* if line is just right or this is the last
				 * row then don't wrap
				 */
				continue;
			}

			if (_formi_join_line(field, (unsigned int) pos,
					     JOIN_NEXT_NW) == E_OK) {
				goto AGAIN;
			} else
				break;
		} else {
			  /* line is too long, split it - maybe */
			
			  /* first check if we have not run out of room */
			if ((field->opts & O_STATIC) == O_STATIC) {
				/* check static field */
				if ((field->rows + field->nrows - 1) == row)
					return E_REQUEST_DENIED;
			} else {
				/* check dynamic field */
				if ((field->max != 0)
				    && ((field->max - 1) == row))
					return E_REQUEST_DENIED;
			}
			
			  /* split on first whitespace before current word */
			pos = width + field->lines[row].start;
			if (pos >= field->buffers[0].length)
				pos = field->buffers[0].length - 1;
			
			if ((!isblank(str[pos])) &&
			    ((field->opts & O_WRAP) == O_WRAP)) {
				pos = find_sow(str, (unsigned int) pos);
				if ((pos == 0) || (!isblank(str[pos - 1]))) {
					return E_REQUEST_DENIED;
				}
			}

			if (split_line(field, pos) != E_OK) {
				return E_REQUEST_DENIED;
			}
		}
	}

	return E_OK;
}

/*
 * Join the two lines that surround the location pos, the type
 * variable indicates the direction of the join, JOIN_NEXT will join
 * the next line to the current line, JOIN_PREV will join the current
 * line to the previous line, the new lines will be wrapped unless the
 * _NW versions of the directions are used.  Returns E_OK if the join
 * was successful or E_REQUEST_DENIED if the join cannot happen.
 */
static int
_formi_join_line(FIELD *field, unsigned int pos, int direction)
{
	unsigned int row, i;
	int old_alloced, old_row_count;
	struct _formi_field_lines *saved;

	if ((saved = (struct _formi_field_lines *)
	     malloc(field->row_count * sizeof(struct _formi_field_lines)))
	    == NULL)
		return E_REQUEST_DENIED;

	bcopy(field->lines, saved,
	      field->row_count * sizeof(struct _formi_field_lines));
	old_alloced = field->lines_alloced;
	old_row_count = field->row_count;
	
	row = find_cur_line(field, pos);
	
	if ((direction == JOIN_NEXT) || (direction == JOIN_NEXT_NW)) {
		  /* see if there is another line following... */
		if (row == (field->row_count - 1)) {
			free(saved);
			return E_REQUEST_DENIED;
		}
		field->lines[row].end = field->lines[row + 1].end;
		field->lines[row].length += field->lines[row + 1].length;
		  /* shift all the remaining lines up.... */
		for (i = row + 2; i < field->row_count; i++)
			field->lines[i - 1] = field->lines[i];
	} else {
		if ((pos == 0) || (row == 0)) {
			free(saved);
			return E_REQUEST_DENIED;
		}
		field->lines[row - 1].end = field->lines[row].end;
		field->lines[row - 1].length += field->lines[row].length;
		  /* shift all the remaining lines up */
		for (i = row + 1; i < field->row_count; i++)
			field->lines[i - 1] = field->lines[i];
	}

	field->row_count--;
	
	  /* wrap the field if required, if this fails undo the change */
	if ((direction == JOIN_NEXT) || (direction == JOIN_PREV)) {
		if (_formi_wrap_field(field, (unsigned int) pos) != E_OK) {
			free(field->lines);
			field->lines = saved;
			field->lines_alloced = old_alloced;
			field->row_count = old_row_count;
			return E_REQUEST_DENIED;
		}
	}

	free(saved);
	return E_OK;
}

/*
 * Split the line at the given position, if possible
 */
static int
split_line(FIELD *field, unsigned pos)
{
	struct _formi_field_lines *new_lines;
	unsigned int row, i;
#ifdef DEBUG
	short dbg_ok = FALSE;
#endif

	if (pos == 0)
		return E_REQUEST_DENIED;

#ifdef DEBUG
	if (_formi_create_dbg_file() == E_OK) {
		fprintf(dbg, "split_line: splitting line at %d\n", pos);
		dbg_ok = TRUE;
	}
#endif
	
	if ((field->row_count + 1) > field->lines_alloced) {
		if ((new_lines = (struct _formi_field_lines *)
		     realloc(field->lines, (field->row_count + 1)
			     * sizeof(struct _formi_field_lines))) == NULL)
			return E_SYSTEM_ERROR;
		field->lines = new_lines;
		field->lines_alloced++;
	}
	
	row = find_cur_line(field, pos);
#ifdef DEBUG
	if (dbg_ok == TRUE) {
		fprintf(dbg,
	"split_line: enter: lines[%d].end = %d, lines[%d].length = %d\n",
			row, field->lines[row].end, row,
			field->lines[row].length);
	}
#endif
	
	field->lines[row + 1].end = field->lines[row].end;
	field->lines[row].end = pos - 1;
	field->lines[row].length = pos - field->lines[row].start;
	field->lines[row + 1].start = pos;
	field->lines[row + 1].length = field->lines[row + 1].end
		- field->lines[row + 1].start + 1;

#ifdef DEBUG
	assert(((field->lines[row + 1].end < INT_MAX) &&
		(field->lines[row].end < INT_MAX) &&
		(field->lines[row].length < INT_MAX) &&
		(field->lines[row + 1].start < INT_MAX) &&
		(field->lines[row + 1].length < INT_MAX)));
	
	if (dbg_ok == TRUE) {
		fprintf(dbg,
	"split_line: exit: lines[%d].end = %d, lines[%d].length = %d, ",
			row, field->lines[row].end, row,
			field->lines[row].length);
		fprintf(dbg, "lines[%d].start = %d, lines[%d].end = %d, ",
			row + 1, field->lines[row + 1].start, row + 1,
			field->lines[row + 1].end);
		fprintf(dbg, "lines[%d].length = %d\n", row + 1,
			field->lines[row + 1].length);
	}
#endif
		
	for (i = row + 2; i < field->row_count; i++) {
		field->lines[i + 1] = field->lines[i];
	}

	field->row_count++;
	return E_OK;
}

/*
 * skip the blanks in the given string, start at the index start and
 * continue forward until either the end of the string or a non-blank
 * character is found.  Return the index of either the end of the string or
 * the first non-blank character.
 */
unsigned
_formi_skip_blanks(char *string, unsigned int start)
{
	unsigned int i;

	i = start;

	while ((string[i] != '\0') && isblank(string[i]))
		i++;

	return i;
}

/*
 * Return the index of the top left most field of the two given fields.
 */
static int
_formi_top_left(FORM *form, int a, int b)
{
	  /* lower row numbers always win here.... */
	if (form->fields[a]->form_row < form->fields[b]->form_row)
		return a;

	if (form->fields[a]->form_row > form->fields[b]->form_row)
		return b;

	  /* rows must be equal, check columns */
	if (form->fields[a]->form_col < form->fields[b]->form_col)
		return a;

	if (form->fields[a]->form_col > form->fields[b]->form_col)
		return b;

	  /* if we get here fields must be in exactly the same place, punt */
	return a;
}

/*
 * Return the index to the field that is the bottom-right-most of the
 * two given fields.
 */
static int
_formi_bottom_right(FORM *form, int a, int b)
{
	  /* check the rows first, biggest row wins */
	if (form->fields[a]->form_row > form->fields[b]->form_row)
		return a;
	if (form->fields[a]->form_row < form->fields[b]->form_row)
		return b;

	  /* rows must be equal, check cols, biggest wins */
	if (form->fields[a]->form_col > form->fields[b]->form_col)
		return a;
	if (form->fields[a]->form_col < form->fields[b]->form_col)
		return b;

	  /* fields in the same place, punt */
	return a;
}

/*
 * Find the end of the current word in the string str, starting at
 * offset - the end includes any trailing whitespace.  If the end of
 * the string is found before a new word then just return the offset
 * to the end of the string.
 */
static int
find_eow(char *str, unsigned int offset)
{
	int start;

	start = offset;
	  /* first skip any non-whitespace */
	while ((str[start] != '\0') && !isblank(str[start]))
		start++;

	  /* see if we hit the end of the string */
	if (str[start] == '\0')
		return start;

	  /* otherwise skip the whitespace.... */
	while ((str[start] != '\0') && isblank(str[start]))
		start++;

	return start;
}

/*
 * Find the beginning of the current word in the string str, starting
 * at offset.
 */
static int
find_sow(char *str, unsigned int offset)
{
	int start;

	start = offset;

	if (start > 0) {
		if (isblank(str[start]) || isblank(str[start - 1])) {
			if (isblank(str[start - 1]))
				start--;
			  /* skip the whitespace.... */
			while ((start >= 0) && isblank(str[start]))
				start--;
		}
	}
	
	  /* see if we hit the start of the string */
	if (start < 0)
		return 0;

	  /* now skip any non-whitespace */
	while ((start >= 0) && !isblank(str[start]))
		start--;

	if (start > 0)
		start++; /* last loop has us pointing at a space, adjust */
	
	if (start < 0)
		start = 0;

	return start;
}

/*
 * Scroll the field forward the given number of lines.
 */
static void
_formi_scroll_fwd(FIELD *field, unsigned int amt)
{
	  /* check if we have lines to scroll */
	if (field->row_count < (field->start_line + field->rows))
		return;

	field->start_line += min(amt,
				 field->row_count - field->start_line
				 - field->rows);
}

/*
 * Scroll the field backward the given number of lines.
 */
static void
_formi_scroll_back(FIELD *field, unsigned int amt)
{
	if (field->start_line == 0)
		return;

	field->start_line -= min(field->start_line, amt);
}

/*
 * Scroll the field forward the given number of characters.
 */
void
_formi_hscroll_fwd(FIELD *field, int unsigned amt)
{
	field->start_char += min(amt,
		field->lines[field->start_line + field->cursor_ypos].end);
}
	
/*
 * Scroll the field backward the given number of characters.
 */
void
_formi_hscroll_back(FIELD *field, unsigned int amt)
{
	field->start_char -= min(field->start_char, amt);
}
	
/*
 * Find the different pages in the form fields and assign the form
 * page_starts array with the information to find them.
 */
int
_formi_find_pages(FORM *form)
{
	int i, cur_page = 0;

	if ((form->page_starts = (_FORMI_PAGE_START *)
	     malloc((form->max_page + 1) * sizeof(_FORMI_PAGE_START))) == NULL)
		return E_SYSTEM_ERROR;

	  /* initialise the page starts array */
	memset(form->page_starts, 0,
	       (form->max_page + 1) * sizeof(_FORMI_PAGE_START));
	
	for (i =0; i < form->field_count; i++) {
		if (form->fields[i]->page_break == 1)
			cur_page++;
		if (form->page_starts[cur_page].in_use == 0) {
			form->page_starts[cur_page].in_use = 1;
			form->page_starts[cur_page].first = i;
			form->page_starts[cur_page].last = i;
			form->page_starts[cur_page].top_left = i;
			form->page_starts[cur_page].bottom_right = i;
		} else {
			form->page_starts[cur_page].last = i;
			form->page_starts[cur_page].top_left =
				_formi_top_left(form,
						form->page_starts[cur_page].top_left,
						i);
			form->page_starts[cur_page].bottom_right =
				_formi_bottom_right(form,
						    form->page_starts[cur_page].bottom_right,
						    i);
		}
	}
	
	return E_OK;
}

/*
 * Completely redraw the field of the given form.
 */
void
_formi_redraw_field(FORM *form, int field)
{
	unsigned int pre, post, flen, slen, i, row, start;
	char *str;
	FIELD *cur;
#ifdef DEBUG
	char buffer[100];
#endif

	cur = form->fields[field];
	str = cur->buffers[0].string;
	flen = cur->cols;
	slen = 0;
	start = 0;

	for (row = 0; row < cur->row_count; row++) {
		wmove(form->scrwin, (int) (cur->form_row + row),
		      (int) cur->form_col);
		start = cur->lines[row].start;
		slen = cur->lines[row].length;
		
		if ((cur->opts & O_STATIC) == O_STATIC) {
			switch (cur->justification) {
			case JUSTIFY_RIGHT:
				post = 0;
				if (flen < slen)
					pre = 0;
				else
					pre = flen - slen;
				break;

			case JUSTIFY_CENTER:
				if (flen < slen) {
					pre = 0;
					post = 0;
				} else {
					pre = flen - slen;
					post = pre = pre / 2;
					  /* get padding right if
                                             centring is not even */
					if ((post + pre + slen) < flen)
						post++;
				}
				break;

			case NO_JUSTIFICATION:
			case JUSTIFY_LEFT:
			default:
				pre = 0;
				if (flen <= slen)
					post = 0;
				else {
					post = flen - slen;
					if (post > flen)
						post = flen;
				}
				break;
			}
		} else {
			  /* dynamic fields are not justified */
			pre = 0;
			if (flen <= slen)
				post = 0;
			else {
				post = flen - slen;
				if (post > flen)
					post = flen;
			}

			  /* but they do scroll.... */
			
			if (pre > cur->start_char - start)
				pre = pre - cur->start_char + start;
			else
				pre = 0;
		
			if (slen > cur->start_char) {
				slen -= cur->start_char;
				post += cur->start_char;
				if (post > flen)
					post = flen;
			} else {
				slen = 0;
				post = flen - pre;
			}
		}
			
		if (form->cur_field == field)
			wattrset(form->scrwin, cur->fore);
		else
			wattrset(form->scrwin, cur->back);

#ifdef DEBUG
		if (_formi_create_dbg_file() == E_OK) {
			fprintf(dbg,
  "redraw_field: start=%d, pre=%d, slen=%d, flen=%d, post=%d, start_char=%d\n",
				start, pre, slen, flen, post, cur->start_char);
			if (str != NULL) {
				strncpy(buffer,
					&str[cur->start_char
					    + cur->lines[row].start], flen);
			} else {
				strcpy(buffer, "(null)");
			}
			buffer[flen] = '\0';
			fprintf(dbg, "redraw_field: %s\n", buffer);
		}
#endif
		
		for (i = start + cur->start_char; i < pre; i++)
			waddch(form->scrwin, cur->pad);

#ifdef DEBUG
		fprintf(dbg, "redraw_field: will add %d chars\n",
			min(slen, flen));
#endif
		for (i = 0; i < min(slen, flen); i++) 
		{
#ifdef DEBUG
			fprintf(dbg, "adding char str[%d]=%c\n",
				i + cur->start_char + cur->lines[row].start,
				str[i + cur->start_char
				   + cur->lines[row].start]);
#endif
			if (((cur->opts & O_PUBLIC) != O_PUBLIC)) {
				waddch(form->scrwin, cur->pad);
			} else if ((cur->opts & O_VISIBLE) == O_VISIBLE) {
				waddch(form->scrwin, str[i + cur->start_char
				+ cur->lines[row].start]);
			} else {
				waddch(form->scrwin, ' ');
			}
		}

		for (i = 0; i < post; i++)
			waddch(form->scrwin, cur->pad);
	}
	
	return;
}

/*
 * Display the fields attached to the form that are on the current page
 * on the screen.
 *
 */
int
_formi_draw_page(FORM *form)
{
	int i;
	
	if (form->page_starts[form->page].in_use == 0)
		return E_BAD_ARGUMENT;

	wclear(form->scrwin);
	
	for (i = form->page_starts[form->page].first;
	     i <= form->page_starts[form->page].last; i++)
		_formi_redraw_field(form, i);
	
	return E_OK;
}

/*
 * Add the character c at the position pos in buffer 0 of the given field
 */
int
_formi_add_char(FIELD *field, unsigned int pos, char c)
{
	char *new;
	unsigned int new_size;
	int status;

	  /*
	   * If buffer has not had a string before, set it to a blank
	   * string.  Everything should flow from there....
	   */
	if (field->buffers[0].string == NULL) {
		set_field_buffer(field, 0, "");
	}

	if (_formi_validate_char(field, c) != E_OK) {
#ifdef DEBUG
		fprintf(dbg, "add_char: char %c failed char validation\n", c);
#endif
		return E_INVALID_FIELD;
	}
	
#ifdef DEBUG
	fprintf(dbg, "add_char: pos=%d, char=%c\n", pos, c);
	fprintf(dbg,
	   "add_char enter: xpos=%d, start=%d, length=%d(%d), allocated=%d\n",
		field->cursor_xpos, field->start_char,
		field->buffers[0].length, strlen(field->buffers[0].string),
		field->buffers[0].allocated);
	fprintf(dbg, "add_char enter: %s\n", field->buffers[0].string);
	fprintf(dbg, "add_char enter: buf0_status=%d\n", field->buf0_status);
#endif
	if (((field->opts & O_BLANK) == O_BLANK) &&
	    (field->buf0_status == FALSE) &&
	    ((field->cursor_xpos + field->start_char) == 0)) {
		field->buffers[0].length = 0;
		field->buffers[0].string[0] = '\0';
		pos = 0;
		field->start_char = 0;
		field->start_line = 0;
		field->row_count = 1;
		field->cursor_xpos = 0;
		field->cursor_ypos = 0;
		field->lines[0].start = 0;
		field->lines[0].end = 0;
		field->lines[0].length = 0;
	}


	if ((field->overlay == 0)
	    || ((field->overlay == 1) && (pos >= field->buffers[0].length))) {
		  /* first check if the field can have more chars...*/
		if ((((field->opts & O_STATIC) == O_STATIC) &&
		     (field->buffers[0].length >= (field->cols * field->rows))) ||
		    (((field->opts & O_STATIC) != O_STATIC) &&
/*XXXXX this is wrong - should check max row or col */
		     ((field->max > 0) &&
		      (field->buffers[0].length >= field->max))))
			return E_REQUEST_DENIED;
		
		if (field->buffers[0].length + 1
		    >= field->buffers[0].allocated) {
			new_size = field->buffers[0].allocated + 64
				- (field->buffers[0].allocated % 64);
			if ((new = (char *) realloc(field->buffers[0].string,
						    new_size )) == NULL)
				return E_SYSTEM_ERROR;
			field->buffers[0].allocated = new_size;
			field->buffers[0].string = new;
		}
	}
	    
	if ((field->overlay == 0) && (field->buffers[0].length > pos)) {
		bcopy(&field->buffers[0].string[pos],
		      &field->buffers[0].string[pos + 1],
		      field->buffers[0].length - pos + 1);
	}
	
	field->buffers[0].string[pos] = c;
	if (pos >= field->buffers[0].length) {
		  /* make sure the string is terminated if we are at the
		   * end of the string, the terminator would be missing
		   * if we are are at the end of the field.
		   */
		field->buffers[0].string[pos + 1] = '\0';
	}

	  /* only increment the length if we are inserting characters
	   * OR if we are at the end of the field in overlay mode.
	   */
	if ((field->overlay == 0)
	    || ((field->overlay == 1) && (pos >= field->buffers[0].length))) {
		field->buffers[0].length++;
		bump_lines(field, (int) pos, 1);
	}
	

	  /* wrap the field, if needed */
	status = _formi_wrap_field(field, pos);
	if (status != E_OK) {
		  /* wrap failed for some reason, back out the char insert */
		bcopy(&field->buffers[0].string[pos + 1],
		      &field->buffers[0].string[pos],
		      field->buffers[0].length - pos);
		field->buffers[0].length--;
	} else {
		field->buf0_status = TRUE;
		if ((field->rows + field->nrows) == 1) {
			if ((field->cursor_xpos < (field->cols - 1)) ||
			    ((field->opts & O_STATIC) != O_STATIC))
				field->cursor_xpos++;
		
			if (field->cursor_xpos > field->cols) {
				field->start_char++;
				field->cursor_xpos = field->cols;
			}
		} else {
			field->cursor_ypos = find_cur_line(field, pos)
				- field->start_line;
			field->cursor_xpos = pos
				- field->lines[field->cursor_ypos
					      + field->start_line].start + 1;
		}
	}
	
#ifdef DEBUG
	fprintf(dbg,
	    "add_char exit: xpos=%d, start=%d, length=%d(%d), allocated=%d\n",
		field->cursor_xpos, field->start_char,
		field->buffers[0].length, strlen(field->buffers[0].string),
		field->buffers[0].allocated);
	fprintf(dbg,"add_char exit: %s\n", field->buffers[0].string);
	fprintf(dbg, "add_char exit: buf0_status=%d\n", field->buf0_status);
	fprintf(dbg, "add_char exit: status = %s\n",
		(status == E_OK)? "OK" : "FAILED");
#endif
	return status;
}

/*
 * Manipulate the text in a field, this takes the given form and performs
 * the passed driver command on the current text field.  Returns 1 if the
 * text field was modified.
 */
int
_formi_manipulate_field(FORM *form, int c)
{
	FIELD *cur;
	char *str;
	unsigned int i, start, end, pos, row, len, status;

	cur = form->fields[form->cur_field];

#ifdef DEBUG
	fprintf(dbg,
		"entry: xpos=%d, start_char=%d, length=%d, allocated=%d\n",
		cur->cursor_xpos, cur->start_char, cur->buffers[0].length,
		cur->buffers[0].allocated);
	fprintf(dbg, "entry: string=");
	if (cur->buffers[0].string == NULL)
		fprintf(dbg, "(null)\n");
	else
		fprintf(dbg, "\"%s\"\n", cur->buffers[0].string);
#endif

	  /* Cannot manipulate a null string! */
	if (cur->buffers[0].string == NULL)
		return E_REQUEST_DENIED;
	
	switch (c) {
	case REQ_NEXT_CHAR:
		  /* for a dynamic field allow an offset of one more
		   * char so we can insert chars after end of string.
		   * Static fields cannot do this so deny request if
		   * cursor is at the end of the field.
		   */
		if (((cur->opts & O_STATIC) == O_STATIC) &&
		    (cur->cursor_xpos == cur->cols - 1))
			return E_REQUEST_DENIED;
								
		if ((cur->cursor_xpos + cur->start_char + 1)
		    > cur->buffers[0].length)
			return E_REQUEST_DENIED;

		cur->cursor_xpos++;
		if (cur->cursor_xpos >= cur->cols - 1) {
			cur->cursor_xpos = cur->cols - 1;
			if ((cur->opts & O_STATIC) != O_STATIC)
				cur->start_char++;
		}
		break;
			
	case REQ_PREV_CHAR:
		if (cur->cursor_xpos == 0) {
			if (cur->start_char > 0)
				cur->start_char--;
			else
				return E_REQUEST_DENIED;
		} else
			cur->cursor_xpos--;
		break;
		
	case REQ_NEXT_LINE:
		cur->cursor_ypos++;
		if (cur->cursor_ypos > cur->rows) {
			if ((cur->opts & O_STATIC) == O_STATIC) {
				if (cur->start_line + cur->cursor_ypos
				    > cur->drows) {
					cur->cursor_ypos--;
					return E_REQUEST_DENIED;
				}
			} else {
				if (cur->start_line + cur->cursor_ypos
				    > cur->nrows + cur->rows) {
					cur->cursor_ypos--;
					return E_REQUEST_DENIED;
				}
			}
			cur->start_line++;
		}
		break;
		
	case REQ_PREV_LINE:
		if (cur->cursor_ypos == 0) {
			if (cur->start_line == 0)
				return E_REQUEST_DENIED;
			cur->start_line--;
		} else
			cur->cursor_ypos--;
		break;
		
	case REQ_NEXT_WORD:
		start = cur->start_char + cur->cursor_xpos;
		str = cur->buffers[0].string;

		start = find_eow(str, start);
		
		  /* check if we hit the end */
		if (str[start] == '\0')
			return E_REQUEST_DENIED;

		  /* otherwise we must have found the start of a word...*/
		if (start - cur->start_char < cur->cols) {
			cur->cursor_xpos = start;
		} else {
			cur->start_char = start;
			cur->cursor_xpos = 0;
		}
		break;
		
	case REQ_PREV_WORD:
		start = cur->start_char + cur->cursor_xpos;
		if (cur->start_char > 0)
			start--;
		
		if (start == 0)
			return E_REQUEST_DENIED;
		
		str = cur->buffers[0].string;

		start = find_sow(str, start);
		
		if (start - cur->start_char > 0) {
			cur->cursor_xpos = start;
		} else {
			cur->start_char = start;
			cur->cursor_xpos = 0;
		}
		break;
		
	case REQ_BEG_FIELD:
		cur->start_char = 0;
		cur->start_line = 0;
		cur->cursor_xpos = 0;
		cur->cursor_ypos = 0;
		break;
		
	case REQ_BEG_LINE:
		cur->start_char =
			cur->lines[cur->start_line + cur->cursor_ypos].start;
		cur->cursor_xpos = 0;
		break;
			
	case REQ_END_FIELD:
		if (cur->row_count > cur->rows) {
			cur->start_line = cur->row_count - cur->rows;
			cur->cursor_ypos = cur->rows - 1;
		} else {
			cur->start_line = 0;
			cur->cursor_ypos = cur->row_count - 1;
		}

		cur->start_char =
			cur->lines[cur->start_line + cur->cursor_ypos].start;
		cur->cursor_xpos = 0;
		  /* we fall through here deliberately, we are on the
		   * correct row, now we need to get to the end of the
		   * line.
		   */
		  /* FALLTHRU */
		
	case REQ_END_LINE:
		start = cur->lines[cur->start_line + cur->cursor_ypos].start;
		end = cur->lines[cur->start_line + cur->cursor_ypos].end;

		if (end - start > cur->cols - 1) {
			cur->cursor_xpos = cur->cols - 1;
			cur->start_char = end - cur->cols;
			if ((cur->opts & O_STATIC) != O_STATIC)
				cur->start_char++;
		} else {
			cur->cursor_xpos = end - start + 1;
			if (((cur->opts & O_STATIC) == O_STATIC) &&
			    ((end - start) == (cur->cols - 1)))
				cur->cursor_xpos--;
				
			cur->start_char = start;
		}
		break;
		
	case REQ_LEFT_CHAR:
		if ((cur->cursor_xpos == 0) && (cur->start_char == 0))
			return E_REQUEST_DENIED;

		if (cur->cursor_xpos == 0) {
			cur->start_char--;
			start = cur->lines[cur->cursor_ypos
					  + cur->start_line].start;
			if (cur->start_char < start) {
				if ((cur->cursor_ypos == 0) &&
				    (cur->start_line == 0))
				{
					cur->start_char++;
					return E_REQUEST_DENIED;
				}

				if (cur->cursor_ypos == 0)
					cur->start_line--;
				else
					cur->cursor_ypos--;

				end = cur->lines[cur->cursor_ypos
						+ cur->start_line].end;
				len = cur->lines[cur->cursor_ypos
						+ cur->start_line].length;
				if (len >= cur->cols) {
					cur->cursor_xpos = cur->cols - 1;
					cur->start_char = end - cur->cols;
				} else {
					cur->cursor_xpos = len;
					cur->start_char = start;
				}
			}
		} else
			cur->cursor_xpos--;
		break;
				    
	case REQ_RIGHT_CHAR:
		pos = cur->start_char + cur->cursor_xpos;
		row = cur->start_line + cur->cursor_ypos;
		end = cur->lines[row].end;
		
		if (cur->buffers[0].string[pos] == '\0')
			return E_REQUEST_DENIED;

#ifdef DEBUG
		fprintf(dbg, "req_right_char enter: start=%d, xpos=%d, c=%c\n",
			cur->start_char, cur->cursor_xpos,
			cur->buffers[0].string[pos]);
#endif
		
		if (pos == end) {
			start = pos + 1;
			if ((cur->buffers[0].length <= start)
			    || ((row + 1) >= cur->row_count))
				return E_REQUEST_DENIED;

			if ((cur->cursor_ypos + 1) >= cur->rows) {
				cur->start_line++;
				cur->cursor_ypos = cur->rows - 1;
			} else
				cur->cursor_ypos++;

			row++;
			
			cur->cursor_xpos = 0;
			cur->start_char = cur->lines[row].start;
		} else {
			if (cur->cursor_xpos == cur->cols - 1)
				cur->start_char++;
			else
				cur->cursor_xpos++;
		}
#ifdef DEBUG
		fprintf(dbg, "req_right_char exit: start=%d, xpos=%d, c=%c\n",
			cur->start_char, cur->cursor_xpos,
			cur->buffers[0].string[cur->start_char +
					      cur->cursor_xpos]);
#endif
		break;
		
	case REQ_UP_CHAR:
		if (cur->cursor_ypos == 0) {
			if (cur->start_line == 0)
				return E_REQUEST_DENIED;

			cur->start_line--;
		} else
			cur->cursor_ypos--;

		row = cur->start_line + cur->cursor_ypos;
		
		cur->start_char = cur->lines[row].start;
		if (cur->cursor_xpos > cur->lines[row].length)
			cur->cursor_xpos = cur->lines[row].length;
		break;
		
	case REQ_DOWN_CHAR:
		if (cur->cursor_ypos == cur->rows - 1) {
			if (cur->start_line + cur->rows == cur->row_count)
				return E_REQUEST_DENIED;
			cur->start_line++;
		} else
			cur->cursor_ypos++;
		
		row = cur->start_line + cur->cursor_ypos;
		cur->start_char = cur->lines[row].start;
		if (cur->cursor_xpos > cur->lines[row].length)
			cur->cursor_xpos = cur->lines[row].length;
		break;
	
	case REQ_NEW_LINE:
		if ((status = split_line(cur,
				cur->start_char + cur->cursor_xpos)) != E_OK)
			return status;
		break;
		
	case REQ_INS_CHAR:
		_formi_add_char(cur, cur->start_char + cur->cursor_xpos,
				cur->pad);
		break;
		
	case REQ_INS_LINE:
		start = cur->lines[cur->start_line + cur->cursor_ypos].start;
		if ((status = split_line(cur, start)) != E_OK)
			return status;
		break;
		
	case REQ_DEL_CHAR:
		if (cur->buffers[0].length == 0)
			return E_REQUEST_DENIED;

		row = cur->start_line + cur->cursor_ypos;
		start = cur->start_char + cur->cursor_xpos;
		end = cur->buffers[0].length;
		if (start == cur->lines[row].start) {
			if (cur->row_count > 1) {
				if (_formi_join_line(cur,
						start, JOIN_NEXT) != E_OK) {
					return E_REQUEST_DENIED;
				}
			} else {
				cur->buffers[0].string[start] = '\0';
				if (cur->lines[row].end > 0) {
					cur->lines[row].end--;
					cur->lines[row].length--;
				}
			}
		} else {
			bcopy(&cur->buffers[0].string[start + 1],
			      &cur->buffers[0].string[start],
			      (unsigned) end - start + 1);
			bump_lines(cur, _FORMI_USE_CURRENT, -1);
		}
		
		cur->buffers[0].length--;
		break;
		
	case REQ_DEL_PREV:
		if ((cur->cursor_xpos == 0) && (cur->start_char == 0))
			   return E_REQUEST_DENIED;

		start = cur->cursor_xpos + cur->start_char;
		end = cur->buffers[0].length;
		row = cur->start_line + cur->cursor_ypos;
		
		if (cur->lines[row].start ==
		    (cur->start_char + cur->cursor_xpos)) {
			_formi_join_line(cur,
					 cur->start_char + cur->cursor_xpos,
					 JOIN_PREV);
		} else {
			bcopy(&cur->buffers[0].string[start],
			      &cur->buffers[0].string[start - 1],
			      (unsigned) end - start + 1);
			bump_lines(cur, _FORMI_USE_CURRENT, -1);
		}

		cur->buffers[0].length--;
		if ((cur->cursor_xpos == 0) && (cur->start_char > 0))
			cur->start_char--;
		else if ((cur->cursor_xpos == cur->cols - 1)
			 && (cur->start_char > 0))
			cur->start_char--;
		else if (cur->cursor_xpos > 0)
			cur->cursor_xpos--;

		break;
		
	case REQ_DEL_LINE:
		row = cur->start_line + cur->cursor_ypos;
		start = cur->lines[row].start;
		end = cur->lines[row].end;
		bcopy(&cur->buffers[0].string[end + 1],
		      &cur->buffers[0].string[start],
		      (unsigned) cur->buffers[0].length - end + 1);
		if (cur->row_count > 1) {
			cur->row_count--;
			bcopy(&cur->lines[row + 1], &cur->lines[row],
			      sizeof(struct _formi_field_lines)
			      * cur->row_count);

			len = end - start;
			for (i = row; i < cur->row_count; i++) {
				cur->lines[i].start -= len;
				cur->lines[i].end -= len;
			}
			
			if (row > cur->row_count) {
				if (cur->cursor_ypos == 0) {
					if (cur->start_line > 0) {
						cur->start_line--;
					}
				} else {
					cur->cursor_ypos--;
				}
				row--;
			}
			
			if (cur->cursor_xpos > cur->lines[row].length)
				cur->cursor_xpos = cur->lines[row].length;
		}
		break;
		
	case REQ_DEL_WORD:
		start = cur->start_char + cur->cursor_xpos;
		end = find_eow(cur->buffers[0].string, start);
		start = find_sow(cur->buffers[0].string, start);
		bcopy(&cur->buffers[0].string[end + 1],
		      &cur->buffers[0].string[start],
		      (unsigned) cur->buffers[0].length - end + 1);
		len = end - start;
		cur->buffers[0].length -= len;
		bump_lines(cur, _FORMI_USE_CURRENT, - (int) len);
		
		if (cur->cursor_xpos > cur->lines[row].length)
			cur->cursor_xpos = cur->lines[row].length;
		break;
		
	case REQ_CLR_EOL:
		row = cur->start_line + cur->cursor_ypos;
		start = cur->start_char + cur->cursor_xpos;
		end = cur->lines[row].end;
		len = end - start;
		bcopy(&cur->buffers[0].string[end + 1],
		      &cur->buffers[0].string[start],
		      cur->buffers[0].length - end + 1);
		cur->buffers[0].length -= len;
		bump_lines(cur, _FORMI_USE_CURRENT, - (int) len);
		
		if (cur->cursor_xpos > cur->lines[row].length)
			cur->cursor_xpos = cur->lines[row].length;
		break;

	case REQ_CLR_EOF:
		row = cur->start_line + cur->cursor_ypos;
		cur->buffers[0].string[cur->start_char
				      + cur->cursor_xpos] = '\0';
		cur->buffers[0].length = strlen(cur->buffers[0].string);
		cur->lines[row].end = cur->buffers[0].length;
		cur->lines[row].length = cur->lines[row].end
			- cur->lines[row].start;
		
		for (i = cur->start_char + cur->cursor_xpos;
		     i < cur->buffers[0].length; i++)
			cur->buffers[0].string[i] = cur->pad;
		break;
		
	case REQ_CLR_FIELD:
		cur->buffers[0].string[0] = '\0';
		cur->buffers[0].length = 0;
		cur->row_count = 1;
		cur->start_line = 0;
		cur->cursor_ypos = 0;
		cur->cursor_xpos = 0;
		cur->start_char = 0;
		cur->lines[0].start = 0;
		cur->lines[0].end = 0;
		cur->lines[0].length = 0;
		break;
		
	case REQ_OVL_MODE:
		cur->overlay = 1;
		break;
		
	case REQ_INS_MODE:
		cur->overlay = 0;
		break;
		
	case REQ_SCR_FLINE:
		_formi_scroll_fwd(cur, 1);
		break;
		
	case REQ_SCR_BLINE:
		_formi_scroll_back(cur, 1);
		break;
		
	case REQ_SCR_FPAGE:
		_formi_scroll_fwd(cur, cur->rows);
		break;
		
	case REQ_SCR_BPAGE:
		_formi_scroll_back(cur, cur->rows);
		break;
		
	case REQ_SCR_FHPAGE:
		_formi_scroll_fwd(cur, cur->rows / 2);
		break;
		
	case REQ_SCR_BHPAGE:
		_formi_scroll_back(cur, cur->rows / 2);
		break;
		
	case REQ_SCR_FCHAR:
		_formi_hscroll_fwd(cur, 1);
		break;
		
	case REQ_SCR_BCHAR:
		_formi_hscroll_back(cur, 1);
		break;
		
	case REQ_SCR_HFLINE:
		_formi_hscroll_fwd(cur, cur->cols);
		break;
		
	case REQ_SCR_HBLINE:
		_formi_hscroll_back(cur, cur->cols);
		break;
		
	case REQ_SCR_HFHALF:
		_formi_hscroll_fwd(cur, cur->cols / 2);
		break;
		
	case REQ_SCR_HBHALF:
		_formi_hscroll_back(cur, cur->cols / 2);
		break;

	default:
		return 0;
	}
	
#ifdef DEBUG
	fprintf(dbg, "exit: xpos=%d, start_char=%d, length=%d, allocated=%d\n",
		cur->cursor_xpos, cur->start_char, cur->buffers[0].length,
		cur->buffers[0].allocated);
	fprintf(dbg, "exit: string=\"%s\"\n", cur->buffers[0].string);
#endif
	return 1;
}

/*
 * Validate the give character by passing it to any type character
 * checking routines, if they exist.
 */
int
_formi_validate_char(FIELD *field, char c)
{
	int ret_val;
	
	if (field->type == NULL)
		return E_OK;

	ret_val = E_INVALID_FIELD;
	_formi_do_char_validation(field, field->type, c, &ret_val);

	return ret_val;
}

		
/*
 * Perform the validation of the character, invoke all field_type validation
 * routines.  If the field is ok then update ret_val to E_OK otherwise
 * ret_val is not changed.
 */
static void
_formi_do_char_validation(FIELD *field, FIELDTYPE *type, char c, int *ret_val)
{
	if ((type->flags & _TYPE_IS_LINKED) == _TYPE_IS_LINKED) {
		_formi_do_char_validation(field, type->link->next, c, ret_val);
		_formi_do_char_validation(field, type->link->prev, c, ret_val);
	} else {
		if (type->char_check == NULL)
			*ret_val = E_OK;
		else {
			if (type->char_check((int)(unsigned char) c,
					     field->args) == TRUE)
				*ret_val = E_OK;
		}
	}
}

/*
 * Validate the current field.  If the field validation returns success then
 * return E_OK otherwise return E_INVALID_FIELD.
 *
 */
int
_formi_validate_field(FORM *form)
{
	FIELD *cur;
	char *bp;
	int ret_val, count;
	

	if ((form == NULL) || (form->fields == NULL) ||
	    (form->fields[0] == NULL))
		return E_INVALID_FIELD;

	cur = form->fields[form->cur_field];
	
	bp = cur->buffers[0].string;
	count = _formi_skip_blanks(bp, 0);

	  /* check if we have a null field, depending on the nullok flag
	   * this may be acceptable or not....
	   */
	if (cur->buffers[0].string[count] == '\0') {
		if ((cur->opts & O_NULLOK) == O_NULLOK)
			return E_OK;
		else
			return E_INVALID_FIELD;
	}
	
	  /* check if an unmodified field is ok */
	if (cur->buf0_status == 0) {
		if ((cur->opts & O_PASSOK) == O_PASSOK)
			return E_OK;
		else
			return E_INVALID_FIELD;
	}

	  /* if there is no type then just accept the field */
	if (cur->type == NULL)
		return E_OK;

	ret_val = E_INVALID_FIELD;
	_formi_do_validation(cur, cur->type, &ret_val);
	
	return ret_val;
}

/*
 * Perform the validation of the field, invoke all field_type validation
 * routines.  If the field is ok then update ret_val to E_OK otherwise
 * ret_val is not changed.
 */
static void
_formi_do_validation(FIELD *field, FIELDTYPE *type, int *ret_val)
{
	if ((type->flags & _TYPE_IS_LINKED) == _TYPE_IS_LINKED) {
		_formi_do_validation(field, type->link->next, ret_val);
		_formi_do_validation(field, type->link->prev, ret_val);
	} else {
		if (type->field_check == NULL)
			*ret_val = E_OK;
		else {
			if (type->field_check(field, field_buffer(field, 0))
			    == TRUE)
				*ret_val = E_OK;
		}
	}
}

/*
 * Select the next/previous choice for the field, the driver command
 * selecting the direction will be passed in c.  Return 1 if a choice
 * selection succeeded, 0 otherwise.
 */
int
_formi_field_choice(FORM *form, int c)
{
	FIELDTYPE *type;
	FIELD *field;
	
	if ((form == NULL) || (form->fields == NULL) ||
	    (form->fields[0] == NULL) ||
	    (form->fields[form->cur_field]->type == NULL))
		return 0;
	
	field = form->fields[form->cur_field];
	type = field->type;
	
	switch (c) {
	case REQ_NEXT_CHOICE:
		if (type->next_choice == NULL)
			return 0;
		else
			return type->next_choice(field,
						 field_buffer(field, 0));

	case REQ_PREV_CHOICE:
		if (type->prev_choice == NULL)
			return 0;
		else
			return type->prev_choice(field,
						 field_buffer(field, 0));

	default: /* should never happen! */
		return 0;
	}
}

/*
 * Update the fields if they have changed.  The parameter old has the
 * previous current field as the current field may have been updated by
 * the driver.  Return 1 if the form page needs updating.
 *
 */
int
_formi_update_field(FORM *form, int old_field)
{
	int cur, i;

	cur = form->cur_field;
	
	if (old_field != cur) {
		if (!((cur >= form->page_starts[form->page].first) &&
		      (cur <= form->page_starts[form->page].last))) {
			  /* not on same page any more */
			for (i = 0; i < form->max_page; i++) {
				if ((form->page_starts[i].in_use == 1) &&
				    (form->page_starts[i].first <= cur) &&
				    (form->page_starts[i].last >= cur)) {
					form->page = i;
					return 1;
				}
			}
		}
	}

	_formi_redraw_field(form, old_field);
	_formi_redraw_field(form, form->cur_field);
	return 0;
}

/*
 * Compare function for the field sorting
 *
 */
static int
field_sort_compare(const void *one, const void *two)
{
	const FIELD *a, *b;
	int tl;
	
	  /* LINTED const castaway; we don't modify these! */	
	a = (const FIELD *) *((const FIELD **) one);
	b = (const FIELD *) *((const FIELD **) two);

	if (a == NULL)
		return 1;

	if (b == NULL)
		return -1;
	
	  /*
	   * First check the page, we want the fields sorted by page.
	   *
	   */
	if (a->page != b->page)
		return ((a->page > b->page)? 1 : -1);

	tl = _formi_top_left(a->parent, a->index, b->index);

	  /*
	   * sort fields left to right, top to bottom so the top left is
	   * the less than value....
	   */
	return ((tl == a->index)? -1 : 1);
}
	
/*
 * Sort the fields in a form ready for driver traversal.
 */
void
_formi_sort_fields(FORM *form)
{
	FIELD **sort_area;
	int i;
	
	CIRCLEQ_INIT(&form->sorted_fields);

	if ((sort_area = (FIELD **) malloc(sizeof(FIELD *) * form->field_count))
	    == NULL)
		return;

	bcopy(form->fields, sort_area, sizeof(FIELD *) * form->field_count);
	qsort(sort_area, (unsigned) form->field_count, sizeof(FIELD *),
	      field_sort_compare);
	
	for (i = 0; i < form->field_count; i++)
		CIRCLEQ_INSERT_TAIL(&form->sorted_fields, sort_area[i], glue);

	free(sort_area);
}

/*
 * Set the neighbours for all the fields in the given form.
 */
void
_formi_stitch_fields(FORM *form)
{
	int above_row, below_row, end_above, end_below, cur_row, real_end;
	FIELD *cur, *above, *below;

	  /*
	   * check if the sorted fields circle queue is empty, just
	   * return if it is.
	   */
	if (CIRCLEQ_EMPTY(&form->sorted_fields))
		return;
	
	  /* initially nothing is above..... */
	above_row = -1;
	end_above = TRUE;
	above = NULL;

	  /* set up the first field as the current... */
	cur = CIRCLEQ_FIRST(&form->sorted_fields);
	cur_row = cur->form_row;
	
	  /* find the first field on the next row if any */
	below = CIRCLEQ_NEXT(cur, glue);
	below_row = -1;
	end_below = TRUE;
	real_end = TRUE;
	while (below != (void *)&form->sorted_fields) {
		if (below->form_row != cur_row) {
			below_row = below->form_row;
			end_below = FALSE;
			real_end = FALSE;
			break;
		}
		below = CIRCLEQ_NEXT(below, glue);
	}

	  /* walk the sorted fields, setting the neighbour pointers */
	while (cur != (void *) &form->sorted_fields) {
		if (cur == CIRCLEQ_FIRST(&form->sorted_fields))
			cur->left = NULL;
		else
			cur->left = CIRCLEQ_PREV(cur, glue);

		if (cur == CIRCLEQ_LAST(&form->sorted_fields))
			cur->right = NULL;
		else
			cur->right = CIRCLEQ_NEXT(cur, glue);

		if (end_above == TRUE)
			cur->up = NULL;
		else {
			cur->up = above;
			above = CIRCLEQ_NEXT(above, glue);
			if (above_row != above->form_row) {
				end_above = TRUE;
				above_row = above->form_row;
			}
		}

		if (end_below == TRUE)
			cur->down = NULL;
		else {
			cur->down = below;
			below = CIRCLEQ_NEXT(below, glue);
			if (below == (void *) &form->sorted_fields) {
				end_below = TRUE;
				real_end = TRUE;
			} else if (below_row != below->form_row) {
				end_below = TRUE;
				below_row = below->form_row;
			}
		}

		cur = CIRCLEQ_NEXT(cur, glue);
		if ((cur != (void *) &form->sorted_fields)
		    && (cur_row != cur->form_row)) {
			cur_row = cur->form_row;
			if (end_above == FALSE) {
				for (; above != CIRCLEQ_FIRST(&form->sorted_fields);
				     above = CIRCLEQ_NEXT(above, glue)) {
					if (above->form_row != above_row) {
						above_row = above->form_row;
						break;
					}
				}
			} else if (above == NULL) {
				above = CIRCLEQ_FIRST(&form->sorted_fields);
				end_above = FALSE;
				above_row = above->form_row;
			} else
				end_above = FALSE;

			if (end_below == FALSE) {
				while (below_row == below->form_row) {
					below = CIRCLEQ_NEXT(below,
							     glue);
					if (below ==
					    (void *)&form->sorted_fields) {
						real_end = TRUE;
						end_below = TRUE;
						break;
					}
				}

				if (below != (void *)&form->sorted_fields)
					below_row = below->form_row;
			} else if (real_end == FALSE)
				end_below = FALSE;
			
		}
	}
}
