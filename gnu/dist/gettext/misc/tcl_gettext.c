/* tcl_gettext - Module implementing gettext interface for Tcl.
   Copyright (C) 1995 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, December 1995.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libintl.h>
#include <locale.h>
#include <string.h>

/* Data for Tcl interpreter interface.  */
#include "tcl.h"

/* Prototypes for local functions.  */
static int
tcl_gettext (ClientData client_data, Tcl_Interp *interp, int argc,
	     char *argv[]);
static int
tcl_textdomain (ClientData client_data, Tcl_Interp *interp, int argc,
		char *argv[]);
static int
tcl_bindtextdomain (ClientData client_data, Tcl_Interp *interp, int argc,
		    char *argv[]);


/* Initialization functions.  Called from the tclAppInit.c/tkAppInit.c
   or while the dynamic loading with Tcl7.x, x>= 5.  */
int
Gettext_Init (interp)
     Tcl_Interp *interp;
{
  Tcl_CreateCommand (interp, "gettext", tcl_gettext, (ClientData) 0,
		     (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand (interp, "textdomain", tcl_textdomain, (ClientData) 0,
		     (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand (interp, "bindtextdomain", tcl_bindtextdomain,
		     (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

  return TCL_OK;
}


static int
tcl_gettext (client_data, interp, argc, argv)
     ClientData client_data;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
  const char *domainname = NULL;
  int category = LC_MESSAGES;
  const char *msgid;

  /* The pointer which is assigned in the following statement might
     reference an illegal part of the address space.  But we don't use
     this value before we know the pointer is correct.  */
  msgid = argv[1];

  switch (argc)
    {
    case 4:
#ifdef LC_CTYPE
      if (strcmp (argv[3], "LC_CTYPE") == 0)
	category = LC_CTYPE;
      else
#endif
#ifdef LC_COLLATE
	if (strcmp (argv[3], "LC_COLLATE") == 0)
	  category = LC_COLLATE;
	else
#endif
#ifdef LC_MESSAGES
	  if (strcmp (argv[3], "LC_MESSAGES") == 0)
	    category = LC_MESSAGES;
	  else
#endif
#ifdef LC_MONETARY
	    if (strcmp (argv[3], "LC_MONETARY") == 0)
	      category = LC_MONETARY;
	    else
#endif
#ifdef LC_NUMERIC
	      if (strcmp (argv[3], "LC_NUMERIC") == 0)
		category = LC_NUMERIC;
	      else
#endif
#ifdef LC_TIME
		if (strcmp (argv[3], "LC_TIME") == 0)
		  category = LC_TIME;
		else
#endif
		  {
		    interp->result = gettext ("illegal third argument");
		    return TCL_ERROR;
		  }
      /* FALLTHROUGH */

    case 3:
      domainname = argv[1];
      msgid = argv[2];
      /* FALLTHROUGH */

    case 2:
      interp->result = dcgettext (domainname, msgid, category);
      break;

    default:
      interp->result = gettext ("wrong number of arguments");
      return TCL_ERROR;
    }

  return TCL_OK;
}


static int
tcl_textdomain (client_data, interp, argc, argv)
     ClientData client_data;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
  if (argc != 2)
    {
      interp->result = gettext ("wrong number of arguments");
      return TCL_ERROR;
    }

  interp->result = textdomain (argv[1]);

  return TCL_OK;
}


static int
tcl_bindtextdomain (client_data, interp, argc, argv)
     ClientData client_data;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
  if (argc != 3)
    {
      interp->result = gettext ("wrong number of arguments");
      return TCL_ERROR;
    }

  return bindtextdomain (argv[1], argv[2]) == NULL ? TCL_ERROR : TCL_OK;
}
