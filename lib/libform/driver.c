/*	$NetBSD: driver.c,v 1.1 2000/12/17 12:04:30 blymn Exp $	*/

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
#include "form.h"
#include "internals.h"

static int
traverse_form_links(FORM *form, int direction);

/*
 * Traverse the links of the current field in the given direction until
 * either a active & visible field is found or we return to the current
 * field.  Direction is the REQ_{LEFT,RIGHT,UP,DOWN}_FIELD driver commands.
 * The function returns E_OK if a valid field is found, E_REQUEST_DENIED
 * otherwise.
 */
static int
traverse_form_links(FORM *form, int direction)
{
	unsigned index;

	index = form->cur_field;
	
	do {
		switch (direction) {
		case REQ_LEFT_FIELD:
			if (form->fields[index]->left == NULL)
				return E_REQUEST_DENIED;
			index = form->fields[index]->left->index;
			break;
			
		case REQ_RIGHT_FIELD:
			if (form->fields[index]->right == NULL)
				return E_REQUEST_DENIED;
			index = form->fields[index]->right->index;
			break;
			
		case REQ_UP_FIELD:
			if (form->fields[index]->up == NULL)
				return E_REQUEST_DENIED;
			index = form->fields[index]->up->index;
			break;
			
		case REQ_DOWN_FIELD:
			if (form->fields[index]->down == NULL)
				return E_REQUEST_DENIED;
			index = form->fields[index]->down->index;
			break;

		default:
			return E_REQUEST_DENIED;
		}

		if ((form->fields[index]->opts & (O_ACTIVE | O_VISIBLE))
		    == (O_ACTIVE | O_VISIBLE)) {
			form->cur_field = index;
			return E_OK;
		}
	} while (index != form->cur_field);

	return E_REQUEST_DENIED;
}

int
form_driver(FORM *form, int c)
{
	FIELD *fieldp;
	FORM_STR buf;
	int update_page, update_field, old_field, old_page, status;
	unsigned int pos;
	
	if (form == NULL)
		return E_BAD_ARGUMENT;

	if ((form->fields == NULL) || (*(form->fields) == NULL))
		return E_INVALID_FIELD;

	if (form->posted != 1)
		return E_NOT_POSTED;

	if (form->in_init == 1)
		return E_BAD_STATE;


	old_field = form->cur_field;
	update_page = update_field = 0;
	
	if (c < REQ_MIN_REQUEST) {
		if (isprint(c)) {
		  next_field:
			fieldp = form->fields[form->cur_field];
			buf = fieldp->buffers[0];
			
			pos = fieldp->start_char + fieldp->cursor_xpos
				+ fieldp->hscroll;

			  /*
			   * Need to check here if we want to autoskip.
			   * we call the form driver recursively to pos
			   * us on the next field and then we loop back to
			   * ensure the next field selected can have data
			   * added to it
			   */
			if ((((fieldp->opts & O_STATIC) == O_STATIC) &&
			     (buf.length >= fieldp->cols)) ||
			    (((fieldp->opts & O_STATIC) != O_STATIC) &&
			     ((fieldp->max > 0) &&
			      (buf.length >= fieldp->max)))) {
				if ((fieldp->opts & O_AUTOSKIP) != O_AUTOSKIP)
					return E_REQUEST_DENIED;
				status = form_driver(form, REQ_NEXT_FIELD);
				if (status != E_OK)
					return status;
				old_field = form->cur_field;
				goto next_field;
			}
			
				
			if (fieldp->start_char > 0)
  				pos--;

			update_field = _formi_add_char(fieldp, pos, c);
		
		} else
			return E_REQUEST_DENIED;
	} else {
		if (c > REQ_MAX_COMMAND)
			return E_UNKNOWN_COMMAND;

		switch (c) {
		case REQ_NEXT_PAGE:
			if (form->page < form->max_page) {
				old_page = form->page;
				form->page++;
				update_page = 1;
				if (_formi_pos_first_field(form) != E_OK) {
					form->page = old_page;
					return E_REQUEST_DENIED;
				}
			} else
				return E_REQUEST_DENIED;
			break;
		
		case REQ_PREV_PAGE:
			if (form->page > 0) {
				old_page = form->page;
				form->page--;
				update_page = 1;
				if (_formi_pos_first_field(form) != E_OK) {
					form->page = old_page;
					return E_REQUEST_DENIED;
				}
			} else
				return E_REQUEST_DENIED;
			break;
		
		case REQ_FIRST_PAGE:
			old_page = form->page;
			form->page = 0;
			update_page = 1;
			if (_formi_pos_first_field(form) != E_OK) {
				form->page = old_page;
				return E_REQUEST_DENIED;
			}
			break;
		
		case REQ_LAST_PAGE:
			old_page = form->page;
			form->page = form->max_page - 1;
			update_page = 1;
			if (_formi_pos_first_field(form) != E_OK) {
				form->page = old_page;
				return E_REQUEST_DENIED;
			}
			break;
		
		case REQ_NEXT_FIELD:
			status = _formi_pos_new_field(form, _FORMI_FORWARD,
						      FALSE);
			if (status != E_OK) {
				return status;
			}
			
			update_field = 1;
			break;
		
		case REQ_PREV_FIELD:
			status = _formi_pos_new_field(form, _FORMI_BACKWARD,
						      FALSE);
			
			if (status != E_OK)
				return status;

			update_field = 1;
			break;
		
		case REQ_FIRST_FIELD:
			form->cur_field = 0;
			update_field = 1;
			break;
		
		case REQ_LAST_FIELD:
			form->cur_field = form->field_count - 1;
			update_field = 1;
			break;
		
		case REQ_SNEXT_FIELD:
			status = _formi_pos_new_field(form, _FORMI_FORWARD,
						      TRUE);
			if (status != E_OK)
				return status;

			update_field = 1;
			break;
		
		case REQ_SPREV_FIELD:
			status = _formi_pos_new_field(form, _FORMI_BACKWARD,
						      TRUE);
			if (status != E_OK)
				return status;

			update_field = 1;
			break;
		
		case REQ_SFIRST_FIELD:
			fieldp = CIRCLEQ_FIRST(&form->sorted_fields);
			form->cur_field = fieldp->index;
			update_field = 1;
			break;
		
		case REQ_SLAST_FIELD:
			fieldp = CIRCLEQ_LAST(&form->sorted_fields);
			form->cur_field = fieldp->index;
			update_field = 1;
			break;

			  /*
			   * The up, down, left and right field traversals
			   * are rolled up into a single function, allow a
			   * fall through to that function.
			   */
			  /* FALLTHROUGH */
		case REQ_LEFT_FIELD:
		case REQ_RIGHT_FIELD:
		case REQ_UP_FIELD:
		case REQ_DOWN_FIELD:
			status = traverse_form_links(form, c);
			if (status != E_OK)
				return status;
			
			update_field = 1;
			break;

			  /* the following commands modify the buffer, check if
			     this is allowed first before falling through. */
			  /* FALLTHROUGH */
		case REQ_INS_CHAR:
		case REQ_INS_LINE:
		case REQ_DEL_CHAR:
		case REQ_DEL_PREV:
		case REQ_DEL_LINE:
		case REQ_DEL_WORD:
		case REQ_CLR_EOL:
		case REQ_CLR_EOF:
		case REQ_CLR_FIELD:
		case REQ_OVL_MODE:
		case REQ_INS_MODE:
		case REQ_NEW_LINE:
			  /* check if we are allowed to edit the field and fall
			   * through if we are.
			   */
			if ((form->fields[form->cur_field]->opts & O_EDIT) != O_EDIT)
				return E_REQUEST_DENIED;
		
			  /* the following manipulate the field contents, bundle
			     them into one function.... */
			  /* FALLTHROUGH */
		case REQ_NEXT_CHAR:
		case REQ_PREV_CHAR:
		case REQ_NEXT_LINE:
		case REQ_PREV_LINE:
		case REQ_NEXT_WORD:
		case REQ_PREV_WORD:
		case REQ_BEG_FIELD:
		case REQ_END_FIELD:
		case REQ_BEG_LINE:
		case REQ_END_LINE:
		case REQ_LEFT_CHAR:
		case REQ_RIGHT_CHAR:
		case REQ_UP_CHAR:
		case REQ_DOWN_CHAR:
		case REQ_SCR_FLINE:
		case REQ_SCR_BLINE:
		case REQ_SCR_FPAGE:
		case REQ_SCR_BPAGE:
		case REQ_SCR_FHPAGE:
		case REQ_SCR_BHPAGE:
		case REQ_SCR_FCHAR:
		case REQ_SCR_BCHAR:
		case REQ_SCR_HFLINE:
		case REQ_SCR_HBLINE:
		case REQ_SCR_HFHALF:
		case REQ_SCR_HBHALF:
			update_field = _formi_manipulate_field(form, c);
			break;
		
		case REQ_VALIDATION:
			return _formi_validate_field(form);
			  /* NOTREACHED */
			break;
		
		case REQ_PREV_CHOICE:
		case REQ_NEXT_CHOICE:
			update_field = _formi_field_choice(form, c);
			break;

		default: /* should not need to do this, but.... */
			return E_UNKNOWN_COMMAND;
			  /* NOTREACHED */
			break;
		}
	}
	
	if (update_field < 0)
		return update_field;

	if (update_field == 1)
		update_page |= _formi_update_field(form, old_field);

	if (update_page == 1)
		_formi_draw_page(form);

	pos_form_cursor(form);
	return E_OK;
}

