#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/04 19:58:14 $
 * $Revision: 1.1.1.1 $
 */

DeclareCDKObjects(my_funcs,Dialog);

/*
 * This function creates a dialog widget.
 */
CDKDIALOG *newCDKDialog (CDKSCREEN *cdkscreen, int xplace, int yplace, char **mesg, int rows, char **buttonLabel, int buttonCount, chtype buttonHighlight, boolean separator, boolean Box, boolean shadow)
{
   /* Declare local variables. */
   CDKDIALOG *dialog	= newCDKObject(CDKDIALOG, &my_funcs);
   int boxWidth		= MIN_DIALOG_WIDTH;
   int boxHeight	= rows + 2;
   int xpos		= xplace;
   int ypos		= yplace;
   int maxmessagewidth	= 0;
   int buttonWidth, buttonPos;
   int x, junk2;

   /* Translate the char * message to a chtype * */
   for (x=0; x < rows; x++)
   {
      dialog->info[x]	= char2Chtype (mesg[x], &dialog->infoLen[x], &dialog->infoPos[x]);
      maxmessagewidth	= MAXIMUM (maxmessagewidth, dialog->infoLen[x]);
   }

   /* Translate the button label char * to a chtype * */
   buttonWidth = 0;
   for (x=0; x < buttonCount; x++)
   {
      dialog->buttonLabel[x]	= char2Chtype (buttonLabel[x], &dialog->buttonLen[x], &junk2);
      buttonWidth		+= dialog->buttonLen[x] + 1;
   }
   buttonWidth--;

   /* Determine the final dimensions of the box. */
   boxWidth	= MAXIMUM (boxWidth, maxmessagewidth + 2);
   boxWidth	= MAXIMUM (boxWidth, buttonWidth + 2);

   if (buttonCount > 0)
   {
      boxHeight += separator + 1;
   }

   /* Now we have to readjust the x and y positions. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the dialog window. */
   dialog->win			= newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   /* If we couldn't create the window, we should return a null value. */
   if (dialog->win == 0)
   {
      /* Couldn't create the window. Clean up used memory. */
      for (x=0; x < dialog->messageRows ; x++)
      {
	 freeChtype (dialog->info[x]);
      }
      for (x=0; x < dialog->buttonCount; x++)
      {
	 freeChtype (dialog->buttonLabel[x]);
      }

      /* Remove the memory used by the dialog pointer. */
      free (dialog);

      /* Return a null dialog box. */
      return (0);
   }
   keypad (dialog->win, TRUE);
   leaveok (dialog->win, TRUE);

   /* Make the info window. */
   dialog->infoWin = subwin (dialog->win,
				rows, boxWidth - 2,
				ypos + 1, xpos + 1);

   /* Make the button window. */
   if (buttonCount > 0)
   {
      dialog->buttonWin = subwin (dialog->win,
				1, boxWidth - 2,
				ypos + rows + 2, xpos + 1);
   }

   /* Set up the dialog box attributes. */
   ScreenOf(dialog)		= cdkscreen;
   dialog->parent		= cdkscreen->window;
   dialog->buttonCount		= buttonCount;
   dialog->buttonHighlight	= buttonHighlight;
   dialog->currentButton	= 0;
   dialog->messageRows		= rows;
   dialog->boxHeight		= boxHeight;
   dialog->boxWidth		= boxWidth;
   dialog->separator		= separator;
   dialog->exitType		= vNEVER_ACTIVATED;
   ObjOf(dialog)->box		= Box;
   dialog->shadow		= shadow;
   dialog->ULChar		= ACS_ULCORNER;
   dialog->URChar		= ACS_URCORNER;
   dialog->LLChar		= ACS_LLCORNER;
   dialog->LRChar		= ACS_LRCORNER;
   dialog->HChar		= ACS_HLINE;
   dialog->VChar		= ACS_VLINE;
   dialog->BoxAttrib		= A_NORMAL;
   dialog->preProcessFunction	= 0;
   dialog->preProcessData	= 0;
   dialog->postProcessFunction	= 0;
   dialog->postProcessData	= 0;

   /* Find the button positions. */
   buttonPos = (boxWidth - 2 - buttonWidth) / 2;
   for (x=0; x < buttonCount; x++)
   {
      dialog->buttonPos[x]	= buttonPos;
      buttonPos			+= dialog->buttonLen[x] + 1;
   }

   /* Create the string alignments. */
   for (x=0; x < rows; x++)
   {
      dialog->infoPos[x] = justifyString (boxWidth - 2, dialog->infoLen[x], dialog->infoPos[x]);
   }

   /* Empty the key bindings. */
   cleanCDKObjectBindings (vDIALOG, dialog);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vDIALOG, dialog);

   /* Return the dialog box pointer. */
   return (dialog);
}

/*
 * This lets the user select the button.
 */
int activateCDKDialog (CDKDIALOG *dialog, chtype *actions)
{
   /* Declare local variables. */
   chtype input = 0;
   int ret;

   /* Draw the dialog box. */
   drawCDKDialog (dialog, ObjOf(dialog)->box);

   /* Check if actions is null. */
   if (actions == 0)
   {
      for (;;)
      {
	 /* Get the input. */
	 wrefresh (dialog->win);
	 input = wgetch (dialog->win);

	 /* Inject the character into the widget. */
	 ret = injectCDKDialog (dialog, input);
	 if (dialog->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }
   else
   {
      int length = chlen (actions);
      int x = 0;

      /* Inject each character one at a time. */
      for (x=0; x < length; x++)
      {
	 ret = injectCDKDialog (dialog, actions[x]);
	 if (dialog->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }

   /* Set the exit type and exit. */
   dialog->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This injects a single character into the dialog widget.
 */
int injectCDKDialog (CDKDIALOG *dialog, chtype input)
{
   int firstButton	= 0;
   int lastButton	= dialog->buttonCount - 1;
   int ppReturn		= 1;

   /* Set the exit type. */
   dialog->exitType = vEARLY_EXIT;

   /* Check if there is a pre-process function to be called. */
   if (dialog->preProcessFunction != 0)
   {
      ppReturn = ((PROCESSFN)(dialog->preProcessFunction)) (vDIALOG, dialog, dialog->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check for a key binding. */
      if (checkCDKObjectBind (vDIALOG, dialog, input) != 0)
      {
	 dialog->exitType = vESCAPE_HIT;
	 return -1;
      }
      else
      {
	 switch (input)
	 {
	    case KEY_LEFT : case CDK_PREV :
		 if (dialog->currentButton == firstButton)
		 {
		    dialog->currentButton = lastButton;;
		 }
		 else
		 {
		    dialog->currentButton--;
		 }
		 break;

	    case KEY_RIGHT : case CDK_NEXT : case KEY_TAB : case ' ' :
		 if (dialog->currentButton == lastButton)
		 {
		    dialog->currentButton = firstButton;
		 }
		 else
		 {
		    dialog->currentButton++;
		 }
		 break;

	    case KEY_UP : case KEY_DOWN :
		 Beep();
		 break;

	    case KEY_ESC :
		 dialog->exitType = vESCAPE_HIT;
		 return -1;

	    case KEY_RETURN : case KEY_ENTER :
		 dialog->exitType = vNORMAL;
		 return dialog->currentButton;

	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(dialog));
		 refreshCDKScreen (ScreenOf(dialog));
		 break;

	    default :
		 Beep();
		 break;
	 }
      }

      /* Should we call a post-process? */
      if (dialog->postProcessFunction != 0)
      {
	 ((PROCESSFN)(dialog->postProcessFunction)) (vDIALOG, dialog, dialog->postProcessData, input);
      }
   }

   /* Redraw the buttons. */
   drawCDKDialogButtons (dialog);

   /* Exit the dialog box. */
   dialog->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This moves the dialog field to the given location.
 */
static void _moveCDKDialog (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKDIALOG *dialog = (CDKDIALOG *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(dialog->win);
      yplace += getbegy(dialog->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(dialog), &xplace, &yplace, dialog->boxWidth, dialog->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(dialog->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKDialog (dialog, ObjOf(dialog)->box);
   }
}

/*
 * This function draws the dialog widget.
 */
static void _drawCDKDialog (CDKOBJS *object, boolean Box)
{
   CDKDIALOG *dialog = (CDKDIALOG *)object;
   int x = 0;

   /* Box the widget if they asked. */
   if (Box)
   {
      attrbox (dialog->win,
		dialog->ULChar, dialog->URChar,
		dialog->LLChar, dialog->LRChar,
		dialog->HChar,	dialog->VChar,
		dialog->BoxAttrib,
		dialog->shadow);
   }

   /* Draw in the message. */
   for (x=0; x < dialog->messageRows; x++)
   {
      writeChtype (dialog->infoWin,
			dialog->infoPos[x], x,
			dialog->info[x],
			HORIZONTAL, 0,
			dialog->infoLen[x]);
   }

   wnoutrefresh (dialog->infoWin);

   /* Draw in the buttons. */
   drawCDKDialogButtons (dialog);
}

/*
 * This function destroys the dialog widget.
 */
void destroyCDKDialog (CDKDIALOG *dialog)
{
   /* Declare local variables. */
   int x;

   /* Erase the object. */
   eraseCDKDialog (dialog);

   /* Clean up the char pointers. */
   for (x=0; x < dialog->messageRows; x++)
   {
      freeChtype (dialog->info[x]);
   }
   for (x=0; x < dialog->buttonCount; x++)
   {
      freeChtype (dialog->buttonLabel[x]);
   }

   /* Clean up the windows. */
   deleteCursesWindow (dialog->win);

   /* Unregister this object. */
   unregisterCDKObject (vDIALOG, dialog);

   /* Finish cleaning up. */
   free (dialog);
}

/*
 * This function erases the dialog widget from the screen.
 */
static void _eraseCDKDialog (CDKOBJS *object)
{
   CDKDIALOG *dialog = (CDKDIALOG *)object;

   eraseCursesWindow (dialog->win);
}

/*
 * This sets attributes of the dialog box.
 */
void setCDKDialog (CDKDIALOG *dialog, chtype buttonHighlight, boolean separator, boolean Box)
{
   setCDKDialogHighlight (dialog, buttonHighlight);
   setCDKDialogSeparator (dialog, separator);
   setCDKDialogBox (dialog, Box);
}

/*
 * This sets the highlight attribute for the buttons.
 */
void setCDKDialogHighlight (CDKDIALOG *dialog, chtype buttonHighlight)
{
   dialog->buttonHighlight = buttonHighlight;
}
chtype getCDKDialogHighlight (CDKDIALOG *dialog)
{
   return dialog->buttonHighlight;
}

/*
 * This sets whether or not the dialog box will have a separator line.
 */
void setCDKDialogSeparator (CDKDIALOG *dialog, boolean separator)
{
   dialog->separator = separator;
}
boolean getCDKDialogSeparator (CDKDIALOG *dialog)
{
   return dialog->separator;
}

/*
 * This sets the box attribute of the widget.
 */
void setCDKDialogBox (CDKDIALOG *dialog, boolean Box)
{
   ObjOf(dialog)->box = Box;
}
boolean getCDKDialogBox (CDKDIALOG *dialog)
{
   return ObjOf(dialog)->box;
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKDialogULChar (CDKDIALOG *dialog, chtype character)
{
   dialog->ULChar = character;
}
void setCDKDialogURChar (CDKDIALOG *dialog, chtype character)
{
   dialog->URChar = character;
}
void setCDKDialogLLChar (CDKDIALOG *dialog, chtype character)
{
   dialog->LLChar = character;
}
void setCDKDialogLRChar (CDKDIALOG *dialog, chtype character)
{
   dialog->LRChar = character;
}
void setCDKDialogVerticalChar (CDKDIALOG *dialog, chtype character)
{
   dialog->VChar = character;
}
void setCDKDialogHorizontalChar (CDKDIALOG *dialog, chtype character)
{
   dialog->HChar = character;
}
void setCDKDialogBoxAttribute (CDKDIALOG *dialog, chtype character)
{
   dialog->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKDialogBackgroundColor (CDKDIALOG *dialog, char *color)
{
   chtype *holder = 0;
   int junk1, junk2;

   /* Make sure the color isn't null. */
   if (color == 0)
   {
      return;
   }

   /* Convert the value of the environment variable to a chtype. */
   holder = char2Chtype (color, &junk1, &junk2);

   /* Set the widgets background color. */
   wbkgd (dialog->win, holder[0]);

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This draws the dialog buttons and the separation line.
 */
void drawCDKDialogButtons (CDKDIALOG *dialog)
{
   /* Declare local variables. */
   int x;

   if (dialog->buttonCount > 0)
   {
      for (x=0; x < dialog->buttonCount; x++)
      {
	 if (x == dialog->currentButton)
	 {
	    writeChtypeAttrib (dialog->buttonWin,
			dialog->buttonPos[x], 0,
			dialog->buttonLabel[x],
			dialog->buttonHighlight,
			HORIZONTAL, 0,
			dialog->buttonLen[x]);
	 }
	 else
	 {
	    writeChtype (dialog->buttonWin,
			dialog->buttonPos[x], 0,
			dialog->buttonLabel[x],
			HORIZONTAL, 0,
			dialog->buttonLen[x]);
	 }
      }

      /* Draw the separation line. */
      if (dialog->separator)
      {
	 mvwaddch (dialog->win, dialog->boxHeight-3, 0, ACS_LTEE | dialog->BoxAttrib);
	 mvwhline (dialog->win, dialog->boxHeight-3, 1, ACS_HLINE | dialog->BoxAttrib, dialog->boxWidth-2);
	 mvwaddch (dialog->win, dialog->boxHeight-3, dialog->boxWidth-1, ACS_RTEE | dialog->BoxAttrib);
      }

      wnoutrefresh (dialog->buttonWin);
   }

   wnoutrefresh (dialog->win);
}

/*
 * This function sets the pre-process function.
 */
void setCDKDialogPreProcess (CDKDIALOG *dialog, PROCESSFN callback, void *data)
{
   dialog->preProcessFunction = callback;
   dialog->preProcessData = data;
}

/*
 * This function sets the post-process function.
 */
void setCDKDialogPostProcess (CDKDIALOG *dialog, PROCESSFN callback, void *data)
{
   dialog->postProcessFunction = callback;
   dialog->postProcessData = data;
}
