#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/09 18:41:54 $
 * $Revision: 1.3 $
 */

/*
 * Declare file local prototypes.
 */
static void drawCDKSwindowList (CDKSWINDOW *swindow);

DeclareCDKObjects(my_funcs,Swindow);

/*
 * This function creates a scrolling window widget.
 */
CDKSWINDOW *newCDKSwindow (CDKSCREEN *cdkscreen, int xplace, int yplace, int height, int width, char *title, int saveLines, boolean Box, boolean shadow)
{
   /* Declare local variables. */
   CDKSWINDOW	*swindow	= newCDKObject(CDKSWINDOW, &my_funcs);
   chtype *holder		= 0;
   int parentWidth		= getmaxx(cdkscreen->window);
   int parentHeight		= getmaxy(cdkscreen->window);
   int boxWidth			= width;
   int boxHeight		= height;
   int maxWidth			= INT_MIN;
   int xpos			= xplace;
   int ypos			= yplace;
   char **temp			= 0;
   int x, len, junk2;

   /*
    * If the height is a negative value, the height will
    * be ROWS-height, otherwise, the height will be the
    * given height.
    */
   boxHeight = setWidgetDimension (parentHeight, height, 0);

   /*
    * If the width is a negative value, the width will
    * be COLS-width, otherwise, the width will be the
    * given width.
    */
   boxWidth = setWidgetDimension (parentWidth, width, 0);

   /* Translate the char * items to chtype * */
   if (title != 0)
   {
      temp = CDKsplitString (title, '\n');
      swindow->titleLines = CDKcountStrings (temp);

      /* We need to determine the widest title line. */
      for (x=0; x < swindow->titleLines; x++)
      {
	 holder = char2Chtype (temp[x], &len, &junk2);
	 maxWidth = MAXIMUM (maxWidth, len);
	 freeChtype (holder);
      }
      boxWidth = MAXIMUM (boxWidth, maxWidth + 2);

      /* For each line in the title, convert from char * to chtype * */
      for (x=0; x < swindow->titleLines; x++)
      {
	 swindow->title[x]	= char2Chtype (temp[x],
					&swindow->titleLen[x],
					&swindow->titlePos[x]);
	 swindow->titlePos[x]	= justifyString (boxWidth - 2,
					swindow->titleLen[x],
					swindow->titlePos[x]);
      }

      CDKfreeStrings(temp);
   }
   else
   {
      /* No title? Set the required variables. */
      swindow->titleLines = 0;
   }

   swindow->fieldWidth  = boxWidth - 2;
   swindow->viewSize	= boxHeight - 2 - swindow->titleLines;

   /*
    * Make sure we didn't extend beyond the dimensions of the window.
    */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the scrolling window */
   swindow->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   /* Is the window null?? */
   if (swindow->win == 0)
   {
      /* Clean up. */
      for (x=0; x < swindow->titleLines; x++)
      {
	 freeChtype (swindow->title[x]);
      }
      free(swindow);

      /* Return a null pointer. */
      return (0);
   }
   keypad (swindow->win, TRUE);
   leaveok (swindow->win, TRUE);

   if (swindow->titleLines > 0)
   {
      /* Make the title window. */
      swindow->titleWin = subwin (swindow->win,
				swindow->titleLines, boxWidth - 2,
				ypos + 1, xpos + 1);
   }

   /* Create the viewing window. */
   swindow->fieldWin = subwin (swindow->win,
				swindow->viewSize, swindow->fieldWidth,
				ypos + swindow->titleLines + 1,
				xpos + 1);

   /* Set the rest of the variables */
   ScreenOf(swindow)		= cdkscreen;
   swindow->parent		= cdkscreen->window;
   swindow->boxHeight		= boxHeight;
   swindow->boxWidth		= boxWidth;
   swindow->currentTop		= 0;
   swindow->maxTopLine		= 0;
   swindow->leftChar		= 0;
   swindow->maxLeftChar		= 0;
   swindow->itemCount		= 0;
   swindow->saveLines		= saveLines;
   swindow->exitType		= vNEVER_ACTIVATED;
   ObjOf(swindow)->box		= Box;
   swindow->shadow		= shadow;
   swindow->preProcessFunction	= 0;
   swindow->preProcessData	= 0;
   swindow->postProcessFunction = 0;
   swindow->postProcessData	= 0;
   swindow->ULChar		= ACS_ULCORNER;
   swindow->URChar		= ACS_URCORNER;
   swindow->LLChar		= ACS_LLCORNER;
   swindow->LRChar		= ACS_LRCORNER;
   swindow->HChar		= ACS_HLINE;
   swindow->VChar		= ACS_VLINE;
   swindow->BoxAttrib		= A_NORMAL;

   /* For each line in the window, set the value to null. */
   for (x=0; x < MAX_LINES; x++)
   {
      swindow->info[x] = 0;
   }

   /* Clean the key bindings. */
   cleanCDKObjectBindings (vSWINDOW, swindow);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vSWINDOW, swindow);

   /* Return the scrolling window */
   return (swindow);
}

/*
 * This sets the lines and the box attribute of the scrolling window.
 */
void setCDKSwindow (CDKSWINDOW *swindow, char **info, int lines, boolean Box)
{
   setCDKSwindowContents (swindow, info, lines);
   setCDKSwindowBox (swindow, Box);
}

/*
 * This sets all the lines inside the scrolling window.
 */
void setCDKSwindowContents (CDKSWINDOW *swindow, char **info, int lines)
{
   /* Declare local variables. */
   int widestItem, x;

   /* First lets clean all the lines in the window. */
   cleanCDKSwindow(swindow);

   /* Now lets set all the lines inside the window. */
   for (x=0; x < lines; x++)
   {
      swindow->info[x]		= char2Chtype (info[x],
					&swindow->infoLen[x],
					&swindow->infoPos[x]);
      swindow->infoPos[x]	= justifyString (swindow->fieldWidth,
					swindow->infoLen[x],
					swindow->infoPos[x]);
      widestItem		= MAXIMUM (widestItem, swindow->infoLen[x]);
   }

   /* Set some of the more important members of the scrolling window. */
   swindow->itemCount	= lines;
   swindow->maxTopLine	= MAXIMUM (0, lines - swindow->viewSize);
   swindow->maxLeftChar = MAXIMUM (0, widestItem - swindow->fieldWidth);
   swindow->currentTop	= 0;
   swindow->leftChar	= 0;
}
chtype **getCDKSwindowContents (CDKSWINDOW *swindow, int *size)
{
   (*size) = swindow->itemCount;
   return swindow->info;
}

/*
 * This sets the box attribute for the widget.
 */
void setCDKSwindowBox (CDKSWINDOW *swindow, boolean Box)
{
   ObjOf(swindow)->box = Box;
}
boolean getCDKSwindowBox (CDKSWINDOW *swindow)
{
   return ObjOf(swindow)->box;
}

/*
 * This adds a line to the scrolling window.
 */
void addCDKSwindow  (CDKSWINDOW *swindow, char *info, int insertPos)
{
   /* Declare variables. */
   int x = 0;

   /*
    * If we are at the maximum number of save lines. Erase
    * the first position and bump everything up one spot.
    */
   if (swindow->itemCount == swindow->saveLines)
   {
      /* Free up the memory. */
      freeChtype (swindow->info[0]);

      /* Bump everything up one spot. */
      for (x=0; x < swindow->itemCount; x++)
      {
	 swindow->info[x] = swindow->info[x + 1];
	 swindow->infoPos[x] = swindow->infoPos[x + 1];
	 swindow->infoLen[x] = swindow->infoLen[x + 1];
      }

      /* Clean out the last position. */
      swindow->info[x] = 0;
      swindow->infoLen[x] = 0;
      swindow->infoPos[x] = 0;
      swindow->itemCount--;
   }

   /* Determine where the line is being added. */
   if (insertPos == TOP)
   {
      /* We need to 'bump' everything down one line... */
      for (x=swindow->itemCount; x > 0; x--)
      {
	 /* Copy in the new row. */
	 swindow->info[x] = swindow->info[x - 1];
	 swindow->infoPos[x] = swindow->infoPos[x - 1];
	 swindow->infoLen[x] = swindow->infoLen[x - 1];
      }

      /* Add it into the scrolling window. */
      swindow->info[0] = char2Chtype (info, &swindow->infoLen[0], &swindow->infoPos[0]);
      swindow->infoPos[0] = justifyString (swindow->fieldWidth, swindow->infoLen[0], swindow->infoPos[0]);

      /* Set some variables. */
      swindow->maxLeftChar = MAXIMUM (swindow->maxLeftChar, swindow->infoLen[0] - swindow->fieldWidth);
      swindow->itemCount++;
      swindow->maxTopLine = MAXIMUM (0, swindow->itemCount - swindow->viewSize);
      swindow->currentTop = 0;
   }
   else
   {
      /* Add to the bottom. */
      swindow->info[swindow->itemCount]	 = char2Chtype (info, &swindow->infoLen[swindow->itemCount], &swindow->infoPos[swindow->itemCount]);
      swindow->infoPos[swindow->itemCount] = justifyString (swindow->fieldWidth, swindow->infoLen[swindow->itemCount], swindow->infoPos[swindow->itemCount]);

      /* Set some variables. */
      swindow->maxLeftChar = MAXIMUM (swindow->maxLeftChar, swindow->infoLen[swindow->itemCount] - swindow->fieldWidth);
      swindow->itemCount++;
      swindow->maxTopLine = MAXIMUM (0, swindow->itemCount - swindow->viewSize);
      swindow->currentTop = swindow->maxTopLine;
   }

   /* Draw in the list. */
   drawCDKSwindowList (swindow);
}

/*
 * This jumps to a given line.
 */
void jumpToLineCDKSwindow (CDKSWINDOW *swindow, int line)
{
  /*
   * Make sure the line is in bounds.
   */
   if (line == BOTTOM || line >= swindow->itemCount)
   {
      /* We are moving to the last page. */
      swindow->currentTop = swindow->maxTopLine;
   }
   else if (line == TOP || line <= 0)
   {
      /* We are moving to the top of the page. */
      swindow->currentTop = 0;
   }
   else
   {
      /* We are moving in the middle somewhere. */
      swindow->currentTop = MINIMUM (line, swindow->maxTopLine);
   }

   /* Redraw the window. */
   drawCDKSwindow (swindow, ObjOf(swindow)->box);
}

/*
 * This removes all the lines inside the scrolling window.
 */
void cleanCDKSwindow (CDKSWINDOW *swindow)
{
   /* Declare local variables. */
   int x;

   /* Clean up the memory used ... */
   for (x=0; x < swindow->itemCount; x++)
   {
      freeChtype (swindow->info[x]);
   }

   /* Reset some variables. */
   swindow->maxLeftChar = 0;
   swindow->itemCount	= 0;
   swindow->maxTopLine	= 0;
   swindow->currentTop	= 0;

   /* Redraw the window. */
   drawCDKSwindow (swindow, ObjOf(swindow)->box);
}

/*
 * This trims lines from the scrolling window.
 */
void trimCDKSwindow (CDKSWINDOW *swindow, int begin, int end)
{
   int start, finish, x;

   /* Check the value of begin. */
   if (begin < 0)
   {
      start = 0;
   }
   else if (begin >= swindow->itemCount)
   {
      start = swindow->itemCount-1;
   }
   else
   {
      start = begin;
   }

   /* Check the value of end. */
   if (end < 0)
   {
      finish = 0;
   }
   else if (end >= swindow->itemCount)
   {
      finish = swindow->itemCount-1;
   }
   else
   {
      finish = end;
   }

   /* Make sure the start is lower than the end. */
   if (start > finish)
   {
      return;
   }

   /* Start nuking elements from the window. */
   for (x=start; x <=finish; x++)
   {
      freeChtype (swindow->info[x]);

      swindow->info[x] = copyChtype (swindow->info[x + 1]);
      swindow->infoPos[x] = swindow->infoPos[x + 1];
      swindow->infoLen[x] = swindow->infoLen[x + 1];
   }

   /* Adjust the item count correctly. */
   swindow->itemCount = swindow->itemCount - (end - begin + 1);

   /* Redraw the window. */
   drawCDKSwindow (swindow, ObjOf(swindow)->box);
}

/*
 * This allows the user to play inside the scolling window.
 */
void activateCDKSwindow (CDKSWINDOW *swindow, chtype *actions)
{
   /* Draw the scrolling list */
   drawCDKSwindow (swindow, ObjOf(swindow)->box);

   /* Check if actions is null. */
   if (actions == 0)
   {
      /* Declare some local variables. */
      chtype input;
      int ret;

      for (;;)
      {
	 /* Get the input. */
	 wrefresh (swindow->win);
	 input = wgetch (swindow->win);

	 /* Inject the character into the widget. */
	 ret = injectCDKSwindow (swindow, input);
	 if (swindow->exitType != vEARLY_EXIT)
	 {
	    return;
	 }
      }
   }
   else
   {
      /* Declare some local variables. */
      int length = chlen (actions);
      int x, ret;

      /* Inject each character one at a time. */
      for (x=0; x < length; x++)
      {
	 ret = injectCDKSwindow (swindow, actions[x]);
	 if (swindow->exitType != vEARLY_EXIT)
	 {
	    return;
	 }
      }
   }

   /* Set the exit type and return. */
   swindow->exitType = vEARLY_EXIT;
   return;
}

/*
 * This injects a single character into the widget.
 */
int injectCDKSwindow (CDKSWINDOW *swindow, chtype input)
{
   /* Declare local variables. */
   int ppReturn = 1;

   /* Set the exit type */
   swindow->exitType = vEARLY_EXIT;

   /* Draw the window.... */
   drawCDKSwindow (swindow, ObjOf(swindow)->box);

   /* Check if there is a pre-process function to be called. */
   if (swindow->preProcessFunction != 0)
   {
      /* Call the pre-process function. */
      ppReturn = ((PROCESSFN)(swindow->preProcessFunction)) (vSWINDOW, swindow, swindow->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check for a key binding. */
      if (checkCDKObjectBind (vSWINDOW, swindow, input) != 0)
      {
	 return -1;
      }
      else
      {
	 switch (input)
	 {
	    case KEY_UP :
		 if (swindow->currentTop > 0)
		 {
		    swindow->currentTop--;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_DOWN :
		 if (swindow->currentTop < swindow->maxTopLine)
		 {
		    swindow->currentTop++;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_RIGHT :
		 if (swindow->leftChar < swindow->maxLeftChar)
		 {
		    swindow->leftChar++;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_LEFT :
		 if (swindow->leftChar > 0)
		 {
		    swindow->leftChar--;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_PPAGE :
	    case CONTROL('B') :
	    case 'b' :
	    case 'B' :
		 if (swindow->currentTop > 0)
		 {
		    if (swindow->currentTop - (swindow->viewSize - 1) >= 0)
		    {
		       swindow->currentTop	-= (swindow->viewSize - 1);
		    }
		    else
		    {
		       swindow->currentTop	= 0;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_NPAGE :
	    case CONTROL('F') :
	    case ' ' :
	    case 'f' :
	    case 'F' :
		 if (swindow->currentTop < swindow->maxTopLine)
		 {
		    if (swindow->currentTop + (swindow->viewSize + 1) <= swindow->maxTopLine)
		    {
		       swindow->currentTop	+= (swindow->viewSize - 1);
		    }
		    else
		    {
		       swindow->currentTop	= swindow->maxTopLine;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case 'g' :
	    case '1' :
	    case KEY_HOME :
		 swindow->currentTop	= 0;
		 break;

	    case 'G' :
	    case KEY_END :
		 swindow->currentTop	= swindow->maxTopLine;
		 break;

	    case '|' :
		 swindow->leftChar	= 0;
		 break;

	    case '$' :
		 swindow->leftChar	= swindow->maxLeftChar;
		 break;

	    case 'l' :
	    case 'L' :
		 loadCDKSwindowInformation (swindow);
		 break;

	    case 's' :
	    case 'S' :
		 saveCDKSwindowInformation (swindow);
		 break;

	    case KEY_ESC :
		 swindow->exitType = vESCAPE_HIT;
		 return -1;

	    case KEY_RETURN :
	    case KEY_TAB :
	    case KEY_ENTER :
	    case KEY_CR :
		 swindow->exitType = vNORMAL;
		 return 1;

	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(swindow));
		 refreshCDKScreen (ScreenOf(swindow));
		 break;

	    default :
		 Beep();
		 break;
	 }
      }

      /* Should we call a post-process? */
      if (swindow->postProcessFunction != 0)
      {
	 ((PROCESSFN)(swindow->postProcessFunction)) (vSWINDOW, swindow, swindow->postProcessData, input);
      }
   }

   /* Redraw the list */
   drawCDKSwindowList (swindow);

   /* Set the exit type and return. */
   swindow->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This moves the swindow field to the given location.
 */
static void _moveCDKSwindow (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKSWINDOW *swindow = (CDKSWINDOW *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(swindow->win);
      yplace += getbegy(swindow->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(swindow), &xplace, &yplace, swindow->boxWidth, swindow->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(swindow->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKSwindow (swindow, ObjOf(swindow)->box);
   }
}

/*
 * This function draws the swindow window widget.
 */
static void _drawCDKSwindow (CDKOBJS *object, boolean Box)
{
   CDKSWINDOW *swindow = (CDKSWINDOW *)object;
   int x;

   /* Box the widget if needed */
   if (Box)
   {
      attrbox (swindow->win,
		swindow->ULChar, swindow->URChar,
		swindow->LLChar, swindow->LRChar,
		swindow->HChar,	 swindow->VChar,
		swindow->BoxAttrib,
		swindow->shadow);
   }

   if (swindow->titleLines > 0)
   {
      /* Draw in the title if there is one */
      for (x=0; x < swindow->titleLines; x++)
      {
	 writeChtype (swindow->titleWin,
			swindow->titlePos[x], x,
			swindow->title[x],
			HORIZONTAL, 0,
			swindow->titleLen[x]);
      }
      wnoutrefresh (swindow->titleWin);
   }

   /* Draw in the list. */
   drawCDKSwindowList (swindow);
}

/*
 * This draws in the contents of the scrolling window.
 */
static void drawCDKSwindowList (CDKSWINDOW *swindow)
{
   /* Declare local variables. */
   int screenPos, x;

   werase (swindow->fieldWin);

   /* Start drawing in each line. */
   for (x=0; x < swindow->viewSize; x++)
   {
      if (x + swindow->currentTop >= swindow->itemCount)
      {
	 break;
      }

      screenPos = swindow->infoPos[x + swindow->currentTop] - swindow->leftChar;

      /* Write in the correct line. */
      if (screenPos >= 0)
      {
	 writeChtype (swindow->fieldWin,
			screenPos, x,
			swindow->info[x + swindow->currentTop],
			HORIZONTAL, 0,
			swindow->infoLen[x + swindow->currentTop]);
      }
      else
      {
	 writeChtype (swindow->fieldWin,
			0, x,
			swindow->info[x + swindow->currentTop],
			HORIZONTAL, -screenPos,
			swindow->infoLen[x + swindow->currentTop]);
      }
   }

   /* Refresh the window. */
   wnoutrefresh (swindow->fieldWin);
   wnoutrefresh (swindow->win);
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKSwindowULChar (CDKSWINDOW *swindow, chtype character)
{
   swindow->ULChar = character;
}
void setCDKSwindowURChar (CDKSWINDOW *swindow, chtype character)
{
   swindow->URChar = character;
}
void setCDKSwindowLLChar (CDKSWINDOW *swindow, chtype character)
{
   swindow->LLChar = character;
}
void setCDKSwindowLRChar (CDKSWINDOW *swindow, chtype character)
{
   swindow->LRChar = character;
}
void setCDKSwindowVerticalChar (CDKSWINDOW *swindow, chtype character)
{
   swindow->VChar = character;
}
void setCDKSwindowHorizontalChar (CDKSWINDOW *swindow, chtype character)
{
   swindow->HChar = character;
}
void setCDKSwindowBoxAttribute (CDKSWINDOW *swindow, chtype character)
{
   swindow->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKSwindowBackgroundColor (CDKSWINDOW *swindow, char *color)
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
   wbkgd (swindow->win, holder[0]);

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This function destroys the scrolling window widget.
 */
void destroyCDKSwindow (CDKSWINDOW *swindow)
{
   /* Declare local variables. */
   int x;

   /* Erase the object. */
   eraseCDKSwindow (swindow);

   /* Clear out the character pointers. */
   for (x=0; x <= swindow->itemCount; x++)
   {
      freeChtype (swindow->info[x]);
   }
   for (x=0; x < swindow->titleLines; x++)
   {
      freeChtype (swindow->title[x]);
   }

   /* Delete the windows. */
   deleteCursesWindow (swindow->win);

   /* Unregister this object. */
   unregisterCDKObject (vSWINDOW, swindow);

   /* Finish cleaning up. */
   free (swindow);
}

/*
 * This function erases the scrolling window widget.
 */
static void _eraseCDKSwindow (CDKOBJS *object)
{
   CDKSWINDOW *swindow = (CDKSWINDOW *)object;

   eraseCursesWindow (swindow->win);
}

/*
 * This exec's a command and redirects the output to the scrolling window.
 */
int execCDKSwindow (CDKSWINDOW *swindow, char *command, int insertPos)
{
   /* Declare local variables. */
   FILE *ps;
   char temp[BUFSIZ];
   int count = 0;
   size_t len;

   /* Try to open the command. */
   if ((ps = popen (command, "r")) == 0)
   {
      return -1;
   }

   /* Start reading. */
   while (fgets (temp, sizeof(temp), ps) != 0)
   {
      len = strlen (temp);
      if (temp[len-1] == '\n')
      {
	 temp[len-1] = '\0';
      }
      /* Add the line to the scrolling window. */
      addCDKSwindow  (swindow, temp, insertPos);
      count++;
   }

   /* Close the pipe. */
   pclose (ps);
   return count;
}

/*
 * This function allows the user to dump the information from the
 * scrollong window to a file.
 */
void saveCDKSwindowInformation (CDKSWINDOW *swindow)
{
   /* Declare local variables. */
   CDKENTRY *entry	= 0;
   char *filename	= 0;
   char temp[256], *mesg[10];
   int linesSaved;

   /* Create the entry field to get the filename. */
   entry = newCDKEntry (ScreenOf(swindow), CENTER, CENTER,
				"<C></B/5>Enter the filename of the save file.",
				"Filename: ",
				A_NORMAL, '_', vMIXED,
				20, 1, 256,
				TRUE, FALSE);

   /* Get the filename. */
   filename = activateCDKEntry (entry, 0);

   /* Did they hit escape? */
   if (entry->exitType == vESCAPE_HIT)
   {
      /* Popup a message. */
      mesg[0] = "<C></B/5>Save Canceled.";
      mesg[1] = "<C>Escape hit. Scrolling window information not saved.";
      mesg[2] = " ";
      mesg[3] = "<C>Press any key to continue.";
      popupLabel (ScreenOf(swindow), mesg, 4);

      /* Clean up and exit. */
      destroyCDKEntry (entry);
      return;
   }

   /* Write the contents of the scrolling window to the file. */
   linesSaved = dumpCDKSwindow (swindow, filename);

   /* Was the save successful? */
   if (linesSaved == -1)
   {
      /* Nope, tell 'em. */
      mesg[0] = copyChar ("<C></B/16>Error");
      mesg[1] = copyChar ("<C>Could not save to the file.");
      sprintf (temp, "<C>(%s)", filename);
      mesg[2] = copyChar (temp);
      mesg[3] = copyChar (" ");
      mesg[4] = copyChar ("<C>Press any key to continue.");
      popupLabel (ScreenOf(swindow), mesg, 5);
      freeCharList (mesg, 5);
   }
   else
   {
      /* Yep, let them know how many lines were saved. */
      mesg[0] = copyChar ("<C></B/5>Save Successful");
      sprintf (temp, "<C>There were %d lines saved to the file", linesSaved);
      mesg[1] = copyChar (temp);
      sprintf (temp, "<C>(%s)", filename);
      mesg[2] = copyChar (temp);
      mesg[3] = copyChar (" ");
      mesg[4] = copyChar ("<C>Press any key to continue.");
      popupLabel (ScreenOf(swindow), mesg, 5);
      freeCharList (mesg, 5);
   }

   /* Clean up and exit. */
   destroyCDKEntry (entry);
   eraseCDKScreen (ScreenOf(swindow));
   drawCDKScreen (ScreenOf(swindow));
}

/*
 * This function allows the user to load new informatrion into the scrolling
 * window.
 */
void loadCDKSwindowInformation (CDKSWINDOW *swindow)
{
   /* Declare local variables. */
   CDKFSELECT *fselect	= 0;
   CDKDIALOG *dialog	= 0;
   char *filename	= 0;
   char temp[256], *mesg[15], *button[5], *fileInfo[MAX_LINES];
   int lines, answer;

   /* Create the file selector to choose the file. */
   fselect = newCDKFselect (ScreenOf(swindow), CENTER, CENTER, 20, 55,
					"<C>Load Which File",
					"Filename: ",
					A_NORMAL, '_',
					A_REVERSE,
					"</5>", "</48>", "</N>", "</N>",
					TRUE, FALSE);

   /* Get the filename to load. */
   filename = activateCDKFselect (fselect, 0);

   /* Make sure they selected a file. */
   if (fselect->exitType == vESCAPE_HIT)
   {
      /* Popup a message. */
      mesg[0] = "<C></B/5>Load Canceled.";
      mesg[1] = " ";
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (ScreenOf(swindow), mesg, 3);

      /* Clean up and exit. */
      destroyCDKFselect (fselect);
      return;
   }

   /* Copy the filename and destroy the file selector. */
   filename = copyChar (fselect->pathname);
   destroyCDKFselect (fselect);

   /*
    * Maye we should check before nuking all the information
    * in the scrolling window...
     */
   if (swindow->itemCount > 0)
   {
      /* Create the dialog message. */
      mesg[0] = "<C></B/5>Save Information First";
      mesg[1] = "<C>There is information in the scrolling window.";
      mesg[2] = "<C>Do you want to save it to a file first?";
      button[0] = "(Yes)";
      button[1] = "(No)";

      /* Create the dialog widget. */
      dialog = newCDKDialog (ScreenOf(swindow), CENTER, CENTER,
				mesg, 2, button, 2,
				COLOR_PAIR(2)|A_REVERSE,
				TRUE, TRUE, FALSE);

      /* Activate the widget. */
      answer = activateCDKDialog (dialog, 0);
      destroyCDKDialog (dialog);

      /* Check the answer. */
      if (answer == -1 || answer == 0)
      {
	 /* Save the information. */
	 saveCDKSwindowInformation (swindow);
      }
   }

   /* Open the file and read it in. */
   lines = readFile (filename, fileInfo, MAX_LINES);
   if (lines == -1)
   {
      /* The file read didn't work. */
      mesg[0] = copyChar ("<C></B/16>Error");
      mesg[1] = copyChar ("<C>Could not read the file");
      sprintf (temp, "<C>(%s)", filename);
      mesg[2] = copyChar (temp);
      mesg[3] = copyChar (" ");
      mesg[4] = copyChar ("<C>Press any key to continue.");
      popupLabel (ScreenOf(swindow), mesg, 5);
      freeCharList (mesg, 5);
      freeChar (filename);
      return;
   }

   /* Clean out the scrolling window. */
   cleanCDKSwindow (swindow);

   /* Set the new information in the scrolling window. */
   setCDKSwindow (swindow, fileInfo, lines, ObjOf(swindow)->box);

   /* Clean up. */
   freeCharList (fileInfo, lines);
   freeChar (filename);
}

/*
 * This actually dumps the information from the scrolling window to a
 * file.
 */
int dumpCDKSwindow (CDKSWINDOW *swindow, char *filename)
{
   /* Declare local variables. */
   FILE *outputFile	= 0;
   char *rawLine	= 0;
   int x;

   /* Try to open the file. */
   if ((outputFile = fopen (filename, "w")) == 0)
   {
      return -1;
   }

   /* Start writing out the file. */
   for (x=0; x < swindow->itemCount; x++)
   {
      rawLine = chtype2Char (swindow->info[x]);
      fprintf (outputFile, "%s\n", rawLine);
      freeChar (rawLine);
   }

   /* Close the file and return the number of lines written. */
   fclose (outputFile);
   return swindow->itemCount;
}

/*
 * This function sets the pre-process function.
 */
void setCDKSwindowPreProcess (CDKSWINDOW *swindow, PROCESSFN callback, void *data)
{
   swindow->preProcessFunction = callback;
   swindow->preProcessData = data;
}

/*
 * This function sets the post-process function.
 */
void setCDKSwindowPostProcess (CDKSWINDOW *swindow, PROCESSFN callback, void *data)
{
   swindow->postProcessFunction = callback;
   swindow->postProcessData = data;
}
