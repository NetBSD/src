/*	$NetBSD: internals.c,v 1.23 2002/05/20 15:00:11 blymn Exp $	*/

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
 *    derived from this software without specific prior written permission
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

/*
 * map the request numbers to strings for debug
 */
char *reqs[] = {
	"NEXT_PAGE", "PREV_PAGE", "FIRST_PAGE",	"LAST_PAGE", "NEXT_FIELD",
	"PREV_FIELD", "FIRST_FIELD", "LAST_FIELD", "SNEXT_FIELD",
	"SPREV_FIELD", "SFIRST_FIELD", "SLAST_FIELD", "LEFT_FIELD",
	"RIGHT_FIELD", "UP_FIELD", "DOWN_FIELD", "NEXT_CHAR", "PREV_CHAR",
	"NEXT_LINE", "PREV_LINE", "NEXT_WORD", "PREV_WORD", "BEG_FIELD",
	"END_FIELD", "BEG_LINE", "END_LINE", "LEFT_CHAR", "RIGHT_CHAR",
	"UP_CHAR", "DOWN_CHAR", "NEW_LINE", "INS_CHAR", "INS_LINE",
	"DEL_CHAR", "DEL_PREV", "DEL_LINE", "DEL_WORD", "CLR_EOL",
	"CLR_EOF", "CLR_FIELD", "OVL_MODE", "INS_MODE", "SCR_FLINE",
	"SCR_BLINE", "SCR_FPAGE", "SCR_BPAGE", "SCR_FHPAGE", "SCR_BHPAGE",
	"SCR_FCHAR", "SCR_BCHAR", "SCR_HFLINE", "SCR_HBLINE", "SCR_HFHALF",
	"SCR_HBHALF", "VALIDATION", "PREV_CHOICE", "NEXT_CHOICE" };
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
bump_lines(FIELD *field, int pos, int amt, bool do_len);
static bool
check_field_size(FIELD *field);
static int
add_tab(FORM *form, FIELD *field, unsigned row, unsigned int i, char c);
static int
tab_size(FIELD *field, unsigned int offset, unsigned int i);
static unsigned int
tab_fit_len(FIELD *field, unsigned int row, unsigned int len);
static int
tab_fit_window(FIELD *field, unsigned int pos, unsigned int window);


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
bump_lines(FIELD *field, int pos, int amt, bool do_len)
{
	int i, row, old_len;
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
		
	if (((int)field->lines[row].length + amt) < 0) {
		field->lines[row].length = 0;
		old_len = 0;
	} else {
		old_len = field->lines[row].length;
		if (do_len == TRUE) {
			field->lines[row].length =
				_formi_tab_expanded_length(
					&field->buffers[0].string[
						field->lines[row].start], 0,
					field->lines[row].end + amt
					    -field->lines[row].start);
		}
	}
	
	if (old_len > 0) {
		if ((amt < 0) && (- amt > field->lines[row].end))
			field->lines[row].end = field->lines[row].start;
		else
			field->lines[row].end += amt;
	} else
		field->lines[row].end = field->lines[row].start;

#ifdef DEBUG
	if (dbg_ok)
		fprintf(dbg, "bump_lines: expanded length %d\n",
			field->lines[row].length);
#endif
	
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
		field->lines[i].length = _formi_tab_expanded_length(
			&field->buffers[0].string[field->lines[i].start],
			0, field->lines[i].end - field->lines[i].start);
#ifdef DEBUG
		if (dbg_ok) {
			fprintf(dbg,
				"bump_lines: row %d, expanded length %d\n",
				i, field->lines[i].length);
		}
#endif
	}
}

/*
 * Check the sizing of the field, if the maximum size is set for a
 * dynamic field then check that the number of rows or columns does
 * not exceed the set maximum.  The decision to check the rows or
 * columns is made on the basis of how many rows are in the field -
 * one row means the max applies to the number of columns otherwise it
 * applies to the number of rows.  If the row/column count is less
 * than the maximum then return TRUE.
 *
 */
static bool
check_field_size(FIELD *field)
{
	if ((field->opts & O_STATIC) != O_STATIC) {
		  /* dynamic field */
		if (field->max == 0) /* unlimited */
			return TRUE;

		if (field->rows == 1) {
			return (field->buffers[0].length < field->max);
		} else {
			return (field->row_count <= field->max);
		}
	} else {
		if ((field->rows + field->nrows) == 1) {
			return (field->buffers[0].length <= field->cols);
		} else {
			return (field->row_count <= (field->rows
						     + field->nrows));
		}
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

	  /* if there is only one row then that must be it */
	if (cur->row_count == 1)
		return 0;
	
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
_formi_wrap_field(FIELD *field, unsigned int loc)
{
	char *str;
	int width, row, start_row;
	unsigned int pos;
	
	str = field->buffers[0].string;

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

	start_row = find_cur_line(field, loc);
	
	  /* if we are not at the top of the field then back up one
	   * row because we may be able to merge the current row into
	   * the one above.
	   */
	if (start_row > 0)
		start_row--;
	
	for (row = start_row; row < field->row_count; row++) {
	  AGAIN:
		pos = field->lines[row].end;
		if (field->lines[row].length < width) {
			  /* line may be too short, try joining some lines */

			if ((((int) field->row_count) - 1) == row) {
				/* if this is the last row then don't
				 * wrap
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
			
			  /*
			   * split on first whitespace before current word
			   * if the line has tabs we need to work out where
			   * the field border lies when the tabs are expanded.
			   */
			if (field->lines[row].tabs == NULL) {
				pos = width + field->lines[row].start - 1;
				if (pos >= field->buffers[0].length)
					pos = field->buffers[0].length - 1;
			} else {
				pos = tab_fit_len(field, (unsigned) row,
						  field->cols);
			}
			
			
			if ((!isblank(str[pos])) &&
			    ((field->opts & O_WRAP) == O_WRAP)) {
				if (!isblank(str[pos - 1]))
					pos = find_sow(str,
						       (unsigned int) pos);
				/*
				 * If we cannot split the line then return
				 * NO_ROOM so the driver can tell that it
				 * should not autoskip (if that is enabled)
				 */
				if ((pos == 0) || (!isblank(str[pos - 1]))
				    || ((pos <= field->lines[row].start)
					&& (field->buffers[0].length
					    >= (width - 1
						+ field->lines[row].start)))) {
					return E_NO_ROOM;
				}
			}

			  /* if we are at the end of the string and it has
			   * a trailing blank, don't wrap the blank.
			   */
			if ((pos == field->buffers[0].length - 1) &&
			    (isblank(str[pos])) &&
			    field->lines[row].length <= field->cols)
				continue;

			  /*
			   * otherwise, if we are still sitting on a
			   * blank but not at the end of the line
			   * move forward one char so the blank
			   * is on the line boundary.
			   */
			if ((isblank(str[pos])) &&
			    (pos != field->buffers[0].length - 1))
				pos++;
			
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
#ifdef DEBUG
	int dbg_ok = FALSE;

	if (_formi_create_dbg_file() == E_OK) {
		dbg_ok = TRUE;
	}
#endif
	
	if ((saved = (struct _formi_field_lines *)
	     malloc(field->lines_alloced * sizeof(struct _formi_field_lines)))
	    == NULL)
		return E_REQUEST_DENIED;

	bcopy(field->lines, saved,
	      field->row_count * sizeof(struct _formi_field_lines));
	old_alloced = field->lines_alloced;
	old_row_count = field->row_count;
	
	row = find_cur_line(field, pos);

#ifdef DEBUG
	if (dbg_ok == TRUE) {
		fprintf(dbg, "join_line: working on row %d, row_count = %d\n",
			row, field->row_count);
	}
#endif
	
	if ((direction == JOIN_NEXT) || (direction == JOIN_NEXT_NW)) {
		  /* see if there is another line following... */
		if (row == (field->row_count - 1)) {
			free(saved);
			return E_REQUEST_DENIED;
		}

#ifdef DEBUG
		if (dbg_ok == TRUE) {
			fprintf(dbg,
			"join_line: join_next before end = %d, length = %d",
				field->lines[row].end,
				field->lines[row].length);
			fprintf(dbg,
				" :: next row end = %d, length = %d\n",
				field->lines[row + 1].end,
				field->lines[row + 1].length);
		}
#endif
		
		field->lines[row].end = field->lines[row + 1].end;
		field->lines[row].length =
			_formi_tab_expanded_length(field->buffers[0].string,
					    field->lines[row].start,
					    field->lines[row].end);
		_formi_calculate_tabs(field, row);
		
		  /* shift all the remaining lines up.... */
		for (i = row + 2; i < field->row_count; i++)
			field->lines[i - 1] = field->lines[i];
	} else {
		if ((pos == 0) || (row == 0)) {
			free(saved);
			return E_REQUEST_DENIED;
		}
		
#ifdef DEBUG
		if (dbg_ok == TRUE) {
			fprintf(dbg,
			"join_line: join_prev before end = %d, length = %d",
				field->lines[row].end,
				field->lines[row].length);
			fprintf(dbg,
				" :: prev row end = %d, length = %d\n",
				field->lines[row - 1].end,
				field->lines[row - 1].length);
		}
#endif
		
		field->lines[row - 1].end = field->lines[row].end;
		field->lines[row - 1].length =
			_formi_tab_expanded_length(field->buffers[0].string,
					    field->lines[row - 1].start,
					    field->lines[row].end);
		  /* shift all the remaining lines up */
		for (i = row + 1; i < field->row_count; i++)
			field->lines[i - 1] = field->lines[i];
	}

#ifdef DEBUG
	if (dbg_ok == TRUE) {
		fprintf(dbg,
			"join_line: exit end = %d, length = %d\n",
			field->lines[row].end, field->lines[row].length);
	}
#endif
	
	field->row_count--;
	
	  /* wrap the field if required, if this fails undo the change */
	if ((direction == JOIN_NEXT) || (direction == JOIN_PREV)) {
		if (_formi_wrap_field(field, (unsigned int) pos) != E_OK) {
			free(field->lines);
			field->lines = saved;
			field->lines_alloced = old_alloced;
			field->row_count = old_row_count;
			for (i = 0; i < field->row_count; i++)
				_formi_calculate_tabs(field, i);
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

	assert(((field->lines[row].end < INT_MAX) &&
		(field->lines[row].length < INT_MAX) &&
		(field->lines[row].length > 0)));
	
#endif

	  /* if asked to split right where the line already starts then
	   * just return - nothing to do.
	   */
	if (field->lines[row].start == pos)
		return E_OK;
	
	for (i = field->row_count - 1; i > row; i--) {
		field->lines[i + 1] = field->lines[i];
	}

	field->lines[row + 1].end = field->lines[row].end;
	field->lines[row].end = pos - 1;
	field->lines[row].length =
		_formi_tab_expanded_length(field->buffers[0].string,
				    field->lines[row].start,
				    field->lines[row].end);
	_formi_calculate_tabs(field, row);
	field->lines[row + 1].start = pos;
	field->lines[row + 1].length =
		_formi_tab_expanded_length(field->buffers[0].string,
				    field->lines[row + 1].start,
				    field->lines[row + 1].end);
	field->lines[row + 1].tabs = NULL;
	_formi_calculate_tabs(field, row + 1);

#ifdef DEBUG
	assert(((field->lines[row + 1].end < INT_MAX) &&
		(field->lines[row].end < INT_MAX) &&
		(field->lines[row].length < INT_MAX) &&
		(field->lines[row + 1].start < INT_MAX) &&
		(field->lines[row + 1].length < INT_MAX) &&
		(field->lines[row].length > 0) &&
		(field->lines[row + 1].length > 0)));
	
	if (dbg_ok == TRUE) {
		fprintf(dbg,
	"split_line: exit: lines[%d].end = %d, lines[%d].length = %d, ",
			row, field->lines[row].end, row,
			field->lines[row].length);
		fprintf(dbg, "lines[%d].start = %d, lines[%d].end = %d, ",
			row + 1, field->lines[row + 1].start, row + 1,
			field->lines[row + 1].end);
		fprintf(dbg, "lines[%d].length = %d, row_count = %d\n",
			row + 1, field->lines[row + 1].length,
			field->row_count + 1);
	}
#endif
		
	field->row_count++;
	
#ifdef DEBUG
	if (dbg_ok == TRUE) {
		bump_lines(field, 0, 0, FALSE); /* will report line data for us */
	}
#endif
	
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
	unsigned int row, end, scroll_amt, expanded;
	_formi_tab_t *ts;
	
	row = field->start_line + field->cursor_ypos;

	if ((field->lines[row].tabs == NULL)
	    || (field->lines[row].tabs->in_use == FALSE)) {
		  /* if the line has no tabs things are easy... */
		end =  field->start_char + field->cols + amt - 1;
		scroll_amt = amt;
		if (end > field->lines[row].end) {
			end = field->lines[row].end;
			scroll_amt = end - field->start_char - field->cols + 1;
		}
	} else {
		  /*
		   * If there are tabs we need to add on the scroll amount,
		   * find the last char position that will fit into
		   * the field and finally fix up the start_char.  This
		   * is a lot of work but handling the case where there
		   * are not enough chars to scroll by amt is difficult.
		   */
		end = field->start_char + field->row_xpos + amt;
		if (end >= field->buffers[0].length)
			end = field->buffers[0].length - 1;
		else {
			expanded = _formi_tab_expanded_length(
				field->buffers[0].string,
				field->start_char + amt,
				field->start_char + field->row_xpos + amt);
			ts = field->lines[0].tabs;
			  /* skip tabs to the lhs of our starting point */
			while ((ts != NULL) && (ts->in_use == TRUE)
			       && (ts->pos < end))
				ts = ts->fwd;
			
			while ((expanded <= field->cols)
			       && (end < field->buffers[0].length)) {
				if (field->buffers[0].string[end] == '\t') {
#ifdef DEBUG
					assert((ts != NULL)
					       && (ts->in_use == TRUE));
#endif
					if (ts->pos == end) {
						if ((expanded + ts->size)
						    > field->cols)
							break;
						expanded += ts->size;
						ts = ts->fwd;
					}
#ifdef DEBUG
					else
						assert(ts->pos == end);
#endif
				} else
					expanded++;
				end++;
			}
		}

		scroll_amt = tab_fit_window(field, end, field->cols);
		if (scroll_amt < field->start_char)
			scroll_amt = 1;
		else
			scroll_amt -= field->start_char;

		scroll_amt = min(scroll_amt, amt);
	}

	field->start_char += scroll_amt;
	field->cursor_xpos =
		_formi_tab_expanded_length(field->buffers[0].string,
					   field->start_char,
					   field->row_xpos
					   + field->start_char) - 1;
	
}
	
/*
 * Scroll the field backward the given number of characters.
 */
void
_formi_hscroll_back(FIELD *field, unsigned int amt)
{
	field->start_char -= min(field->start_char, amt);
	field->cursor_xpos =
		_formi_tab_expanded_length(field->buffers[0].string,
					   field->start_char,
					   field->row_xpos
					   + field->start_char) - 1;
	if (field->cursor_xpos >= field->cols) {
		field->row_xpos = 0;
		field->cursor_xpos = 0;
	}
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
	unsigned int last_row, tab, cpos, len;
	char *str, c;
	FIELD *cur;
#ifdef DEBUG
	char buffer[100];
#endif

	cur = form->fields[field];
	str = cur->buffers[0].string;
	flen = cur->cols;
	slen = 0;
	start = 0;

	if ((cur->row_count - cur->start_line) < cur->rows)
		last_row = cur->row_count;
	else
		last_row = cur->start_line + cur->rows;
		
	for (row = cur->start_line; row < last_row; row++) {
		wmove(form->scrwin,
		      (int) (cur->form_row + row - cur->start_line),
		      (int) cur->form_col);
		start = cur->lines[row].start;
		if ((cur->rows + cur->nrows) == 1) {
			if ((cur->start_char + cur->cols)
			    >= cur->buffers[0].length)
				len = cur->buffers[0].length;
			else
				len = cur->cols + cur->start_char;
			slen = _formi_tab_expanded_length(
				cur->buffers[0].string, cur->start_char, len);
			if (slen > cur->cols)
				slen = cur->cols;
			slen += cur->start_char;
		} else
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
				if (slen > flen)
					post = 0;
				else
					post = flen - slen;
				
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
		str = &cur->buffers[0].string[cur->start_char
					     + cur->lines[row].start];
		
		for (i = 0, cpos = cur->start_char; i < min(slen, flen);
		     i++, str++, cpos++) 
		{
			c = *str;
			tab = 0; /* just to shut gcc up */
#ifdef DEBUG
			fprintf(dbg, "adding char str[%d]=%c\n",
				cpos + cur->start_char + cur->lines[row].start,
				c);
#endif
			if (((cur->opts & O_PUBLIC) != O_PUBLIC)) {
				if (c == '\t')
					tab = add_tab(form, cur, row,
						      cpos, cur->pad);
				else
					waddch(form->scrwin, cur->pad);
			} else if ((cur->opts & O_VISIBLE) == O_VISIBLE) {
				if (c == '\t')
					tab = add_tab(form, cur, row, cpos,
						      ' ');
				else
					waddch(form->scrwin, c);
			} else {
				if (c == '\t')
					tab = add_tab(form, cur, row, cpos,
						      ' ');
				else
					waddch(form->scrwin, ' ');
			}

			  /*
			   * If we have had a tab then skip forward
			   * the requisite number of chars to keep
			   * things in sync.
			   */
			if (c == '\t')
				i += tab - 1;
		}

		for (i = 0; i < post; i++)
			waddch(form->scrwin, cur->pad);
	}

	for (row = cur->row_count - cur->start_line; row < cur->rows; row++) {
		wmove(form->scrwin, (int) (cur->form_row + row),
		      (int) cur->form_col);
		for (i = 0; i < cur->cols; i++) {
			waddch(form->scrwin, cur->pad);
		}
	}

	return;
}

/*
 * Add the correct number of the given character to simulate a tab
 * in the field.
 */
static int
add_tab(FORM *form, FIELD *field, unsigned row, unsigned int i, char c)
{
	int j;
	_formi_tab_t *ts = field->lines[row].tabs;

	while ((ts != NULL) && (ts->pos != i))
		ts = ts->fwd;

#ifdef DEBUG
	assert(ts != NULL);
#endif
	
	for (j = 0; j < ts->size; j++)
		waddch(form->scrwin, c);

	return ts->size;
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
	char *new, old_c;
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

	if ((c == '\t') && (field->cols <= 8)) {
#ifdef DEBUG
		fprintf(dbg, "add_char: field too small for a tab\n");
#endif
		return E_NO_ROOM;
	}
	
#ifdef DEBUG
	fprintf(dbg, "add_char: pos=%d, char=%c\n", pos, c);
	fprintf(dbg, "add_char enter: xpos=%d, row_pos=%d, start=%d\n",
		field->cursor_xpos, field->row_xpos, field->start_char);
	fprintf(dbg, "add_char enter: length=%d(%d), allocated=%d\n",
		field->buffers[0].length, strlen(field->buffers[0].string),
		field->buffers[0].allocated);
	fprintf(dbg, "add_char enter: %s\n", field->buffers[0].string);
	fprintf(dbg, "add_char enter: buf0_status=%d\n", field->buf0_status);
#endif
	if (((field->opts & O_BLANK) == O_BLANK) &&
	    (field->buf0_status == FALSE) &&
	    ((field->row_xpos + field->start_char) == 0)) {
		field->buffers[0].length = 0;
		field->buffers[0].string[0] = '\0';
		pos = 0;
		field->start_char = 0;
		field->start_line = 0;
		field->row_count = 1;
		field->row_xpos = 0;
		field->cursor_xpos = 0;
		field->cursor_ypos = 0;
		field->lines[0].start = 0;
		field->lines[0].end = 0;
		field->lines[0].length = 0;
	}


	if ((field->overlay == 0)
	    || ((field->overlay == 1) && (pos >= field->buffers[0].length))) {
		  /* first check if the field can have more chars...*/
		if (check_field_size(field) == FALSE)
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

	old_c = field->buffers[0].string[pos];
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
		bump_lines(field, (int) pos, 1, TRUE);
	} else if (field->overlay == 1)
		bump_lines(field, (int) pos, 0, TRUE);

	new_size = find_cur_line(field, pos);
	_formi_calculate_tabs(field, new_size);
	
	  /* wrap the field, if needed */
	status = _formi_wrap_field(field, pos);

	  /* just in case the row we are on wrapped */
	new_size = find_cur_line(field, pos);
	
	  /*
	   * check the wrap worked or that we have not exceeded the
	   * max field size - this can happen if the field is re-wrapped
	   * and the row count is increased past the set limit.
	   */
	if ((status != E_OK) || (check_field_size(field) == FALSE)) {
		if ((field->overlay == 0)
		    || ((field->overlay == 1)
			&& (pos >= field->buffers[0].length))) {
			  /*
			   * wrap failed for some reason, back out the
			   * char insert
			   */
			bcopy(&field->buffers[0].string[pos + 1],
			      &field->buffers[0].string[pos],
			      field->buffers[0].length - pos);
			field->buffers[0].length--;
			bump_lines(field, (int) pos, -1, TRUE);
			if (pos > 0)
				pos--;
		} else if (field->overlay == 1) {
			  /* back out character overlay */
			field->buffers[0].string[pos] = old_c;
		}
		
		new_size = find_cur_line(field, pos);
		_formi_calculate_tabs(field, new_size);
		
		_formi_wrap_field(field, pos);
		  /*
		   * If we are here then either the status is bad or we
		   * simply ran out of room.  If the status is E_OK then
		   * we ran out of room, let the form driver know this.
		   */
		if (status == E_OK)
			status = E_REQUEST_DENIED;
		
	} else {
		field->buf0_status = TRUE;
		if ((field->rows + field->nrows) == 1) {
			if ((field->cursor_xpos < (field->cols - 1)) ||
			    ((field->opts & O_STATIC) != O_STATIC)) {
				field->row_xpos++;
				field->cursor_xpos =
					_formi_tab_expanded_length(
						field->buffers[0].string,
						field->start_char,
						field->row_xpos
						+ field->start_char);
				if ((field->start_char + field->row_xpos)
				    < field->buffers[0].length)
					field->cursor_xpos--;
			}
			
			if (field->cursor_xpos > (field->cols - 1)) {
				field->start_char =
					tab_fit_window(field,
						       field->start_char + field->row_xpos,
						       field->cols);
				field->row_xpos = pos - field->start_char + 1;
				field->cursor_xpos =
					_formi_tab_expanded_length(
						field->buffers[0].string,
						field->start_char,
						field->row_xpos
						+ field->start_char - 1);
			}
		} else {
			if (new_size >= field->rows) {
				field->cursor_ypos = field->rows - 1;
				field->start_line = field->row_count
					- field->cursor_ypos - 1;
			} else
				field->cursor_ypos = new_size;

			if ((field->lines[new_size].start) <= (pos + 1)) {
				field->row_xpos = pos
					- field->lines[new_size].start + 1;
				field->cursor_xpos =
					_formi_tab_expanded_length(
						&field->buffers[0].string[
						    field->lines[new_size].start], 0,
						field->row_xpos - 1);
			} else {
				field->row_xpos = 0;
				field->cursor_xpos = 0;
			}
			
			  /*
			   * Annoying corner case - if we are right in
			   * the bottom right corner of the field we
			   * need to scroll the field one line so the
			   * cursor is positioned correctly in the
			   * field.
			   */
			if ((field->cursor_xpos >= field->cols) &&
			    (field->cursor_ypos == (field->rows - 1))) {
				field->cursor_ypos--;
				field->start_line++;
			}
		}
	}
	
#ifdef DEBUG
	assert((field->cursor_xpos <= field->cols)
	       && (field->cursor_ypos < 400000)
	       && (field->start_line < 400000));
	       
	fprintf(dbg, "add_char exit: xpos=%d, row_pos=%d, start=%d\n",
		field->cursor_xpos, field->row_xpos, field->start_char);
	fprintf(dbg, "add_char_exit: length=%d(%d), allocated=%d\n",
		field->buffers[0].length, strlen(field->buffers[0].string),
		field->buffers[0].allocated);
	fprintf(dbg, "add_char exit: ypos=%d, start_line=%d\n",
		field->cursor_ypos, field->start_line);
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
	char *str, saved;
	unsigned int i, start, end, pos, row, status, old_count, size;
	unsigned int old_xpos, old_row_pos;
	int len;
	
	cur = form->fields[form->cur_field];

#ifdef DEBUG
	fprintf(dbg, "entry: request is REQ_%s\n", reqs[c - REQ_MIN_REQUEST]);
	fprintf(dbg,
	"entry: xpos=%d, row_pos=%d, start_char=%d, length=%d, allocated=%d\n",
		cur->cursor_xpos, cur->row_xpos, cur->start_char,
		cur->buffers[0].length,	cur->buffers[0].allocated);
	fprintf(dbg, "entry: start_line=%d, ypos=%d\n", cur->start_line,
		cur->cursor_ypos);
	fprintf(dbg, "entry: string=");
	if (cur->buffers[0].string == NULL)
		fprintf(dbg, "(null)\n");
	else
		fprintf(dbg, "\"%s\"\n", cur->buffers[0].string);
#endif

	  /* Cannot manipulate a null string! */
	if (cur->buffers[0].string == NULL)
		return E_REQUEST_DENIED;

	row = cur->start_line + cur->cursor_ypos;
	saved = cur->buffers[0].string[cur->start_char + cur->row_xpos
				      + cur->lines[row].start];
	
	switch (c) {
	case REQ_RIGHT_CHAR:
		  /*
		   * The right_char request performs the same function
		   * as the next_char request except that the cursor is
		   * not wrapped if it is at the end of the line, so
		   * check if the cursor is at the end of the line and
		   * deny the request otherwise just fall through to
		   * the next_char request handler.
		   */
		if (cur->cursor_xpos >= cur->lines[row].length - 1)
			return E_REQUEST_DENIED;

		  /* FALLTHRU */
		
	case REQ_NEXT_CHAR:
		  /* for a dynamic field allow an offset of one more
		   * char so we can insert chars after end of string.
		   * Static fields cannot do this so deny request if
		   * cursor is at the end of the field.
		   */
		if (((cur->opts & O_STATIC) == O_STATIC) &&
		    (cur->row_xpos == cur->cols - 1) &&
		    ((cur->rows + cur->nrows) == 1))
			return E_REQUEST_DENIED;
								
		if ((cur->row_xpos + cur->start_char + 1)
		    > cur->buffers[0].length)
			return E_REQUEST_DENIED;

		if ((cur->rows + cur->nrows) == 1) {
			if (saved == '\t')
				cur->cursor_xpos += tab_size(cur, 0,
							     cur->row_xpos
							    + cur->start_char);
			else
				cur->cursor_xpos++;
			cur->row_xpos++;
			if (cur->cursor_xpos >= cur->cols - 1) {
				pos = cur->row_xpos + cur->start_char;
				cur->start_char =
					tab_fit_window(cur,
						       cur->start_char + cur->row_xpos,
						       cur->cols);
				cur->row_xpos = pos - cur->start_char;
				cur->cursor_xpos =
					_formi_tab_expanded_length(
						cur->buffers[0].string,
						cur->start_char,
						cur->row_xpos
						+ cur->start_char);
				if ((cur->row_xpos + cur->start_char) <
				    cur->buffers[0].length)
					cur->cursor_xpos--;
			}
		} else {
			if (cur->cursor_xpos >= (cur->lines[row].length - 1)) {
				if ((row + 1) >= cur->row_count)
					return E_REQUEST_DENIED;
				
				cur->cursor_xpos = 0;
				cur->row_xpos = 0;
				if (cur->cursor_ypos == (cur->rows - 1))
					cur->start_line++;
				else
					cur->cursor_ypos++;
			} else {
				old_xpos = cur->cursor_xpos;
				old_row_pos = cur->row_xpos;
				if (saved == '\t')
					cur->cursor_xpos += tab_size(cur,
							cur->lines[row].start,
							cur->row_xpos);
				else
					cur->cursor_xpos++;
				cur->row_xpos++;
				if (cur->cursor_xpos
				    >= cur->lines[row].length) {
					if ((row + 1) >= cur->row_count) {
						cur->cursor_xpos = old_xpos;
						cur->row_xpos = old_row_pos;
						return E_REQUEST_DENIED;
					}

					cur->cursor_xpos = 0;
					cur->row_xpos = 0;
					if (cur->cursor_ypos
					    == (cur->rows - 1))
						cur->start_line++;
					else
						cur->cursor_ypos++;
				}
			}
		}
			
		break;
	
	case REQ_LEFT_CHAR:
		  /*
		   * The behaviour of left_char is the same as prev_char
		   * except that the cursor will not wrap if it has
		   * reached the LHS of the field, so just check this
		   * and fall through if we are not at the LHS.
		   */
		if (cur->cursor_xpos == 0)
			return E_REQUEST_DENIED;

		  /* FALLTHRU */
	case REQ_PREV_CHAR:
		if ((cur->rows + cur->nrows) == 1) {
			if (cur->row_xpos == 0) {
				if (cur->start_char > 0)
					cur->start_char--;
				else
					return E_REQUEST_DENIED;
			} else {
				cur->row_xpos--;
				cur->cursor_xpos =
					_formi_tab_expanded_length(
						cur->buffers[0].string,
						cur->start_char,
						cur->row_xpos
						+ cur->start_char) - 1;
			}
		} else {
			if ((cur->cursor_xpos == 0) &&
			    (cur->cursor_ypos == 0) &&
			    (cur->start_line == 0))
				return E_REQUEST_DENIED;

			row = cur->start_line + cur->cursor_ypos;
			pos = cur->lines[row].start + cur->row_xpos;
			if ((pos >= cur->buffers[0].length) && (pos > 0))
				pos--;
			
			if (cur->cursor_xpos > 0) {
				if (cur->buffers[0].string[pos] == '\t') {
					size = tab_size(cur,
							cur->lines[row].start,
							pos
							- cur->lines[row].start);
					if (size > cur->cursor_xpos) {
						cur->cursor_xpos = 0;
						cur->row_xpos = 0;
					} else {
						cur->row_xpos--;
						cur->cursor_xpos -= size;
					}
				} else {
					cur->cursor_xpos--;
					cur->row_xpos--;
				}
			} else {
				if (cur->cursor_ypos > 0)
					cur->cursor_ypos--;
				else
					cur->start_line--;
				row = cur->start_line + cur->cursor_ypos;
				cur->cursor_xpos = cur->lines[row].length - 1;
				cur->row_xpos = cur->lines[row].end -
					cur->lines[row].start;
			}
		}
		
		break;

	case REQ_DOWN_CHAR:
		  /*
		   * The down_char request has the same functionality as
		   * the next_line request excepting that the field is not
		   * scrolled if the cursor is at the bottom of the field.
		   * Check to see if the cursor is at the bottom of the field
		   * and if it is then deny the request otherwise fall
		   * through to the next_line handler.
		   */
		if (cur->cursor_ypos >= cur->rows - 1)
			return E_REQUEST_DENIED;

		  /* FALLTHRU */
		
	case REQ_NEXT_LINE:
		if ((cur->start_line + cur->cursor_ypos + 1) >= cur->row_count)
			return E_REQUEST_DENIED;
		
		if ((cur->cursor_ypos + 1) >= cur->rows) {
			cur->start_line++;
		} else
			cur->cursor_ypos++;
		row = cur->cursor_ypos + cur->start_line;
		cur->row_xpos =	tab_fit_len(cur, row, cur->cursor_xpos)
			- cur->lines[row].start;
		cur->cursor_xpos =
			_formi_tab_expanded_length(
				&cur->buffers[0].string[cur->lines[row].start],
				0, cur->row_xpos);
		break;
		
	case REQ_UP_CHAR:
		  /*
		   * The up_char request has the same functionality as
		   * the prev_line request excepting the field is not
		   * scrolled, check if the cursor is at the top of the
		   * field, if it is deny the request otherwise fall
		   * through to the prev_line handler.
		   */
		if (cur->cursor_ypos == 0)
			return E_REQUEST_DENIED;

		  /* FALLTHRU */
		
	case REQ_PREV_LINE:
		if (cur->cursor_ypos == 0) {
			if (cur->start_line == 0)
				return E_REQUEST_DENIED;
			cur->start_line--;
		} else
			cur->cursor_ypos--;
		row = cur->cursor_ypos + cur->start_line;
		cur->row_xpos =	tab_fit_len(cur, row, cur->cursor_xpos + 1)
			- cur->lines[row].start;
		cur->cursor_xpos =
			_formi_tab_expanded_length(
				&cur->buffers[0].string[cur->lines[row].start],
				0, cur->row_xpos) - 1;
		break;
		
	case REQ_NEXT_WORD:
		start = cur->lines[cur->start_line + cur->cursor_ypos].start
			+ cur->row_xpos + cur->start_char;
		str = cur->buffers[0].string;

		start = find_eow(str, start);
		
		  /* check if we hit the end */
		if (str[start] == '\0')
			return E_REQUEST_DENIED;

		  /* otherwise we must have found the start of a word...*/
		if ((cur->rows + cur->nrows) == 1) {
			  /* single line field */
			size = _formi_tab_expanded_length(str,
				cur->start_char, start);
			if (size < cur->cols) {
				cur->row_xpos = start - cur->start_char;
				cur->cursor_xpos = size - 1;
			} else {
				cur->start_char = start;
				cur->row_xpos = 0;
				cur->cursor_xpos = 0;
			}
		} else {
			  /* multiline field */
			row = find_cur_line(cur, start);
			cur->row_xpos = start - cur->lines[row].start;
			cur->cursor_xpos = _formi_tab_expanded_length(
				&str[cur->lines[row].start],
				0, cur->row_xpos) - 1;
			if (row != (cur->start_line + cur->cursor_ypos)) {
				if (cur->cursor_ypos == (cur->rows - 1)) {
					cur->start_line = row - cur->rows + 1;
				} else {
					cur->cursor_ypos = row
						- cur->start_line;
				}
			}
		}
		break;
		
	case REQ_PREV_WORD:
		start = cur->start_char + cur->row_xpos
			+ cur->lines[cur->start_line + cur->cursor_ypos].start;
		if (cur->start_char > 0)
			start--;
		
		if (start == 0)
			return E_REQUEST_DENIED;
		
		str = cur->buffers[0].string;

		start = find_sow(str, start);
		
		if ((cur->rows + cur->nrows) == 1) {
			  /* single line field */
			size = _formi_tab_expanded_length(str,
				cur->start_char, start);
			
			if (start > cur->start_char) {
				cur->row_xpos = start - cur->start_char;
				cur->cursor_xpos = size - 1;
			} else {
				cur->start_char = start;
				cur->cursor_xpos = 0;
				cur->row_xpos = 0;
			}
		} else {
			  /* multiline field */
			row = find_cur_line(cur, start);
			cur->row_xpos = start - cur->lines[row].start;
			cur->cursor_xpos = _formi_tab_expanded_length(
				&str[cur->lines[row].start], 0,
				cur->row_xpos) - 1;
			if (row != (cur->start_line + cur->cursor_ypos)) {
				if (cur->cursor_ypos == 0) {
					cur->start_line = row;
				} else {
					if (cur->start_line > row) {
						cur->start_line = row;
						cur->cursor_ypos = 0;
					} else {
						cur->cursor_ypos = row -
							cur->start_line;
					}
				}
			}
		}
		
		break;

	case REQ_BEG_FIELD:
		cur->start_char = 0;
		cur->start_line = 0;
		cur->row_xpos = 0;
		cur->cursor_xpos = 0;
		cur->cursor_ypos = 0;
		break;
		
	case REQ_BEG_LINE:
		cur->cursor_xpos = 0;
		cur->row_xpos = 0;
		cur->start_char = 0;
		break;
			
	case REQ_END_FIELD:
		if (cur->row_count > cur->rows) {
			cur->start_line = cur->row_count - cur->rows;
			cur->cursor_ypos = cur->rows - 1;
		} else {
			cur->start_line = 0;
			cur->cursor_ypos = cur->row_count - 1;
		}

		  /* we fall through here deliberately, we are on the
		   * correct row, now we need to get to the end of the
		   * line.
		   */
		  /* FALLTHRU */
		
	case REQ_END_LINE:
		row = cur->start_line + cur->cursor_ypos;
		start = cur->lines[row].start;
		end = cur->lines[row].end;

		if ((cur->rows + cur->nrows) == 1) {
			if (cur->lines[row].length > cur->cols - 1) {
				if ((cur->opts & O_STATIC) != O_STATIC) {
					cur->start_char = tab_fit_window(
						cur, end, cur->cols) + 1;
				} else {
					cur->start_char = tab_fit_window(
						cur, end, cur->cols);
				}
				cur->row_xpos = cur->buffers[0].length
					- cur->start_char;
				cur->cursor_xpos =
					_formi_tab_expanded_length(
					     cur->buffers[0].string,
					     cur->start_char,
					     cur->row_xpos + cur->start_char);
			} else {
				cur->row_xpos = end + 1;
				cur->cursor_xpos = _formi_tab_expanded_length(
					cur->buffers[0].string,
					cur->start_char,
					cur->row_xpos + cur->start_char);
				if (((cur->opts & O_STATIC) == O_STATIC) &&
				    ((cur->lines[row].length)
				     == (cur->cols - 1)))
					cur->cursor_xpos--;
				
				cur->start_char = 0;
			}
		} else {
			cur->row_xpos = end - start;
			cur->cursor_xpos = cur->lines[row].length - 1;
			if (row == (cur->row_count - 1)) {
				cur->row_xpos++;
				cur->cursor_xpos++;
			}
		}
		break;
		
	case REQ_NEW_LINE:
		if ((status = split_line(cur,
				cur->start_char + cur->row_xpos)) != E_OK)
			return status;
		break;
		
	case REQ_INS_CHAR:
		if ((status = _formi_add_char(cur, cur->start_char
					      + cur->row_xpos,
					      cur->pad)) != E_OK)
			return status;
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
		start = cur->start_char + cur->row_xpos
			+ cur->lines[row].start;
		end = cur->buffers[0].length;
		if (start >= cur->buffers[0].length)
			return E_REQUEST_DENIED;
		
		if (start == cur->lines[row].end) {
			if ((cur->rows + cur->nrows) > 1) {
				/*
				 * If we have more than one row, join the
				 * next row to make things easier unless
				 * we are at the end of the string, in
				 * that case the join would fail but we
				 * really want to delete the last char
				 * in the field.
				 */
				if ((cur->row_count > 1)
				    && (start != (end - 1))) {
					if (_formi_join_line(cur,
							     start,
							     JOIN_NEXT_NW)
					    != E_OK) {
						return E_REQUEST_DENIED;
					}
				}
			}
		}
			
		saved = cur->buffers[0].string[start];
		bcopy(&cur->buffers[0].string[start + 1],
		      &cur->buffers[0].string[start],
		      (unsigned) end - start + 1);
		bump_lines(cur, _FORMI_USE_CURRENT, -1, TRUE);
		cur->buffers[0].length--;

		  /*
		   * recalculate tabs for a single line field, multiline
		   * fields will do this when the field is wrapped.
		   */
		if ((cur->rows + cur->nrows) == 1)
			_formi_calculate_tabs(cur, 0);
		  /*
		   * if we are at the end of the string then back the
		   * cursor pos up one to stick on the end of the line
		   */
		if (start == cur->buffers[0].length) {
			if (cur->buffers[0].length > 1) {
				if ((cur->rows + cur->nrows) == 1) {
					pos = cur->row_xpos + cur->start_char;
					cur->start_char =
						tab_fit_window(cur,
							       cur->start_char + cur->row_xpos,
							       cur->cols);
					cur->row_xpos = pos - cur->start_char
						- 1;
					cur->cursor_xpos =
						_formi_tab_expanded_length(
							cur->buffers[0].string,
							cur->start_char,
							cur->row_xpos
							+ cur->start_char);
				} else {
					if (cur->row_xpos == 0) {
						if (cur->lines[row].start !=
						    cur->buffers[0].length) {
							if (_formi_join_line(
								cur,
								cur->lines[row].start,
								JOIN_PREV_NW)
							    != E_OK) {
								return E_REQUEST_DENIED;
							}
						} else {
							if (cur->row_count > 1)
								cur->row_count--;
						}

						row = cur->start_line
							+ cur->cursor_ypos;
						if (row > 0)
							row--;
					}
					
					cur->row_xpos = start
						- cur->lines[row].start - 1;
					cur->cursor_xpos =
						_formi_tab_expanded_length(
							&cur->buffers[0].string[cur->lines[row].start],
							0, cur->row_xpos);
					if ((cur->cursor_xpos > 0)
					    && (start != (cur->buffers[0].length - 1)))
						cur->cursor_xpos--;
					if (row >= cur->rows)
						cur->start_line = row
							- cur->cursor_ypos;
					else {
						cur->start_line = 0;
						cur->cursor_ypos = row;
					}
				}
				
				start--;
			} else {
				start = 0;
				cur->row_xpos = 0;
				cur->cursor_xpos = 0;
			}
		}
		
		if ((cur->rows + cur->nrows) > 1) {
			if (_formi_wrap_field(cur, start) != E_OK) {
				bcopy(&cur->buffers[0].string[start],
				      &cur->buffers[0].string[start + 1],
				      (unsigned) end - start);
				cur->buffers[0].length++;
				cur->buffers[0].string[start] = saved;
				bump_lines(cur, _FORMI_USE_CURRENT, 1, TRUE);
				_formi_wrap_field(cur, start);
				return E_REQUEST_DENIED;
			}
		}
		break;
			
	case REQ_DEL_PREV:
		if ((cur->cursor_xpos == 0) && (cur->start_char == 0)
		    && (cur->start_line == 0) && (cur->cursor_ypos == 0))
			   return E_REQUEST_DENIED;

		row = cur->start_line + cur->cursor_ypos;
		start = cur->row_xpos + cur->start_char
			+ cur->lines[row].start;
		end = cur->buffers[0].length;

		if ((cur->start_char + cur->row_xpos) == 0) {
			  /*
			   * Join this line to the next one, but only if
			   * we are not at the end of the string because
			   * in that case there are no characters to join.
			   */
			if (cur->lines[row].start != end) {
				if (_formi_join_line(cur,
						     cur->lines[row].start,
						     JOIN_PREV_NW) != E_OK) {
					return E_REQUEST_DENIED;
				}
			} else {
				/* but we do want the row count decremented */
				if (cur->row_count > 1)
					cur->row_count--;
			}
		}

		saved = cur->buffers[0].string[start - 1];
		bcopy(&cur->buffers[0].string[start],
		      &cur->buffers[0].string[start - 1],
		      (unsigned) end - start + 1);
		bump_lines(cur, (int) start - 1, -1, TRUE);
		cur->buffers[0].length--;

		if ((cur->rows + cur->nrows) == 1) {
			_formi_calculate_tabs(cur, 0);
			pos = cur->row_xpos + cur->start_char;
			if (pos > 0)
				pos--;
			cur->start_char =
				tab_fit_window(cur,
					       cur->start_char + cur->row_xpos,
					       cur->cols);
			cur->row_xpos = pos - cur->start_char;
			cur->cursor_xpos =
				_formi_tab_expanded_length(
					cur->buffers[0].string,
					cur->start_char,
					cur->row_xpos
					+ cur->start_char);
		} else {
			pos = start - 1;
			if (pos >= cur->buffers[0].length)
				pos = cur->buffers[0].length - 1;

			if ((_formi_wrap_field(cur, pos) != E_OK)) {
				bcopy(&cur->buffers[0].string[start - 1],
				      &cur->buffers[0].string[start],
				      (unsigned) end - start);
				cur->buffers[0].length++;
				cur->buffers[0].string[start - 1] = saved;
				bump_lines(cur, (int) start - 1, 1, TRUE);
				_formi_wrap_field(cur, pos);
				return E_REQUEST_DENIED;
			}
			
			row = find_cur_line(cur, pos);
			cur->row_xpos = start - cur->lines[row].start - 1;
			cur->cursor_xpos = _formi_tab_expanded_length(
				&cur->buffers[0].string[cur->lines[row].start],
				0, cur->row_xpos);
			if ((cur->cursor_xpos > 0)
			    && (pos != (cur->buffers[0].length - 1)))
				cur->cursor_xpos--;

			if (row >= cur->rows)
				cur->start_line = row - cur->cursor_ypos;
			else {
				cur->start_line = 0;
				cur->cursor_ypos = row;
			}
		}
		break;
			
	case REQ_DEL_LINE:
		row = cur->start_line + cur->cursor_ypos;
		start = cur->lines[row].start;
		end = cur->lines[row].end;
		bcopy(&cur->buffers[0].string[end + 1],
		      &cur->buffers[0].string[start],
		      (unsigned) cur->buffers[0].length - end + 1);

		if (((cur->rows + cur->nrows) == 1) ||
		    (cur->row_count == 1)) {
			  /* single line case */
			cur->buffers[0].length = 0;
			cur->lines[0].end = cur->lines[0].length = 0;
			cur->cursor_xpos = cur->row_xpos = 0;
			cur->cursor_ypos = 0;
		} else {
			  /* multiline field */
			old_count = cur->row_count;
			cur->row_count--;
			if (cur->row_count == 0)
				cur->row_count = 1;

			if (cur->row_count > 1)
				bcopy(&cur->lines[row + 1],
				      &cur->lines[row],
				      (unsigned) (cur->row_count - row)
				      * sizeof(struct _formi_field_lines));

			cur->lines[row].start = start;
			len = start - end - 1; /* yes, this is negative */

			if (row < (cur->row_count - 1))
				bump_lines(cur, (int) start, len, FALSE);
			else if (old_count == 1) {
				cur->lines[0].end = cur->lines[0].length = 0;
				cur->cursor_xpos = 0;
				cur->row_xpos = 0;
				cur->cursor_ypos = 0;
			} else if (cur->row_count == 1) {
				cur->lines[0].length = cur->buffers[0].length
					+ len;
				cur->lines[0].end = cur->lines[0].length - 1;
			}
			
			cur->buffers[0].length += len;
			
			if (row > (cur->row_count - 1)) {
				row--;
				if (cur->cursor_ypos == 0) {
					if (cur->start_line > 0) {
						cur->start_line--;
					}
				} else {
					cur->cursor_ypos--;
				}
			}

			if (old_count > 1) {
				if (cur->cursor_xpos > cur->lines[row].length) {
					cur->cursor_xpos =
						cur->lines[row].length - 1;
					cur->row_xpos = cur->lines[row].end;
				}
				
				if (row >= cur->rows)
					cur->start_line = row
						- cur->cursor_ypos;
				else {
					cur->start_line = 0;
					cur->cursor_ypos = row;
				}
			} 
		}
		break;

	case REQ_DEL_WORD:
		start = cur->start_char + cur->row_xpos
			+ cur->lines[row].start;
		str = cur->buffers[0].string;

		end = find_eow(str, start);

		  /*
		   * If not at the start of a word then find the start,
		   * we cannot blindly call find_sow because this will
		   * skip back a word if we are already at the start of
		   * a word.
		   */
		if ((start > 0)
		    && !(isblank(str[start - 1]) && !isblank(str[start])))
			start = find_sow(cur->buffers[0].string, start);
		bcopy(&cur->buffers[0].string[end],
		      &cur->buffers[0].string[start],
		      (unsigned) cur->buffers[0].length - end + 1);
		len = end - start;
		cur->buffers[0].length -= len;
		bump_lines(cur, _FORMI_USE_CURRENT, - (int) len, TRUE);

		if ((cur->rows + cur->nrows) > 1) {
			row = cur->start_line + cur->cursor_ypos;
			if ((row + 1) < cur->row_count) {
				/*
				 * if not on the last row we need to
				 * join on the next row so the line
				 * will be re-wrapped.
				 */
				_formi_join_line(cur, cur->lines[row].end,
						 JOIN_NEXT_NW);
			}
			_formi_wrap_field(cur, start);
			row = find_cur_line(cur, start);
			cur->row_xpos = start - cur->lines[row].start;
			cur->cursor_xpos = _formi_tab_expanded_length(
				&cur->buffers[0].string[cur->lines[row].start],
				0, cur->row_xpos);
			if ((cur->cursor_xpos > 0)
			    && (start != (cur->buffers[0].length - 1)))
				cur->cursor_xpos--;
		} else {
			_formi_calculate_tabs(cur, 0);
			cur->row_xpos = start - cur->start_char;
			if (cur->row_xpos > 0)
				cur->row_xpos--;
			cur->cursor_xpos = _formi_tab_expanded_length(
				cur->buffers[0].string, cur->start_char,
				cur->row_xpos);
			if (cur->cursor_xpos > 0)
				cur->cursor_xpos--;
		}
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
		bump_lines(cur, _FORMI_USE_CURRENT, - (int) len, TRUE);
		
		if (cur->cursor_xpos > cur->lines[row].length) {
			cur->row_xpos = cur->lines[row].end;
			cur->cursor_xpos = cur->lines[row].length;
		}
		break;

	case REQ_CLR_EOF:
		row = cur->start_line + cur->cursor_ypos;
		cur->buffers[0].string[cur->start_char
				      + cur->cursor_xpos] = '\0';
		cur->buffers[0].length = strlen(cur->buffers[0].string);
		cur->lines[row].end = cur->buffers[0].length;
		cur->lines[row].length = cur->lines[row].end
			- cur->lines[row].start;
		
		for (i = cur->start_char + cur->row_xpos;
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
		cur->row_xpos = 0;
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
	fprintf(dbg,
	 "exit: xpos=%d, row_pos=%d, start_char=%d, length=%d, allocated=%d\n",
		cur->cursor_xpos, cur->row_xpos, cur->start_char,
		cur->buffers[0].length,	cur->buffers[0].allocated);
	fprintf(dbg, "exit: start_line=%d, ypos=%d\n", cur->start_line,
		cur->cursor_ypos);
	fprintf(dbg, "exit: string=\"%s\"\n", cur->buffers[0].string);
	assert ((cur->cursor_xpos < INT_MAX) && (cur->row_xpos < INT_MAX)
		&& (cur->cursor_xpos >= cur->row_xpos));
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

/*
 * Calculate the length of the displayed line allowing for any tab
 * characters that need to be expanded.  We assume that the tab stops
 * are 8 characters apart.  The parameters start and end are the
 * character positions in the string str we want to get the length of,
 * the function returns the number of characters from the start
 * position to the end position that should be displayed after any
 * intervening tabs have been expanded.
 */
int
_formi_tab_expanded_length(char *str, unsigned int start, unsigned int end)
{
	int len, start_len, i;

	  /* if we have a null string then there is no length */
	if (str[0] == '\0')
		return 0;
	
	len = 0;
	start_len = 0;

	  /*
	   * preceding tabs affect the length tabs in the span, so
	   * we need to calculate the length including the stuff before
	   * start and then subtract off the unwanted bit.
	   */
	for (i = 0; i <= end; i++) {
		if (i == start) /* stash preamble length for later */
			start_len = len;

		if (str[i] == '\0')
			break;
		
		if (str[i] == '\t')
			len = len - (len % 8) + 8;
		else
			len++;
	}

#ifdef DEBUG
	if (dbg != NULL) {
		fprintf(dbg,
		    "tab_expanded: start=%d, end=%d, expanded=%d (diff=%d)\n",
			start, end, (len - start_len), (end - start));
	}
#endif
	
	return (len - start_len);
}

/*
 * Calculate the tab stops on a given line in the field and set up
 * the tabs list with the results.  We do this by scanning the line for tab
 * characters and if one is found, noting the position and the number of
 * characters to get to the next tab stop.  This information is kept to
 * make manipulating the field (scrolling and so on) easier to handle.
 */
void
_formi_calculate_tabs(FIELD *field, unsigned row)
{
	_formi_tab_t *ts = field->lines[row].tabs, *old_ts = NULL, **tsp;
	int i, j;

	  /*
	   * If the line already has tabs then invalidate them by
	   * walking the list and killing the in_use flag.
	   */
	for (; ts != NULL; ts = ts->fwd)
		ts->in_use = FALSE;


	  /*
	   * Now look for tabs in the row and record the info...
	   */
	tsp = &field->lines[row].tabs;
	for (i = field->lines[row].start, j = 0; i <= field->lines[row].end;
	     i++, j++) {
		if (field->buffers[0].string[i] == '\t') {
			if (*tsp == NULL) {
				if ((*tsp = (_formi_tab_t *)
				     malloc(sizeof(_formi_tab_t))) == NULL)
					return;
				(*tsp)->back = old_ts;
				(*tsp)->fwd = NULL;
			}
			
			(*tsp)->in_use = TRUE;
			(*tsp)->pos = i - field->lines[row].start;
			(*tsp)->size = 8 - (j % 8);
			j += (*tsp)->size - 1;
			old_ts = *tsp;
			tsp = &(*tsp)->fwd;
		}
	}
}

/*
 * Return the size of the tab padding for a tab character at the given
 * position.  Return 1 if there is not a tab char entry matching the
 * given location.
 */
static int
tab_size(FIELD *field, unsigned int offset, unsigned int i)
{
	int row;
	_formi_tab_t *ts;

	row = find_cur_line(field, offset + i);
	ts = field->lines[row].tabs;
	
	while ((ts != NULL) && (ts->pos != i))
		ts = ts->fwd;

	if (ts == NULL)
		return 1;
	else
		return ts->size;
}

/*
 * Find the character offset that corresponds to longest tab expanded
 * string that will fit into the given window.  Walk the string backwards
 * evaluating the sizes of any tabs that are in the string.  Note that
 * using this function on a multi-line window will produce undefined
 * results - it is really only required for a single row field.
 */
static int
tab_fit_window(FIELD *field, unsigned int pos, unsigned int window)
{
	int scroll_amt, i;
	_formi_tab_t *ts;
	
	  /* first find the last tab */
	ts = field->lines[0].tabs;

	  /*
	   * unless there are no tabs - just return the window size,
	   * if there is enough room, otherwise 0.
	   */
	if (ts == NULL) {
		if (field->buffers[0].length < window)
			return 0;
		else
			return field->buffers[0].length - window + 1;
	}
		
	while ((ts->fwd != NULL) && (ts->fwd->in_use == TRUE))
		ts = ts->fwd;

	  /*
	   * now walk backwards finding the first tab that is to the
	   * left of our starting pos.
	   */
	while ((ts != NULL) && (ts->in_use == TRUE) && (ts->pos > pos))
		ts = ts->back;
	
	scroll_amt = 0;
	for (i = pos; i >= 0; i--) {
		if (field->buffers[0].string[i] == '\t') {
#ifdef DEBUG
			assert((ts != NULL) && (ts->in_use == TRUE));
#endif
			if (ts->pos == i) {
				if ((scroll_amt + ts->size) > window) {
					break;
				}
				scroll_amt += ts->size;
				ts = ts->back;
			}
#ifdef DEBUG
			else
				assert(ts->pos == i);
#endif
		} else {
			scroll_amt++;
			if (scroll_amt > window)
				break;
		}
	}

	return ++i;
}

/*
 * Return the position of the last character that will fit into the
 * given width after tabs have been expanded for a given row of a given
 * field.
 */
static unsigned int
tab_fit_len(FIELD *field, unsigned int row, unsigned int width)
{
	unsigned int pos, len, row_pos;
	_formi_tab_t *ts;

	ts = field->lines[row].tabs;
	pos = field->lines[row].start;
	len = 0;
	row_pos = 0;
	
	while ((len < width) && (pos < field->buffers[0].length)) {
		if (field->buffers[0].string[pos] == '\t') {
#ifdef DEBUG
			assert((ts != NULL) && (ts->in_use == TRUE));
#endif
			if (ts->pos == row_pos) {
				if ((len + ts->size) > width)
					break;
				len += ts->size;
				ts = ts->fwd;
			}
#ifdef DEBUG
			else
				assert(ts->pos == row_pos);
#endif
		} else
			len++;
		pos++;
		row_pos++;
	}

	if (pos > 0)
		pos--;
	return pos;
}
