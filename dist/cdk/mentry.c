#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/04 19:58:02 $
 * $Revision: 1.1.1.1 $
 */

/*
 * Declare file local prototypes.
 */
static void CDKMentryCallBack (CDKMENTRY *mentry, chtype character);

/*
 * Declare file local variables.
 */
extern char *GPasteBuffer;

DeclareCDKObjects(my_funcs,Mentry);

/*
 * This creates a pointer to a muliple line entry widget.
 */
CDKMENTRY *newCDKMentry (CDKSCREEN *cdkscreen, int xplace, int yplace, char *title, char *label, chtype fieldAttr, chtype filler, EDisplayType dispType, int fWidth, int fRows, int logicalRows, int min, boolean Box, boolean shadow)
{
   /* Set up some variables */
   CDKMENTRY *mentry	= newCDKObject(CDKMENTRY, &my_funcs);
   chtype *holder	= 0;
   int parentWidth	= getmaxx(cdkscreen->window);
   int parentHeight	= getmaxy(cdkscreen->window);
   int fieldWidth	= fWidth;
   int fieldRows	= fRows;
   int boxWidth		= 0;
   int boxHeight	= 0;
   int maxWidth		= INT_MIN;
   int horizontalAdjust = 0;
   int xpos		= xplace;
   int ypos		= yplace;
   char **temp		= 0;
   int x, len, junk, junk2;

  /*
   * If the fieldWidth is a negative value, the fieldWidth will
   * be COLS-fieldWidth, otherwise, the fieldWidth will be the
   * given width.
   */
   fieldWidth = setWidgetDimension (parentWidth, fieldWidth, 0);

  /*
   * If the fieldRows is a negative value, the fieldRows will
   * be ROWS-fieldRows, otherwise, the fieldRows will be the
   * given height.
   */
   fieldRows = setWidgetDimension (parentWidth, fieldRows, 0);
   boxHeight = fieldRows + 2;

   /* Set some basic values of the mentry field. */
   mentry->label	= 0;
   mentry->labelLen	= 0;
   mentry->labelWin	= 0;
   mentry->titleLines	= 0;

   /* We need to translate the char * label to a chtype * */
   if (label != 0)
   {
      mentry->label	= char2Chtype (label, &mentry->labelLen, &junk);
   }
   boxWidth = mentry->labelLen + fieldWidth + 2;

   /* Translate the char * items to chtype * */
   if (title != 0)
   {
      temp = CDKsplitString (title, '\n');
      mentry->titleLines = CDKcountStrings (temp);

      /* We need to determine the widest title line. */
      for (x=0; x < mentry->titleLines; x++)
      {
	 holder = char2Chtype (temp[x], &len, &junk2);
	 maxWidth = MAXIMUM (maxWidth, len);
	 freeChtype (holder);
      }

      /*
       * If one of the title lines is wider than the field and the label,
       * the box width will expand to accomodate.
       */
       if (maxWidth > boxWidth)
       {
	  horizontalAdjust = (int)((maxWidth - boxWidth) / 2) + 1;
	  boxWidth = maxWidth + 2;
       }

      /* For each line in the title, convert from char * to chtype * */
      for (x=0; x < mentry->titleLines; x++)
      {
	 mentry->title[x]	= char2Chtype (temp[x], &mentry->titleLen[x], &mentry->titlePos[x]);
	 mentry->titlePos[x]	= justifyString (boxWidth, mentry->titleLen[x], mentry->titlePos[x]);
      }

      CDKfreeStrings(temp);
   }
   else
   {
      /* No title? Set the required variables. */
      mentry->titleLines = 0;
   }
   boxHeight += mentry->titleLines;

  /*
   * Make sure we didn't extend beyond the parent window.
   */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);
   fieldWidth = (fieldWidth > (boxWidth - mentry->labelLen - 2) ? (boxWidth - mentry->labelLen - 2) : fieldWidth);
   fieldRows = (fieldRows > (boxHeight - mentry->titleLines - 2) ? (boxHeight - mentry->titleLines - 2) : fieldRows);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the label window. */
   mentry->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   /* Is the window null??? */
   if (mentry->win == 0)
   {
      /* Free up any memory used. */
      freeChtype (mentry->label);
      free (mentry);

      /* Return a null pointer. */
      return (0);
   }
   keypad (mentry->win, TRUE);

   if (mentry->titleLines > 0)
   {
      /* Make the title window. */
      mentry->titleWin = subwin (mentry->win,
				mentry->titleLines, boxWidth - 2,
				ypos + 1, xpos + 1);
   }

   /* Create the label window. */
   if (mentry->label != 0)
   {
      mentry->labelWin = subwin (mentry->win, fieldRows, mentry->labelLen,
				ypos + mentry->titleLines + 1,
				xpos + horizontalAdjust + 1);
   }

   /* Make the field window. */
   mentry->fieldWin = subwin (mentry->win, fieldRows, fieldWidth,
				ypos + mentry->titleLines + 1,
				xpos + mentry->labelLen + horizontalAdjust + 1);
   keypad (mentry->fieldWin, TRUE);

   /* Set up the rest of the structure. */
   mentry->parent	= cdkscreen->window;
   mentry->totalWidth	= fieldWidth * logicalRows;

   /* Create the info char * pointer. */
   mentry->infoWidth = mentry->totalWidth + 3;
   mentry->info = (char *)malloc (mentry->infoWidth);
   cleanChar (mentry->info, mentry->infoWidth, '\0');

   /* Set up the rest of the widget information. */
   ScreenOf(mentry)		= cdkscreen;
   mentry->fieldAttr		= fieldAttr;
   mentry->fieldWidth		= fieldWidth;
   mentry->rows			= fieldRows;
   mentry->boxHeight		= boxHeight;
   mentry->boxWidth		= boxWidth;
   mentry->filler		= filler;
   mentry->hidden		= filler;
   ObjOf(mentry)->box		= Box;
   mentry->currentRow		= 0;
   mentry->currentCol		= 0;
   mentry->topRow		= 0;
   mentry->shadow		= shadow;
   mentry->dispType		= dispType;
   mentry->min			= min;
   mentry->logicalRows		= logicalRows;
   mentry->ULChar		= ACS_ULCORNER;
   mentry->URChar		= ACS_URCORNER;
   mentry->LLChar		= ACS_LLCORNER;
   mentry->LRChar		= ACS_LRCORNER;
   mentry->HChar		= ACS_HLINE;
   mentry->VChar		= ACS_VLINE;
   mentry->BoxAttrib		= A_NORMAL;
   mentry->exitType		= vNEVER_ACTIVATED;
   mentry->callbackfn		= (void *)&CDKMentryCallBack;
   mentry->preProcessFunction	= 0;
   mentry->preProcessData	= 0;
   mentry->postProcessFunction	= 0;
   mentry->postProcessData	= 0;

   /* Clean the key bindings. */
   cleanCDKObjectBindings (vMENTRY, mentry);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vMENTRY, mentry);

   /* Return the pointer to the structure. */
   return (mentry);
}

/*
 * This actually manages the mentry widget...
 */
char *activateCDKMentry (CDKMENTRY *mentry, chtype *actions)
{
   /* Declare local variables. */
   chtype input = 0;
   char *ret	= 0;

   /* Draw the mentry widget. */
   drawCDKMentry (mentry, ObjOf(mentry)->box);

   /* Check if 'actions' is null. */
   if (actions == 0)
   {
      for (;;)
      {
	 /* Get the input. */
	 wrefresh (mentry->fieldWin);
	 input = wgetch (mentry->fieldWin);

	 /* Inject this character into the widget. */
	 ret = injectCDKMentry (mentry, input);
	 if (mentry->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }
   else
   {
      int length = chlen (actions);
      int x;

      /* Inject each character one at a time. */
      for (x=0; x < length; x++)
      {
	 ret = injectCDKMentry (mentry, actions[x]);
	 if (mentry->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }

   /* Set the exit type and exit. */
   mentry->exitType = vEARLY_EXIT;
   return 0;
}

/*
 * This injects a character into the widget.
 */
char *injectCDKMentry (CDKMENTRY *mentry, chtype input)
{
   /* Declare local variables. */
   int cursorPos	= ((mentry->currentRow + mentry->topRow) *
				mentry->fieldWidth) + mentry->currentCol;
   int ppReturn		= 1;
   int x, infoLength, fieldCharacters;
   char holder;

   /* Check if there is a pre-process function to be called. */
   if (mentry->preProcessFunction != 0)
   {
      /* Call the pre-process function. */
      ppReturn = ((PROCESSFN)(mentry->preProcessFunction)) (vMENTRY, mentry, mentry->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check for a key binding... */
      if (checkCDKObjectBind (vMENTRY, mentry, input) != 0)
      {
	 mentry->exitType = vESCAPE_HIT;
	 return 0;
      }
      else
      {
	 switch (input)
	 {
	    case CDK_BEGOFLINE :
		 mentry->currentCol = 0;
		 mentry->currentRow = 0;
		 mentry->topRow = 0;
		 break;

	    case CDK_ENDOFLINE :
		 infoLength = (int)strlen(mentry->info);
		 fieldCharacters = mentry->rows * mentry->fieldWidth;
		 if (infoLength < fieldCharacters)
		 {
		    mentry->topRow = 0;
		    mentry->currentRow = infoLength / mentry->fieldWidth;
		    mentry->currentCol = infoLength % mentry->fieldWidth;
		 }
		 else
		 {
		    mentry->topRow	= (infoLength / mentry->fieldWidth) - mentry->rows + 1;
		    mentry->currentRow	= mentry->rows - 1;
		    mentry->currentCol	= infoLength % mentry->fieldWidth;
		 }
		 break;

	    case KEY_LEFT : case CDK_BACKCHAR :
		 if (mentry->currentCol != 0)
		 {
		    mentry->currentCol--;
		 }
		 else
		 {
		    if (mentry->currentRow == 0)
		    {
		       if (mentry->topRow == 0)
		       {
			  Beep();
		       }
		       else
		       {
			  /* Move up one row. */
			  mentry->currentCol = mentry->fieldWidth - 1;
			  mentry->topRow--;
		       }
		    }
		    else
		    {
		       mentry->currentRow--;
		       mentry->currentCol = mentry->fieldWidth - 1;
		    }
		 }
		 break;

	    case KEY_RIGHT : case CDK_FORCHAR :
		 if (mentry->currentCol < (mentry->fieldWidth - 1))
		 {
		    if ((((mentry->topRow + mentry->currentRow) * mentry->fieldWidth) + mentry->currentCol + 1) > (int)strlen (mentry->info)-1)
		    {
		       Beep();
		    }
		    else
		    {
		       mentry->currentCol++;
		    }
		 }
		 else
		 {
		    if (mentry->currentRow == mentry->rows - 1)
		    {
		       if ((mentry->topRow + mentry->currentRow + 1) <= mentry->logicalRows)
		       {
			  mentry->currentCol = 0;
			  mentry->topRow++;
		       }
		       else
		       {
			  Beep();
		       }
		    }
		    else
		    {
		       mentry->currentCol = 0;
		       mentry->currentRow++;
		    }
		 }
		 break;

	    case KEY_DOWN :
		 if (mentry->currentRow != (mentry->rows-1))
		 {
		    if ((((mentry->topRow + mentry->currentRow + 1) * mentry->fieldWidth) + mentry->currentCol) > (int)strlen (mentry->info)-1)
		    {
		       Beep();
		    }
		    else
		    {
		       mentry->currentRow++;
		    }
		 }
		 else
		 {
		    if (mentry->topRow >= mentry->logicalRows - mentry->rows)
		    {
		       Beep();
		    }
		    else
		    {
		       infoLength = (int)strlen(mentry->info);
		       if (((mentry->topRow + mentry->currentRow + 1) * mentry->fieldWidth) > infoLength)
		       {
			  Beep();
		       }
		       else
		       {
			  mentry->topRow++;
		       }
		    }
		 }
		 break;

	    case KEY_UP :
		 if (mentry->currentRow != 0)
		 {
		    mentry->currentRow--;
		 }
		 else
		 {
		    if (mentry->topRow == 0)
		    {
		       Beep();
		    }
		    else
		    {
		       mentry->topRow--;
		    }
		 }
		 break;

	    case DELETE : case KEY_BACKSPACE : case CONTROL('H') : case KEY_DC :
		 if (mentry->dispType == vVIEWONLY)
		 {
		    Beep();
		 }
		 else
		 {
		    infoLength	= (int)strlen(mentry->info);

		    /* If there is nothing left to delete, then beep. */
		    if (infoLength == 0)
		    {
		       Beep();
		    }
		    else
		    {
		       /*
			* Check if the cursor is inside the string or
			* at the end of the string.
			 */
		       if (cursorPos != infoLength)
		       {
			  /* We are deleting from the middle of the string.  */
			  for (x = cursorPos; x < infoLength; x++)
			  {
			     mentry->info[x] = mentry->info[x + 1];
			  }
			  mentry->info[--infoLength] = '\0';
		       }
		       else
		       {
			  /* We are deleting from the end of the string. */
			  mentry->info[--infoLength] = '\0';

			  /* Adjust the currentRow/currentCol vars. */
			  if (mentry->currentCol == 0 &&
				mentry->currentRow == 0)
			  {
			     if (mentry->topRow == 0)
			     {
				Beep();
			     }
			     else
			     {
				/* Bump everything down X lines. */
				int rows = infoLength / mentry->fieldWidth;
				int cols = infoLength % mentry->fieldWidth;

				if (rows < mentry->rows)
				{
				   mentry->topRow = 0;
				   mentry->currentCol = cols;
				   mentry->currentRow = rows;
				}
				else
				{
				   mentry->topRow -= mentry->rows;
				   mentry->currentRow = mentry->rows-1;
				   mentry->currentCol = cols;
				}
			     }
			  }
			  else if (mentry->currentCol == 0)
			  {
			     mentry->currentCol = mentry->fieldWidth-1;
			     mentry->currentRow--;
			  }
			  else
			  {
			     mentry->currentCol--;
			  }
		       }
		    }
		 }
		 break;

	    case CDK_TRANSPOSE :
		 infoLength = (int)strlen(mentry->info);
		 if (cursorPos >= infoLength-1)
		 {
		    Beep();
		 }
		 else
		 {
		    holder = mentry->info[cursorPos];
		    mentry->info[cursorPos] = mentry->info[cursorPos + 1];
		    mentry->info[cursorPos + 1] = holder;
		 }
		 break;

	    case CDK_ERASE :
		 infoLength = (int)strlen(mentry->info);
		 if (infoLength != 0)
		 {
		    cleanCDKMentry (mentry);
		 }
		 break;

	    case CDK_CUT:
		 infoLength = (int)strlen(mentry->info);
		 if (infoLength != 0)
		 {
		    freeChar (GPasteBuffer);
		    GPasteBuffer = copyChar (mentry->info);
		    cleanCDKMentry (mentry);
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case CDK_COPY:
		 infoLength = (int)strlen(mentry->info);
		 if (infoLength != 0)
		 {
		    freeChar (GPasteBuffer);
		    GPasteBuffer = copyChar (mentry->info);
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case CDK_PASTE:
		 if (GPasteBuffer != 0)
		 {
		    setCDKMentryValue (mentry, GPasteBuffer);
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_RETURN : case KEY_TAB : case KEY_ENTER :
		 infoLength = (int)strlen(mentry->info);
		 if (infoLength < mentry->min + 1)
		 {
		    Beep();
		 }
		 else
		 {
		    mentry->exitType = vNORMAL;
		    return (mentry->info);
		 }
		 break;

	    case KEY_ESC :
		 mentry->exitType = vESCAPE_HIT;
		 return 0;

	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(mentry));
		 refreshCDKScreen (ScreenOf(mentry));
		 break;

	    default :
		 ((MENTRYCB)mentry->callbackfn)(mentry, input);
		 break;
	 }
      }

      /* Should we do a post-process? */
      if (mentry->postProcessFunction != 0)
      {
	 ((PROCESSFN)(mentry->postProcessFunction)) (vMENTRY, mentry, mentry->postProcessData, input);
      }
   }

   /* Refresh the field. */
   drawCDKMentryField (mentry);

   /* Set the exit type and return. */
   mentry->exitType = vEARLY_EXIT;
   return 0;
}

/*
 * This moves the mentry field to the given location.
 */
static void _moveCDKMentry (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKMENTRY *mentry = (CDKMENTRY *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(mentry->win);
      yplace += getbegy(mentry->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(mentry), &xplace, &yplace, mentry->boxWidth, mentry->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(mentry->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKMentry (mentry, ObjOf(mentry)->box);
   }
}

/*
 * This function redraws the multiple line entry field.
 */
void drawCDKMentryField (CDKMENTRY *mentry)
{
   /* Declare local variables. */
   int currchar		= (mentry->fieldWidth * mentry->topRow);
   int length		= 0;
   int lastpos		= 0;
   int x, y;

   /* Check the value of info. */
   if (mentry->info == 0)
   {
      return;
   }

   /* The information isn't null, redraw the field. */
   length = (int)strlen (mentry->info);
   lastpos = ((chtype)mentry->info[length] == (chtype)mentry->filler ? length-1 : length);

   /* Start redrawing the fields. */
   for (x=0; x < mentry->rows; x++)
   {
      for (y=0; y < mentry->fieldWidth ; y++)
      {
	 if (currchar < lastpos)
	 {
	    if (mentry->dispType == vHCHAR ||
			mentry->dispType == vHINT ||
			mentry->dispType == vHMIXED ||
			mentry->dispType == vUHCHAR ||
			mentry->dispType == vLHCHAR ||
			mentry->dispType == vUHMIXED ||
			mentry->dispType == vLHMIXED)
	    {
	       mvwaddch (mentry->fieldWin, x, y, mentry->filler);
	    }
	    else
	    {
	       mvwaddch (mentry->fieldWin, x, y,
			(mentry->info[currchar++] & A_CHARTEXT) | mentry->fieldAttr);
	    }
	 }
	 else
	 {
	    mvwaddch (mentry->fieldWin, x, y, mentry->filler);
	 }
      }
   }

   /* Refresh the screen. */
   wmove (mentry->fieldWin, mentry->currentRow, mentry->currentCol);
   wnoutrefresh (mentry->fieldWin);
   wnoutrefresh (mentry->win);
}

/*
 * This is a generic character parser for the mentry field. It is used as a
 * callback function, so any personal modifications can be made by creating
 * a new function and calling that one the mentry activation.
 */
static void CDKMentryCallBack (CDKMENTRY *mentry, chtype character)
{
   /* Declare local variables. */
   int cursorPos = ((mentry->currentRow + mentry->topRow) *
			 mentry->fieldWidth) + mentry->currentCol;
   int infoLength = (int)strlen (mentry->info);
   char plainchar = (character & A_CHARTEXT);
   int x;

   /* Check the type of character we are looking for. */
   if ((mentry->dispType == vINT ||
	mentry->dispType == vHINT) &&
	!isdigit((int)plainchar))
   {
      Beep();
   }
   else if ((mentry->dispType == vCHAR ||
		mentry->dispType == vUCHAR ||
		mentry->dispType == vLCHAR ||
		mentry->dispType == vUHCHAR ||
		mentry->dispType == vLHCHAR) &&
		isdigit((int)plainchar))
   {
      Beep();
   }
   else if (mentry->dispType == vVIEWONLY)
   {
      Beep();
   }
   else
   {
      if ((int)strlen (mentry->info) == mentry->totalWidth)
      {
	 Beep();
      }
      else
      {
	 /* We will make any adjustments to the case of the character. */
	 if ((mentry->dispType == vUCHAR ||
		mentry->dispType == vUHCHAR ||
		mentry->dispType == vUMIXED ||
		mentry->dispType == vUHMIXED) &&
		!isdigit((int)plainchar))
	 {
	    plainchar = toupper (plainchar);
	 }
	 else if ((mentry->dispType == vLCHAR ||
			mentry->dispType == vLHCHAR ||
			mentry->dispType == vLMIXED ||
			mentry->dispType == vLHMIXED) &&
			!isdigit((int)plainchar))
	 {
	    plainchar = tolower (plainchar);
	 }

	 /*
	  * Check if we are adding onto the end or we are adding
	  * into the middle?
	  */
	 if (cursorPos != infoLength-1)
	 {
	    /* We are adding in the middle of the string. */
	    for (x = infoLength + 1; x > cursorPos; x--)
	    {
	       mentry->info[x] = mentry->info[x-1];
	    }
	    mentry->info[cursorPos] = plainchar;
	    mentry->currentCol++;
	 }
	 else
	 {
	    /* We are adding onto the end of the string. */
	    mentry->info[infoLength-1]	= plainchar;
	    mentry->info[infoLength]	= mentry->filler;

	    /* Add the character on the screen */
	    if (mentry->dispType == vHCHAR ||
			mentry->dispType == vHINT ||
			mentry->dispType == vHMIXED ||
			mentry->dispType == vUHCHAR ||
			mentry->dispType == vLHCHAR ||
			mentry->dispType == vUHMIXED ||
			mentry->dispType == vLHMIXED)
	    {
	       mvwaddch (mentry->fieldWin,
			mentry->currentRow,
			mentry->currentCol++,
			mentry->filler);
	    }
	    else
	    {
	       mvwaddch (mentry->fieldWin,
			mentry->currentRow,
			mentry->currentCol++,
			plainchar|mentry->fieldAttr);
	    }
	 }

	 /* Have we gone out of bounds. */
	 if (mentry->currentCol == mentry->fieldWidth)
	 {
	    /* Update the row and col values. */
	    mentry->currentCol = 0;
	    mentry->currentRow++;

	    /*
	     * If we have gone outside of the visual boundries, we
	     * need to scroll the window.
	     */
	    if (mentry->currentRow == mentry->rows)
	    {
	       /* We have to redraw the screen. */
	       mentry->currentRow--;
	       mentry->topRow++;
	       drawCDKMentryField (mentry);
	    }
	 }
      }
   }
}

/*
 * This function draws the multiple line entry field.
 */
static void _drawCDKMentry (CDKOBJS *object, boolean Box)
{
   CDKMENTRY *mentry = (CDKMENTRY *)object;
   int x;

   /* Box the widget if asked. */
   if (Box)
   {
      attrbox (mentry->win,
		mentry->ULChar, mentry->URChar,
		mentry->LLChar, mentry->LRChar,
		mentry->HChar,	mentry->VChar,
		mentry->BoxAttrib,
		mentry->shadow);
   }

   if (mentry->titleLines > 0)
   {
      /* Draw in the title if there is one. */
      for (x=0; x < mentry->titleLines; x++)
      {
	 writeChtype (mentry->titleWin,
			mentry->titlePos[x], x,
			mentry->title[x],
			HORIZONTAL, 0,
			mentry->titleLen[x]);
      }
      wnoutrefresh (mentry->titleWin);
   }

   /* Draw in the label to the widget. */
   if (mentry->label != 0)
   {
      writeChtype (mentry->labelWin, 0, 0,
			mentry->label,
			HORIZONTAL, 0,
			mentry->labelLen);
      wnoutrefresh (mentry->labelWin);
   }

   /* Draw the mentry field. */
   drawCDKMentryField (mentry);
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKMentryULChar (CDKMENTRY *mentry, chtype character)
{
   mentry->ULChar = character;
}
void setCDKMentryURChar (CDKMENTRY *mentry, chtype character)
{
   mentry->URChar = character;
}
void setCDKMentryLLChar (CDKMENTRY *mentry, chtype character)
{
   mentry->LLChar = character;
}
void setCDKMentryLRChar (CDKMENTRY *mentry, chtype character)
{
   mentry->LRChar = character;
}
void setCDKMentryVerticalChar (CDKMENTRY *mentry, chtype character)
{
   mentry->VChar = character;
}
void setCDKMentryHorizontalChar (CDKMENTRY *mentry, chtype character)
{
   mentry->HChar = character;
}
void setCDKMentryBoxAttribute (CDKMENTRY *mentry, chtype character)
{
   mentry->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKMentryBackgroundColor (CDKMENTRY *mentry, char *color)
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
   wbkgd (mentry->win, holder[0]);
   wbkgd (mentry->fieldWin, holder[0]);
   if (mentry->label != 0)
   {
      wbkgd (mentry->labelWin, holder[0]);
   }

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This function erases the multiple line entry field from the screen.
 */
static void _eraseCDKMentry (CDKOBJS *object)
{
   CDKMENTRY *mentry = (CDKMENTRY *)object;

   eraseCursesWindow (mentry->win);
}

/*
 * This function destroys a multiple line entry field widget.
 */
void destroyCDKMentry (CDKMENTRY *mentry)
{
   int x;

   /* Erase the object. */
   eraseCDKMentry (mentry);

   /* Clean up the char pointers. */
   for (x=0; x < mentry->titleLines; x++)
   {
      freeChtype (mentry->title[x]);
   }
   freeChtype (mentry->label);
   freeChar (mentry->info);

   /* Clean up the windows. */
   deleteCursesWindow (mentry->win);

   /* Unregister this object. */
   unregisterCDKObject (vMENTRY, mentry);

   /* Finish cleaning up. */
   free (mentry);
}

/*
 * This sets multiple attributes of the widget.
 */
void setCDKMentry (CDKMENTRY *mentry, char *value, int min, boolean Box)
{
   setCDKMentryValue (mentry, value);
   setCDKMentryMin (mentry, min);
   setCDKMentryBox (mentry, Box);
}

/*
 * This removes the old information in the entry field and keeps the
 * new information given.
 */
void setCDKMentryValue (CDKMENTRY *mentry, char *newValue)
{
   /* Declare local variables. */
   int fieldCharacters	= mentry->rows * mentry->fieldWidth;
   int len		= 0;
   int rowsUsed;

   /* Just to be sure, if lets make sure the new value isn't null. */
   if (newValue == 0)
   {
      /* Then we want to just erase the old value. */
      cleanChar (mentry->info, mentry->infoWidth, '\0');
      return;
   }

   /* Determine how many characters we need to copy. */
   len		= (int)strlen (newValue);

   /* OK, erase the old value, and copy in the new value. */
   cleanChar (mentry->info, mentry->infoWidth, '\0');
   strncpy (mentry->info, newValue, mentry->infoWidth);

   /* Set the cursor/row info. */
   if (len < fieldCharacters)
   {
      mentry->topRow = 0;
      mentry->currentRow = len / mentry->fieldWidth;
      mentry->currentCol = len % mentry->fieldWidth;
   }
   else
   {
      rowsUsed			= len / mentry->fieldWidth;
      mentry->topRow		= rowsUsed - mentry->rows + 1;
      mentry->currentRow	= mentry->rows - 1;
      mentry->currentCol	= len % mentry->fieldWidth;
   }

   /* Redraw the widget. */
   drawCDKMentryField (mentry);
}
char *getCDKMentryValue (CDKMENTRY *mentry)
{
   return mentry->info;
}

/*
 * This sets the filler character to use when drawing the widget.
 */
void setCDKMentryFillerChar (CDKMENTRY *mentry, chtype filler)
{
   mentry->filler = filler;
}
chtype getCDKMentryFillerChar (CDKMENTRY *mentry)
{
   return mentry->filler;
}

/*
 * This sets the character to use when a hidden character type is used.
 */
void setCDKMentryHiddenChar (CDKMENTRY *mentry, chtype character)
{
   mentry->hidden = character;
}
chtype getCDKMentryHiddenChar (CDKMENTRY *mentry)
{
   return mentry->hidden;
}

/*
 * This sets the minimum length of the widget.
 */
void setCDKMentryMin (CDKMENTRY *mentry, int min)
{
   mentry->min = min;
}
int getCDKMentryMin (CDKMENTRY *mentry)
{
   return mentry->min;
}

/*
 * This sets the widgets box attribute.
 */
void setCDKMentryBox (CDKMENTRY *mentry, boolean Box)
{
   ObjOf(mentry)->box = Box;
}
boolean getCDKMentryBox (CDKMENTRY *mentry)
{
   return ObjOf(mentry)->box;
}

/*
 * This erases the information in the multiple line entry widget.
 */
void cleanCDKMentry (CDKMENTRY *mentry)
{
   /* Erase the information in the character pointer. */
   cleanChar (mentry->info, mentry->infoWidth, '\0');

   mentry->currentRow	= 0;
   mentry->currentCol	= 0;
   mentry->topRow	= 0;
}

/*
 * This sets the callback function.
 */
void setCDKMentryCB (CDKMENTRY *mentry, MENTRYCB callback)
{
   mentry->callbackfn = (void *)callback;
}

/*
 * This function sets the pre-process function.
 */
void setCDKMentryPreProcess (CDKMENTRY *mentry, PROCESSFN callback, void *data)
{
   mentry->preProcessFunction = callback;
   mentry->preProcessData = data;
}

/*
 * This function sets the post-process function.
 */
void setCDKMentryPostProcess (CDKMENTRY *mentry, PROCESSFN callback, void *data)
{
   mentry->postProcessFunction = callback;
   mentry->postProcessData = data;
}
