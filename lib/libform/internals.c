/*	$NetBSD: internals.c,v 1.1 2000/12/17 12:04:30 blymn Exp $	*/

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "internals.h"
#include "form.h"

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

static void
_formi_do_validation(FIELD *field, FIELDTYPE *type, int *ret_val);
static int
_formi_join_line(FIELD *field, char *str, unsigned int pos, int direction);
static int
_formi_wrap_field(FIELD *field, unsigned int pos);
static void
_formi_redraw_field(FORM *form, int field);
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
find_cur_line(FIELD *cur);

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
				cur = CIRCLEQ_NEXT(cur, glue);
				i = cur->index;
			} else {
				i++;
				if (i >= form->field_count)
					i = 0;
			}
		} else {
			if (use_sorted == TRUE) {
				cur = CIRCLEQ_PREV(cur, glue);
				i = cur->index;
			} else {
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
find_cur_line(FIELD *cur)
{
	unsigned start, end, pos, row;
	const char *str;

	str = cur->buffers[0].string;
	pos = cur->start_char + cur->hscroll + cur->cursor_xpos;
	
	start = 0;
	end = 0;
	
	for (row = 0; row < cur->row_count; row++) {
		start = _formi_find_bol(str, start);
		end = _formi_find_eol(str, end);
		if ((pos >= start) && (pos <= end))
			return row;
	}

	return 0;
}

	
/*
 * Word wrap the contents of the field's buffer 0 if this is allowed.
 * If the wrap is successful, that is, the row count nor the buffer
 * size is exceeded then the function will return E_OK, otherwise it
 * will return E_REQUEST_DENIED.
 */
static int
_formi_wrap_field(FIELD *field, unsigned int pos)
{
	char *str, *new;
	int width, length, allocated, row_count, sol, eol, wrapped;
	size_t new_size;
	
	if ((field->opts & O_WRAP) != O_WRAP)
		return E_REQUEST_DENIED;

	wrapped = FALSE;
	row_count = 0;
	allocated = field->buffers[0].allocated;
	length = field->buffers[0].length;
	if ((str = (char *) malloc(sizeof(char) * allocated)) == NULL)
		return E_SYSTEM_ERROR;
	
	strcpy(str,field->buffers[0].string);
	
	if ((field->opts & O_STATIC) == O_STATIC)
		width = field->cols;
	else
		width = field->dcols;

	while (str[pos] != '\0') {
		row_count++;
		sol = _formi_find_bol(str, pos);
		eol = _formi_find_eol(str, pos);
		if ((eol - sol) <= width) {
			  /* line may be too short, try joining some lines */
			pos = eol;
			if ((eol - sol) == width) {
				/* if line is just right then don't wrap */
				pos++;
				continue;
			}

			if (_formi_join_line(field, str, pos, JOIN_NEXT_NW)
			    == E_OK) {
				row_count--; /* cuz we just joined a line */
				wrapped = TRUE;
			} else
				break;
		} else {
			  /* line is too long, split it - maybe */
			  /* split on first whitespace before current word */
			pos = sol + width;
			if (!isblank(str[pos]))
				pos = find_sow(str, pos);

			if (pos != sol) {
				if (length + 1 >= allocated) {
					new_size = allocated + 64
						- (allocated % 64);
					
					if ((new = (char *) realloc(str,
								    sizeof(char) * new_size)
					     ) == NULL) {
						free(str);
						return E_SYSTEM_ERROR;
					}
					str = new;
					allocated = new_size;
				}

				bcopy(&str[pos], &str[pos + 1],
				      (unsigned) length - pos - 1);
				str[pos] = '\n';
				pos = pos + 1;
				length++;
				wrapped = TRUE;
			} else
				break;
		}
	}

	if (row_count > field->rows) {
		free(str);
		return E_REQUEST_DENIED;
	}
		
	if (wrapped == TRUE) {
		field->buffers[0].length = length;
		field->buffers[0].allocated = allocated;
		free(field->buffers[0].string);
		field->buffers[0].string = str;
	} else /* all that work was in vain.... */
		free(str);

	return E_OK;
}

/*
 * Join the two lines that surround the location pos, the type
 * variable indicates the direction of the join.  Note that pos is
 * assumed to be at either the end of the line for a JOIN_NEXT or at
 * the beginning of the line for a JOIN_PREV.  We need to check the
 * field options to ensure the join does not overflow the line limit
 * (if wrap is off) or wrap the field buffer again.  Returns E_OK if
 * the join was successful or E_REQUEST_DENIED if the join cannot
 * happen.
 */
static int
_formi_join_line(FIELD *field, char *str, unsigned int pos, int direction)
{
	unsigned int len, sol, eol, npos, start, dest;

	npos = pos;

	if ((direction == JOIN_NEXT) || (direction == JOIN_NEXT_NW)) {
		sol = _formi_find_bol(str, pos);
		npos++;
		  /* see if there is another line following... */
		if (str[npos] == '\0')
			return E_REQUEST_DENIED;
		eol = _formi_find_eol(str, npos);

		start = npos;
		dest = pos;
		len = eol - npos;
	} else {
		if (pos == 0)
			return E_REQUEST_DENIED;
		eol = _formi_find_eol(str, pos);
		npos--;
		sol = _formi_find_bol(str, npos);

		start = pos;
		dest = npos;
		len = eol - pos;
	}
	
	
	  /* if we cannot wrap and the length of the resultant line
	   * is bigger than our field width we shall deny the request.
	   */
	if (((field->opts & O_WRAP) != O_WRAP) && /* XXXXX check for dynamic field */
	    ((sol + eol - 1) > field->cols))
		return E_REQUEST_DENIED;

	bcopy(&str[start], &str[dest], (unsigned) len);

	  /* wrap the field if required, if this fails undo the change */
	if ((direction == JOIN_NEXT) || (direction == JOIN_PREV)) {
		if (_formi_wrap_field(field, (unsigned int) pos) != E_OK) {
			bcopy(&str[dest], &str[start], (unsigned) len);
			str[dest] = '\n';
			return E_REQUEST_DENIED;
		}
	}
	
	return E_OK;
}

/*
 * skip the blanks in the given string, start at the index start and
 * continue forward until either the end of the string or a non-blank
 * character is found.  Return the index of either the end of the string or
 * the first non-blank character.
 */
unsigned
skip_blanks(char *string, unsigned int start)
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
 * Find the next '\n' character in the given string starting at offset
 * if there are no newlines found then return the index to the end of the
 * string.
 */
int
_formi_find_eol(const char *string, unsigned int offset)
{
	char *location;
	int eol;

	if ((location = index(&string[offset], '\n')) != NULL)
		eol  = location - string;
	else
		eol = strlen(string);

	if (eol > 0)
		eol--;

	return eol;
}
		
/*
 * Find the previous '\n' character in the given string starting at offset
 * if there are no newlines found then return 0.
 */
int
_formi_find_bol(const char *string, unsigned int offset)
{
	int cnt;

	cnt = offset;
	while ((cnt > 0) && (string[cnt] != '\n'))
		cnt--;

	  /* if we moved and found a newline go forward one to point at the
	   * actual start of the line....
	   */
	if ((cnt != offset) && (string[cnt] == '\n'))
		cnt++;

	return cnt;
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
	int end, scroll_amt;

	end = _formi_find_eol(field->buffers[0].string,
		       field->start_char + field->hscroll
		       + field->cursor_xpos) - field->start_char
		- field->hscroll - field->cursor_xpos;
	
	scroll_amt = min(amt, end);
	if (scroll_amt < 0)
		scroll_amt = 0;

	field->hscroll += scroll_amt;
	if (amt > field->cursor_xpos)
		field->cursor_xpos = 0;
	else
		field->cursor_xpos -= scroll_amt;
}
	
/*
 * Scroll the field backward the given number of characters.
 */
void
_formi_hscroll_back(FIELD *field, unsigned int amt)
{
	int flen, sa;

	sa = min(field->hscroll, amt);
	field->hscroll -= sa;
	field->cursor_xpos += sa;
	flen = field->cols;
	if (field->start_char > 0)
		flen--;
	if (field->cursor_xpos > flen)
		field->cursor_xpos = flen;
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
static void
_formi_redraw_field(FORM *form, int field)
{
	unsigned int pre, post, flen, slen, i, row, start, end, offset;
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
	end = 0;

	wmove(form->subwin, (int) cur->form_row, (int) cur->form_col);
	for (row = 0; row <= cur->row_count; row++) {
		if ((str[end] == '\0') || (str[end + 1] == '\0') || (row == 0))
			start = end;
		else
			start = end + 1;

		if (cur->buffers[0].length > 0) {
			end = _formi_find_eol(str, start);
			slen = end - start + 1;
		} else
			slen = 0;
		
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
				  /* get padding right if centring is not even */
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

		if (pre > cur->hscroll - start)
			pre = pre - cur->hscroll + start;
		else
			pre = 0;
		
		if (slen > cur->hscroll) {
			slen -= cur->hscroll;
			post += cur->hscroll;
			if (post > flen)
				post = flen;
		} else {
			slen = 0;
			post = flen - pre;
		}

		if (form->cur_field == field)
			wattrset(form->subwin, cur->fore);
		else
			wattrset(form->subwin, cur->back);

#ifdef DEBUG
		fprintf(stderr, "redraw_field: start=%d, pre=%d, slen=%d, flen=%d, post=%d, hscroll=%d\n",
			start, pre, slen, flen, post, cur->hscroll);
		strncpy(buffer, &str[cur->start_char], flen);
		buffer[flen] = '\0';
		fprintf(stderr, "redraw_field: %s\n", buffer);
#endif
		
		for (i = start + cur->hscroll; i < pre; i++)
			waddch(form->subwin, cur->pad);

		offset = cur->hscroll;
		if (cur->start_char > 0)
			offset += cur->start_char - 1;

		if (flen > cur->hscroll + 1)
			flen -= cur->hscroll + 1;
		else
			flen = 0;
		
#ifdef DEBUG
		fprintf(stderr, "redraw_field: will add %d chars, offset is %d\n",
			min(slen, flen), offset);
#endif
		for (i = 0;
		     i < min(slen, flen); i++) 
		{
#ifdef DEBUG
			fprintf(stderr, "adding char str[%d]=%c\n",
				i + offset, str[i + offset]);
#endif
			waddch(form->subwin,
			       ((cur->opts & O_PUBLIC) == O_PUBLIC)?
			       str[i + offset] : cur->pad);
		}

		for (i = 0; i < post; i++)
			waddch(form->subwin, cur->pad);
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

	wclear(form->subwin);
	
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

#ifdef DEBUG
	fprintf(stderr,"add_char: pos=%d, char=%c\n", pos, c);
	fprintf(stderr,"add_char enter: xpos=%d, start=%d, length=%d(%d), allocated=%d\n",
		field->cursor_xpos, field->start_char,
		field->buffers[0].length, strlen(field->buffers[0].string),
		field->buffers[0].allocated);
	fprintf(stderr,"add_char enter: %s\n", field->buffers[0].string);
#endif
	if (((field->opts & O_BLANK) == O_BLANK) &&
	    (field->buf0_status == FALSE)) {
		field->buffers[0].length = 0;
		field->buffers[0].string[0] = '\0';
		pos = 0;
		field->start_char = 0;
		field->start_line = 0;
		field->hscroll = 0;
		field->row_count = 0;
		field->cursor_xpos = 0;
		field->cursor_ypos = 0;
	}


	if ((field->overlay == 0)
	    || ((field->overlay == 1) && (pos >= field->buffers[0].length))) {
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
	    || ((field->overlay == 1) && (pos >= field->buffers[0].length)))
			field->buffers[0].length++;

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
		field->cursor_xpos++;
		if (field->cursor_xpos >= field->cols - 1) {
			field->start_char++;
			field->cursor_xpos = field->cols - 1;
		}
	}
	
#ifdef DEBUG
	fprintf(stderr,"add_char exit: xpos=%d, start=%d, length=%d(%d), allocated=%d\n",
		field->cursor_xpos, field->start_char,
		field->buffers[0].length, strlen(field->buffers[0].string),
		field->buffers[0].allocated);
	fprintf(stderr,"add_char exit: %s\n", field->buffers[0].string);
	fprintf(stderr, "add_char exit: status = %s\n",
		(status == E_OK)? "OK" : "FAILED");
#endif
	return (status == E_OK);
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
	unsigned int i, start, end, pos;

	cur = form->fields[form->cur_field];

#ifdef DEBUG
	fprintf(stderr, "entry: xpos=%d, start_char=%d, length=%d, allocated=%d\n",
		cur->cursor_xpos, cur->start_char, cur->buffers[0].length,
		cur->buffers[0].allocated);
	fprintf(stderr, "entry: string=\"%s\"\n", cur->buffers[0].string);
#endif
	switch (c) {
	case REQ_NEXT_CHAR:
		if ((cur->cursor_xpos + cur->start_char
		     - ((cur->start_char > 0)? 1 : 0) + cur->hscroll + 1)
		    > cur->buffers[0].length) {
			return E_REQUEST_DENIED;
		}
		cur->cursor_xpos++;
		if (cur->cursor_xpos >= cur->cols - cur->hscroll - 1) {
			if (cur->cols < (cur->hscroll  + 1))
				cur->cursor_xpos = 0;
			else
				cur->cursor_xpos = cur->cols
					- cur->hscroll - 1;
			cur->start_char++;
		}
		break;
			
	case REQ_PREV_CHAR:
		if (cur->cursor_xpos == 0) {
			if (cur->start_char > 0)
				cur->start_char--;
			else if (cur->hscroll > 0)
				cur->hscroll--;
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
		
	case REQ_END_FIELD:
		if (cur->row_count > cur->rows) {
			cur->start_line = cur->row_count - cur->rows;
			cur->cursor_ypos = cur->rows - 1;
		} else {
			cur->start_line = 0;
			cur->cursor_ypos = cur->row_count - 1;
		}
		
		if ((str = rindex(cur->buffers[0].string, '\n')) == NULL) {
			cur->cursor_xpos = cur->cols - 1;
			if (cur->start_char < (cur->buffers[0].length +
					       cur->cols)) {
				cur->start_char = 0;
				cur->cursor_xpos = cur->buffers[0].length;
			} else {
				cur->start_char = cur->buffers[0].length -
					cur->cols;
			}
		} else {
			cur->start_char = (str - cur->buffers[0].string);
			if (strlen(str) > cur->cols)
				cur->cursor_xpos = cur->cols;
			else
				cur->cursor_xpos = strlen(str);
		}
		break;
		
	case REQ_BEG_LINE:
		start = cur->start_char + cur->cursor_xpos;
		if (cur->buffers[0].string[start] == '\n') {
			if (start > 0)
				start--;
			else
				return E_REQUEST_DENIED;
		}
		
		while ((start > 0)
		       && (cur->buffers[0].string[start] != '\n'))
			start--;

		if (start > 0)
			start++;

		cur->start_char = start;
		cur->cursor_xpos = 0;
		break;
			
	case REQ_END_LINE:
		start = cur->start_char + cur->cursor_xpos;
		end = _formi_find_eol(cur->buffers[0].string, start);
		start = _formi_find_bol(cur->buffers[0].string, start);

		if (end - start > cur->cols - 1) {
			cur->cursor_xpos = cur->cols - 1;
			cur->start_char = end - cur->cols + 3;
		} else {
			cur->cursor_xpos = end - start + 1;
			cur->start_char = start;
		}
		break;
		
	case REQ_LEFT_CHAR:
		if ((cur->cursor_xpos == 0) && (cur->start_char == 0))
			return E_REQUEST_DENIED;

		if (cur->cursor_xpos == 0) {
			cur->start_char--;
			if (cur->buffers[0].string[cur->start_char] == '\n') {
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

				end = _formi_find_eol(cur->buffers[0].string,
					       cur->start_char);
				start = _formi_find_bol(cur->buffers[0].string,
						 cur->start_char);
				if (end - start >= cur->cols) {
					cur->cursor_xpos = cur->cols - 1;
					cur->start_char = end - cur->cols;
				} else {
					cur->cursor_xpos = end - start;
					cur->start_char = start;
				}
			}
		} else
			cur->cursor_xpos--;
		break;
				    
	case REQ_RIGHT_CHAR:
		pos = cur->start_char + cur->cursor_xpos;
		if (cur->buffers[0].string[pos] == '\0')
			return E_REQUEST_DENIED;

#ifdef DEBUG
		fprintf(stderr, "req_right_char enter: start=%d, xpos=%d, c=%c\n",
			cur->start_char, cur->cursor_xpos,
			cur->buffers[0].string[pos]);
#endif
		
		if (cur->buffers[0].string[pos] == '\n') {
			start = pos + 1;
			if (cur->buffers[0].string[start] == 0)
				return E_REQUEST_DENIED;
			end = _formi_find_eol(cur->buffers[0].string, start);
			if (end - start > cur->cols) {
				cur->cursor_xpos = cur->cols - 1;
				cur->start_char = end - cur->cols - 1;
			} else {
				cur->cursor_xpos = end - start;
				cur->start_char = start;
			}
		} else {
			if (cur->cursor_xpos == cur->cols - 1)
				cur->start_char++;
			else
				cur->cursor_xpos++;
		}
#ifdef DEBUG
		fprintf(stderr, "req_right_char exit: start=%d, xpos=%d, c=%c\n",
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

		start = find_cur_line(cur);
		end = _formi_find_eol(cur->buffers[0].string, start);
		cur->start_char = start;
		if (cur->cursor_xpos > end - start)
			cur->cursor_xpos = end - start;
		break;
		
	case REQ_DOWN_CHAR:
		if (cur->cursor_ypos == cur->rows - 1) {
			if (cur->start_line + cur->rows == cur->row_count)
				return E_REQUEST_DENIED;
			cur->start_line++;
		} else
			cur->cursor_ypos++;
		
		start = find_cur_line(cur);
		end = _formi_find_eol(cur->buffers[0].string, start);
		cur->start_char = start;
		if (cur->cursor_xpos > end - start)
			cur->cursor_xpos = end - start;
		break;
	
	case REQ_NEW_LINE:
		_formi_add_char(cur, cur->start_char + cur->cursor_xpos, '\n');
		cur->row_count++;
		break;
		
	case REQ_INS_CHAR:
		_formi_add_char(cur, cur->start_char + cur->cursor_xpos,
				cur->pad);
		break;
		
	case REQ_INS_LINE:
		start = _formi_find_bol(cur->buffers[0].string, cur->start_char);
		_formi_add_char(cur, start, '\n');
		cur->row_count++;
		break;
		
	case REQ_DEL_CHAR:
		if (cur->buffers[0].string[cur->start_char + cur->cursor_xpos]
		    == '\0')
			return E_REQUEST_DENIED;
		
		start = cur->start_char + cur->cursor_xpos;
		end = cur->buffers[0].length;
		if (cur->buffers[0].string[start] == '\n') {
			if (cur->row_count > 0) {
				cur->row_count--;
				_formi_join_line(cur, cur->buffers[0].string,
						 start, JOIN_NEXT);
			} else
				cur->buffers[0].string[start] = '\0';
		} else {
			bcopy(&cur->buffers[0].string[start + 1],
			      &cur->buffers[0].string[start],
			      (unsigned) end - start + 1);
		}
		
		cur->buffers[0].length--;
		break;
		
	case REQ_DEL_PREV:
		if ((cur->cursor_xpos == 0) && (cur->start_char == 0))
			   return E_REQUEST_DENIED;

		start = cur->cursor_xpos + cur->start_char;
		end = cur->buffers[0].length;
		
		if (cur->buffers[0].string[cur->start_char + cur->cursor_xpos] == '\n') {
			_formi_join_line(cur, cur->buffers[0].string,
					 cur->start_char + cur->cursor_xpos,
					 JOIN_PREV);
			cur->row_count--;
		} else {
			bcopy(&cur->buffers[0].string[start],
			      &cur->buffers[0].string[start - 1],
			      (unsigned) end - start + 1);
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
		start = cur->start_char + cur->cursor_xpos;
		end = _formi_find_eol(cur->buffers[0].string, start);
		start = _formi_find_bol(cur->buffers[0].string, start);
		bcopy(&cur->buffers[0].string[end + 1],
		      &cur->buffers[0].string[start],
		      (unsigned) cur->buffers[0].length - end + 1);
		if (cur->row_count > 0)
			cur->row_count--;
		break;
		
	case REQ_DEL_WORD:
		start = cur->start_char + cur->cursor_xpos;
		end = find_eow(cur->buffers[0].string, start);
		start = find_sow(cur->buffers[0].string, start);
		bcopy(&cur->buffers[0].string[end + 1],
		      &cur->buffers[0].string[start],
		      (unsigned) cur->buffers[0].length - end + 1);
		cur->buffers[0].length -= end - start;
		break;
		
	case REQ_CLR_EOL:
		  /*XXXX this right or should we just toast the chars? */
		start = cur->start_char + cur->cursor_xpos;
		end = _formi_find_eol(cur->buffers[0].string, start);
		for (i = start; i < end; i++)
			cur->buffers[0].string[i] = cur->pad;
		break;

	case REQ_CLR_EOF:
		for (i = cur->start_char + cur->cursor_xpos;
		     i < cur->buffers[0].length; i++)
			cur->buffers[0].string[i] = cur->pad;
		break;
		
	case REQ_CLR_FIELD:
		for (i = 0; i < cur->buffers[0].length; i++)
			cur->buffers[0].string[i] = cur->pad;
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
	fprintf(stderr, "exit: xpos=%d, start_char=%d, length=%d, allocated=%d\n",
		cur->cursor_xpos, cur->start_char, cur->buffers[0].length,
		cur->buffers[0].allocated);
	fprintf(stderr, "exit: string=\"%s\"\n", cur->buffers[0].string);
#endif
	return 1;
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
	int ret_val;
	

	if ((form == NULL) || (form->fields == NULL) ||
	    (form->fields[0] == NULL))
		return E_INVALID_FIELD;

	cur = form->fields[form->cur_field];
	
	if (((form->opts & O_PASSOK) == O_PASSOK) && (cur->buf0_status = 0))
		return E_OK;

	if (((form->opts & O_NULLOK) == O_NULLOK) &&
	    (cur->buffers[0].string[0] == '\0'))
		return E_OK;
	
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
	while (below != CIRCLEQ_FIRST(&form->sorted_fields)) {
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
