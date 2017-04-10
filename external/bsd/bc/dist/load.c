/*	$NetBSD: load.c,v 1.1 2017/04/10 02:28:23 phil Exp $ */

/*
 * Copyright (C) 1991-1994, 1997, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Philip A. Nelson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names Philip A. Nelson and Free Software Foundation may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP A. NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP A. NELSON OR THE FREE SOFTWARE FOUNDATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* load.c:  This code "loads" code into the code segments. */

#include "bcdefs.h"
#include "proto.h"

/* Load variables. */

program_counter load_adr;
char load_str;
char load_const;

/* Initialize the load sequence. */
void
init_load (void)
{
  clear_func(0);
  load_adr.pc_func = 0;
  load_adr.pc_addr = 0;
  load_str = FALSE;
  load_const = FALSE;
}

/* addbyte adds one BYTE to the current code segment. */
void
addbyte (unsigned char thebyte)
{
  unsigned long prog_addr;
  bc_function *f;
  char *new_body;

  /* If there was an error, don't continue. */
  if (had_error) return;

  /* Calculate the segment and offset. */
  prog_addr = load_adr.pc_addr++;
  f = &functions[load_adr.pc_func];

  if (prog_addr >= f->f_body_size)
    {
      f->f_body_size *= 2;
      new_body = bc_malloc (f->f_body_size);
      memcpy(new_body, f->f_body, f->f_body_size/2);
      free (f->f_body);
      f->f_body = new_body;
    }

  /* Store the thebyte. */
  f->f_body[prog_addr] = (char) (thebyte & 0xff);
  f->f_code_size++;
}


/* Define a label LAB to be the current program counter. */

void
def_label (unsigned long lab)
{
  bc_label_group *temp;
  unsigned long group, offset, func;
    
  /* Get things ready. */
  group = lab >> BC_LABEL_LOG;
  offset = lab % BC_LABEL_GROUP;
  func = load_adr.pc_func;
  
  /* Make sure there is at least one label group. */
  if (functions[func].f_label == NULL)
    {
      functions[func].f_label = bc_malloc (sizeof(bc_label_group));
      functions[func].f_label->l_next = NULL;
    }

  /* Add the label group. */
  temp = functions[func].f_label;
  while (group > 0)
    {
      if (temp->l_next == NULL)
	{
	  temp->l_next = bc_malloc (sizeof(bc_label_group));
	  temp->l_next->l_next = NULL;
	}
      temp = temp->l_next;
      group --;
    }

  /* Define it! */
  temp->l_adrs [offset] = load_adr.pc_addr;
}

/* Several instructions have integers in the code.  They
   are all known to be legal longs.  So, no error code
   is added.  STR is the pointer to the load string and
   must be moved to the last non-digit character. */

long
long_val (const char **str)
{ int  val = 0;
  char neg = FALSE;

  if (**str == '-')
    {
      neg = TRUE;
      (*str)++;
    }
  while (isdigit((int)(**str))) 
    val = val*10 + *(*str)++ - '0';

  if (neg)
    return -val;
  else
    return val;
}


/* load_code loads the CODE into the machine. */

void
load_code (const char *code)
{
  const char *str;
  unsigned long  ap_name;	/* auto or parameter name. */
  unsigned long  label_no;
  unsigned long  vaf_name;	/* variable, array or function number. */
  unsigned long  func;
  static program_counter save_adr;

  /* Initialize. */
  str = code;
   
  /* Scan the code. */
  while (*str != 0)
    {
      /* If there was an error, don't continue. */
      if (had_error) return;

      if (load_str)
	{
	  if (*str == '"') load_str = FALSE;
	  addbyte (*str++);
	}
      else
	if (load_const)
	  {
	    if (*str == '\n') 
	      str++;
	    else
	      {
		if (*str == ':')
		  {
		    load_const = FALSE;
		    addbyte (*str++);
		  }
		else
		  if (*str == '.')
		    addbyte (*str++);
		  else
                    {
		      if (*str > 'F' && (warn_not_std || std_only))
                        {
                          if (std_only)
                            yyerror ("Error in numeric constant");
                          else
                            ct_warn ("Non-standard base in numeric constant");
                        } 
		      if (*str >= 'A')
		        addbyte (*str++ + 10 - 'A');
		      else
		        addbyte (*str++ - '0');
                    }
	      }
	  }
	else
	  {
	    switch (*str)
	      {

	      case '"':	/* Starts a string. */
		load_str = TRUE;
		break;

	      case 'N': /* A label */
		str++;
		label_no = long_val (&str);
		def_label (label_no);
		break;

	      case 'B':  /* Branch to label. */
	      case 'J':  /* Jump to label. */
	      case 'Z':  /* Branch Zero to label. */
		addbyte(*str++);
		label_no = long_val (&str);
		if (label_no > 65535L)
		  {  /* Better message? */
		    fprintf (stderr,"Program too big.\n");
		    bc_exit(1);
		  }
		addbyte ( (char) (label_no & 0xFF));
		addbyte ( (char) (label_no >> 8));
		break;

	      case 'F':  /* A function, get the name and initialize it. */
		str++;
		func = long_val (&str);
		clear_func (func);
#if DEBUG > 2
		printf ("Loading function number %d\n", func);
#endif
		/* get the parameters */
		while (*str++ != '.')
		  {
		    if (*str == '.')
		      {
			str++;
			break;
		      }
		    if (*str == '*')
		      {
			str++;
			ap_name = long_val (&str);
#if DEBUG > 2
			printf ("var parameter number %d\n", ap_name);
#endif
			functions[(int)func].f_params = 
			  nextarg (functions[(int)func].f_params, ap_name,
				   TRUE);
		      }
		    else
		      {
			ap_name = long_val (&str);
#if DEBUG > 2
			printf ("parameter number %d\n", ap_name);
#endif
			functions[(int)func].f_params = 
			  nextarg (functions[(int)func].f_params, ap_name,
				   FALSE);
		      }
		  }

		/* get the auto vars */
		while (*str != '[')
		  {
		    if (*str == ',') str++;
		    ap_name = long_val (&str);
#if DEBUG > 2
		    printf ("auto number %d\n", ap_name);
#endif
		    functions[(int)func].f_autos = 
		      nextarg (functions[(int)func].f_autos, ap_name, FALSE);
		  }
		save_adr = load_adr;
		load_adr.pc_func = func;
		load_adr.pc_addr = 0;
		break;
		
	      case ']':  /* A function end */
		functions[load_adr.pc_func].f_defined = TRUE;
		load_adr = save_adr;
		break;

	      case 'C':  /* Call a function. */
		addbyte (*str++);
		func = long_val (&str);
		if (func < 128)
		  addbyte ( (char) func);
		else
		  {
		    addbyte (((func >> 8) & 0xff) | 0x80);
		    addbyte (func & 0xff);
		  }
		if (*str == ',') str++;
		while (*str != ':')
		  addbyte (*str++);
		addbyte (':');
		break;
		
	      case 'c':  /* Call a special function. */
		addbyte (*str++);
		addbyte (*str);
		break;

	      case 'K':  /* A constant.... may have an "F" in it. */
		addbyte (*str);
		load_const = TRUE;
		break;

	      case 'd':  /* Decrement. */
	      case 'i':  /* Increment. */
	      case 'l':  /* Load. */
	      case 's':  /* Store. */
	      case 'A':  /* Array Increment */
	      case 'M':  /* Array Decrement */
	      case 'L':  /* Array Load */
	      case 'S':  /* Array Store */
		addbyte (*str++);
		vaf_name = long_val (&str);
		if (vaf_name < 128)
		  addbyte (vaf_name);
		else
		  {
		    addbyte (((vaf_name >> 8) & 0xff) | 0x80);
		    addbyte (vaf_name & 0xff);
		  }
		break;

	      case '@':  /* A command! */
		switch (*(++str))
		  {
		  case 'i':
		    init_load ();
		    break;
		  case 'r':
		    execute ();
		    break;
		  } 
		break;

	      case '\n':  /* Ignore the newlines */
		break;
		
	      default:   /* Anything else */
		addbyte (*str);	   
	      }
	    str++;
	  }
    }
}
