#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/04 20:15:30 $
 * $Revision: 1.2 $
 */

/*
 * Declare file local prototypes.
 */
static void CDKEntryCallBack (CDKENTRY *entry, chtype character);
static void drawCDKEntryField (CDKENTRY *entry);

/*
 * Declare file local variables.
 */
extern char *GPasteBuffer;

DeclareCDKObjects(my_funcs,Entry);

/*
 * This creates a pointer to an entry widget.
 */
CDKENTRY *newCDKEntry (CDKSCREEN *cdkscreen, int xplace, int yplace, char *title, char *label, chtype fieldAttr, chtype filler, EDisplayType dispType, int fWidth, int min, int max, boolean Box, boolean shadow)
{
   /* Set up some variables. */
   CDKENTRY *entry	= newCDKObject(CDKENTRY, &my_funcs);
   chtype *holder	= 0;
   int parentWidth	= getmaxx(cdkscreen->window);
   int parentHeight	= getmaxy(cdkscreen->window);
   int fieldWidth	= fWidth;
   int boxWidth		= 0;
   int boxHeight	= 3;
   int maxWidth		= INT_MIN;
   int xpos		= xplace;
   int ypos		= yplace;
   int junk		= 0;
   int horizontalAdjust = 0;
   char **temp		= 0;
   int x, len, junk2;

  /*
   * If the fieldWidth is a negative value, the fieldWidth will
   * be COLS-fieldWidth, otherwise, the fieldWidth will be the
   * given width.
   */
   fieldWidth = setWidgetDimension (parentWidth, fieldWidth, 0);
   boxWidth = fieldWidth + 2;

   /* Set some basic values of the entry field. */
   entry->label		= 0;
   entry->labelLen	= 0;
   entry->labelWin	= 0;
   entry->titleLines	= 0;

   /* Translate the label char *pointer to a chtype pointer. */
   if (label != 0)
   {
      entry->label = char2Chtype (label, &entry->labelLen, &junk);
      boxWidth += entry->labelLen;
   }

   /* Translate the char * items to chtype * */
   if (title != 0)
   {
      /* We need to split the title on \n. */
      temp = CDKsplitString (title, '\n');
      entry->titleLines = CDKcountStrings (temp);

      /* We need to determine the widest title line. */
      for (x=0; x < entry->titleLines; x++)
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
      for (x=0; x < entry->titleLines; x++)
      {
	 entry->title[x]	= char2Chtype (temp[x], &entry->titleLen[x], &entry->titlePos[x]);
	 entry->titlePos[x]	= justifyString (boxWidth - 2, entry->titleLen[x], entry->titlePos[x]);
      }
      CDKfreeStrings(temp);
   }
   else
   {
      /* No title? Set the required variables. */
      entry->titleLines = 0;
   }
   boxHeight += entry->titleLines;

  /*
   * Make sure we didn't extend beyond the dimensions of the window.
   */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);
   fieldWidth = (fieldWidth > (boxWidth - entry->labelLen - 2) ? (boxWidth - entry->labelLen - 2) : fieldWidth);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the label window. */
   entry->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   /* Is the window null? */
   if (entry->win == 0)
   {
      /* Clean up the pointers. */
      freeChtype (entry->label);
      free (entry);

      /* Exit with null. */
      return (0);
   }
   keypad (entry->win, TRUE);

   if (entry->titleLines > 0)
   {
      /* Make the title window. */
      entry->titleWin = subwin (entry->win,
				entry->titleLines, boxWidth - 2,
				ypos + 1, xpos + 1);
   }

   /* Make the label win, if we need to. */
   if (label != 0)
   {
      entry->labelWin = subwin (entry->win, 1, entry->labelLen,
				ypos + entry->titleLines + 1,
				xpos + horizontalAdjust + 1);
   }

   /* Make the field window. */
   entry->fieldWin = subwin (entry->win, 1, fieldWidth,
				ypos + entry->titleLines + 1,
				xpos + entry->labelLen + horizontalAdjust + 1);
   keypad (entry->fieldWin, TRUE);

   /* Make room for the info char * pointer. */
   entry->infoWidth = max + 3;
   entry->info = (char *)malloc (entry->infoWidth);
   cleanChar (entry->info, entry->infoWidth, '\0');

   /* Set up the rest of the structure. */
   ScreenOf(entry)		= cdkscreen;
   entry->parent		= cdkscreen->window;
   entry->fieldAttr		= fieldAttr;
   entry->fieldWidth		= fieldWidth;
   entry->filler		= filler;
   entry->hidden		= filler;
   ObjOf(entry)->box		= Box;
   entry->shadow		= shadow;
   entry->screenCol		= 0;
   entry->leftChar		= 0;
   entry->min			= min;
   entry->max			= max;
   entry->boxWidth		= boxWidth;
   entry->boxHeight		= boxHeight;
   entry->ULChar		= ACS_ULCORNER;
   entry->URChar		= ACS_URCORNER;
   entry->LLChar		= ACS_LLCORNER;
   entry->LRChar		= ACS_LRCORNER;
   entry->HChar			= ACS_HLINE;
   entry->VChar			= ACS_VLINE;
   entry->BoxAttrib		= A_NORMAL;
   entry->exitType		= vNEVER_ACTIVATED;
   entry->dispType		= dispType;
   entry->callbackfn		= (void *)&CDKEntryCallBack;
   entry->preProcessFunction	= 0;
   entry->preProcessData	= 0;
   entry->postProcessFunction	= 0;
   entry->postProcessData	= 0;

   /* Clean the key bindings. */
   cleanCDKObjectBindings (vENTRY, entry);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vENTRY, entry);

   /* Return the pointer to the structure. */
   return (entry);
}

/*
 * This means you want to use the given entry field. It takes input
 * from the keyboard, and when its done, it fills the entry info
 * element of the structure with what was typed.
 */
char *activateCDKEntry (CDKENTRY *entry, chtype *actions)
{
   /* Declare local variables. */
   chtype input = 0;
   char *ret	= 0;

   /* Draw the widget. */
   drawCDKEntry (entry, ObjOf(entry)->box);

   /* Check if 'actions' is null. */
   if (actions == 0)
   {
      for (;;)
      {
	 /* Get the input. */
	 wrefresh (entry->fieldWin);
	 input = wgetch (entry->fieldWin);

	 /* Inject the character into the widget. */
	 ret = injectCDKEntry (entry, input);
	 if (entry->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }
   else
   {
      int length = chlen (actions);
      int x =0;

      /* Inject each character one at a time. */
      for (x=0; x < length; x++)
      {
	 ret = injectCDKEntry (entry, actions[x]);
	 if (entry->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }

   /* Make sure we return the correct info. */
   if (entry->exitType == vNORMAL)
   {
      return entry->info;
   }
   else
   {
      return 0;
   }
}

/*
 * This injects a single character into the widget.
 */
char *injectCDKEntry (CDKENTRY *entry, chtype input)
{
   /* Declare local variables. */
   int ppReturn = 1;
   int temp, x, charCount, stringLen;
   char holder;

   /* Check if there is a pre-process function to be called. */
   if (entry->preProcessFunction != 0)
   {
      /* Call the pre-process function. */
      ppReturn = ((PROCESSFN)(entry->preProcessFunction)) (vENTRY, entry, entry->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check a predefined binding... */
      if (checkCDKObjectBind (vENTRY, entry, input) != 0)
      {
	 entry->exitType = vEARLY_EXIT;
	 return 0;
      }
      else
      {
	 switch (input)
	 {
	    case KEY_UP :
	    case KEY_DOWN :
		 Beep();
		 break;

	    case CDK_BEGOFLINE :
		 entry->leftChar = 0;
		 entry->screenCol = 0;
		 break;

	    case CDK_TRANSPOSE :
		 stringLen = (int)strlen (entry->info);
		 temp = entry->leftChar + entry->screenCol;
		 if (temp >= stringLen-1)
		 {
		    Beep();
		 }
		 else
		 {
		    holder = entry->info[temp];
		    entry->info[temp] = entry->info[temp + 1];
		    entry->info[temp + 1] = holder;
		 }
		 break;

	    case CDK_ENDOFLINE :
		 stringLen = (int)strlen (entry->info);
		 if (stringLen >= entry->fieldWidth)
		 {
		    charCount = (int)(entry->fieldWidth * .8);
		    entry->leftChar = stringLen - charCount;
		    entry->screenCol = charCount;
		 }
		 else
		 {
		    entry->leftChar = 0;
		    entry->screenCol = stringLen;
		 }
		 break;

	    case KEY_LEFT :
	    case CDK_BACKCHAR :
		 if (entry->screenCol == 0)
		 {
		    if (entry->leftChar == 0)
		    {
		       Beep();
		    }
		    else
		    {
		       /* Scroll left.	*/
		       entry->leftChar--;
		    }
		 }
		 else
		 {
		    --entry->screenCol;
		 }
		 break;

	    case KEY_RIGHT :
	    case CDK_FORCHAR :
		 temp = (int)strlen (entry->info);
		 if (entry->screenCol == entry->fieldWidth-1)
		 {
		    if ((entry->leftChar + entry->screenCol + 1) == temp)
		    {
		       Beep();
		    }
		    else
		    {
		       /* Scroll to the right. */
		       entry->leftChar++;
		    }
		 }
		 else
		 {
		    /* Move right. */
		    if ((entry->leftChar + entry->screenCol) == temp)
		    {
		       Beep();
		    }
		    else
		    {
		       ++entry->screenCol;
		    }
		 }
		 break;

	    case DELETE :
	    case CONTROL('H') :
	    case KEY_BACKSPACE :
	    case KEY_DC :
		 if (entry->dispType == vVIEWONLY)
		 {
		    Beep();
		 }
		 else
		 {
		    /* Get the length of the widget information. */
		    int infoLength = (int)strlen (entry->info);
		    if ((entry->leftChar + entry->screenCol) < infoLength)
		    {
		       /* We are deleteing from inside the string. */
		       int currPos = entry->screenCol + entry->leftChar;
		       for (x=currPos; x < infoLength; x++)
		       {
			  entry->info[x] = entry->info[x + 1];
		       }
		       entry->info[infoLength] = '\0';
		    }
		    else
		    {
		       /* We are deleting from the end of the string. */
		       if (infoLength > 0)
		       {
			  /* Update the character pointer. */
			  entry->info[infoLength-1] = '\0';
			  infoLength--;

			  /* Adjust the cursor. */
			  if (entry->screenCol > 0)
			  {
			     entry->screenCol--;
			  }

			  /*
			   * If we deleted the last character on the
			   * screen and the information has scrolled,
			   * adjust the entry field to show the info.
			   */
			  if (entry->leftChar > 0 && entry->screenCol == 1)
			  {
			     if (infoLength < entry->fieldWidth-1)
			     {
				entry->leftChar = 0;
				entry->screenCol = infoLength;
			     }
			     else
			     {
				entry->leftChar -= (entry->fieldWidth-3);
				entry->screenCol = entry->fieldWidth-2;
			     }
			  }
		       }
		       else
		       {
			  Beep();
		       }
		    }
		 }
		 break;

	    case KEY_ESC :
		 entry->exitType = vESCAPE_HIT;
		 return 0;

	    case CDK_ERASE :
		 if ((int)strlen(entry->info) != 0)
		 {
		    cleanCDKEntry (entry);
		 }
		 break;

	    case CDK_CUT:
		 if ((int)strlen(entry->info) != 0)
		 {
		    freeChar (GPasteBuffer);
		    GPasteBuffer = copyChar (entry->info);
		    cleanCDKEntry (entry);
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case CDK_COPY:
		 if ((int)strlen(entry->info) != 0)
		 {
		    freeChar (GPasteBuffer);
		    GPasteBuffer = copyChar (entry->info);
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case CDK_PASTE:
		 if (GPasteBuffer != 0)
		 {
		    setCDKEntryValue (entry, GPasteBuffer);
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_RETURN : case KEY_TAB : case KEY_ENTER : case KEY_CR :
		 if ((int)strlen (entry->info) >= entry->min)
		 {
		    entry->exitType = vNORMAL;
		    return (entry->info);
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(entry));
		 refreshCDKScreen (ScreenOf(entry));
		 break;

	    default :
		 ((ENTRYCB)entry->callbackfn)(entry, input);
		 break;
	 }
      }

      /* Should we do a post-process? */
      if (entry->postProcessFunction != 0)
      {
	 ((PROCESSFN)(entry->postProcessFunction)) (vENTRY, entry, entry->postProcessData, input);
      }
   }

   /* Refresh the entry field. */
   drawCDKEntryField (entry);

   /* Return and indicate that we exited early. */
   entry->exitType = vEARLY_EXIT;
   return 0;
}

/*
 * This moves the entry field to the given location.
 */
static void _moveCDKEntry (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKENTRY *entry = (CDKENTRY *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(entry->win);
      yplace += getbegy(entry->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(entry), &xplace, &yplace, entry->boxWidth, entry->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(entry->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKEntry (entry, ObjOf(entry)->box);
   }
}

/*
 * This is a generic character parser for the entry field. It is used as a
 * callback function, so any personal modifications can be made by creating
 * a new function and calling the activation with its name.
 */
static void CDKEntryCallBack (CDKENTRY *entry, chtype character)
{
   /* Declare local variables. */
   char plainchar = (character & A_CHARTEXT);
   int temp, x;

   /* Start checking the input. */
   if ((entry->dispType == vINT ||
	entry->dispType == vHINT) &&
	!isdigit((int)plainchar))
   {
      Beep();
   }
   else if ((entry->dispType == vCHAR ||
		entry->dispType == vUCHAR ||
		entry->dispType == vLCHAR ||
		entry->dispType == vUHCHAR ||
		entry->dispType == vLHCHAR) &&
		isdigit((int)plainchar))
   {
      Beep();
   }
   else if (entry->dispType == vVIEWONLY)
   {
      Beep();
   }
   else
   {
      if ((int)strlen (entry->info) == entry->max)
      {
	 Beep();
      }
      else
      {
	 /* We will make any adjustments to the case of the character. */
	 if ((entry->dispType == vUCHAR ||
		entry->dispType == vUHCHAR ||
		entry->dispType == vUMIXED ||
		entry->dispType == vUHMIXED)
		&& !isdigit((int)plainchar))
	 {
	    plainchar = toupper (plainchar);
	 }
	 else if ((entry->dispType == vLCHAR ||
			entry->dispType == vLHCHAR ||
			entry->dispType == vLMIXED ||
			entry->dispType == vLHMIXED) &&
			!isdigit((int)plainchar))
	 {
	    plainchar = tolower (plainchar);
	 }

	 /* Update the screen and pointer. */
	 if (entry->screenCol != entry->fieldWidth-1)
	 {
	    /* Update the character pointer. */
	    temp = (int)strlen (entry->info);
	    for (x=temp; x > entry->screenCol + entry->leftChar; x--)
	    {
	       entry->info[x] = entry->info[x-1];
	    }
	    entry->info[entry->screenCol + entry->leftChar] = plainchar;
	    entry->screenCol++;
	 }
	 else
	 {
	    /* Update the character pointer. */
	    temp = (int)strlen (entry->info);
	    entry->info[temp]	= plainchar;
	    entry->info[temp + 1] = '\0';
	    entry->leftChar++;
	 }
      }
   }
}

/*
 * This erases the information in the entry field
 * and redraws a clean and empty entry field.
 */
void cleanCDKEntry (CDKENTRY *entry)
{
   /* Erase the information in the character pointer. */
   cleanChar (entry->info, entry->infoWidth, '\0');

   /* Clean the entry screen field. */
   mvwhline (entry->fieldWin, 0, 0, entry->filler, entry->fieldWidth);

   /* Reset some variables. */
   entry->screenCol	= 0;
   entry->leftChar	= 0;

   /* Refresh the entry field. */
   wmove (entry->fieldWin, 0, entry->screenCol);
   wnoutrefresh (entry->fieldWin);
   wnoutrefresh (entry->win);
}

/*
 * This draws the entry field.
 */
static void _drawCDKEntry (CDKOBJS *object, boolean Box)
{
   CDKENTRY *entry = (CDKENTRY *)object;
   int x;

   /* Box the widget if asked. */
   if (Box)
   {
      attrbox (entry->win,
		entry->ULChar, entry->URChar,
		entry->LLChar, entry->LRChar,
		entry->HChar,  entry->VChar,
		entry->BoxAttrib,
		entry->shadow);
   }

   if (entry->titleLines > 0)
   {
      /* Draw in the title if there is one. */
      for (x=0; x < entry->titleLines; x++)
      {
	 writeChtype (entry->titleWin,
			entry->titlePos[x], x,
			entry->title[x],
			HORIZONTAL, 0,
			entry->titleLen[x]);
      }
      wnoutrefresh (entry->titleWin);
   }

   /* Draw in the label to the widget. */
   if (entry->label != 0)
   {
      writeChtype (entry->labelWin, 0, 0,
			entry->label,
			HORIZONTAL, 0,
			entry->labelLen);
      wnoutrefresh (entry->labelWin);
   }

   /* Redraw the entry field. */
   drawCDKEntryField (entry);
}

/*
 * This redraws the entry field.
 */
static void drawCDKEntryField (CDKENTRY *entry)
{
   /* Declare variables. */
   int infoLength;

   /* Draw in the filler characters.*/
   mvwhline (entry->fieldWin, 0, 0, entry->filler, entry->fieldWidth);

   /* If there is information in the field. Then draw it in. */
   if (entry->info != 0)
   {
      infoLength = (int)strlen (entry->info);

      /* Redraw the field. */
      if (entry->dispType == vHINT ||
		entry->dispType == vHCHAR ||
		entry->dispType == vHMIXED ||
		entry->dispType == vUHCHAR ||
		entry->dispType == vLHCHAR ||
		entry->dispType == vUHMIXED ||
		entry->dispType == vLHMIXED)
      {
	 mvwhline (entry->fieldWin, 0, 0, entry->hidden,
			infoLength - entry->leftChar);
      }
      else
      {
	 writeCharAttrib (entry->fieldWin, 0, 0, entry->info,
			entry->fieldAttr, HORIZONTAL, entry->leftChar, 
			infoLength);
      }
   }

   /* Refresh the entry field. */
   wmove (entry->fieldWin, 0, entry->screenCol);
   wnoutrefresh (entry->fieldWin);
   wnoutrefresh (entry->win);
}

/*
 * This erases an entry widget from the screen.
 */
static void _eraseCDKEntry (CDKOBJS *object)
{
   CDKENTRY *entry = (CDKENTRY *)object;

   eraseCursesWindow (entry->win);
}

/*
 * This destroys an entry widget.
 */
void destroyCDKEntry (CDKENTRY *entry)
{
   int x;

   /* Erase the object. */
   eraseCDKEntry (entry);

   /* Clear out the character pointers. */
   for (x=0; x < entry->titleLines; x++)
   {
      freeChtype (entry->title[x]);
   }
   freeChtype (entry->label);
   freeChar (entry->info);

   /* Delete the windows. */
   deleteCursesWindow (entry->win);

   /* Unregister this object. */
   unregisterCDKObject (vENTRY, entry);

   /* Finish cleaning up. */
   free (entry);
}

/*
 * This sets specific attributes of the entry field.
 */
void setCDKEntry (CDKENTRY *entry, char *value, int min, int max, boolean Box)
{
   setCDKEntryValue (entry, value);
   setCDKEntryMin (entry, min);
   setCDKEntryMax (entry, max);
   setCDKEntryBox (entry, Box);
}

/*
 * This removes the old information in the entry field and keeps the
 * new information given.
 */
void setCDKEntryValue (CDKENTRY *entry, char *newValue)
{
   /* Declare local variables. */
   int copychars	= 0;
   int stringLen	= 0;
   int charCount	= 0;

   /* If the pointer sent in is the same pointer as before, do nothing. */
   if (entry->info == newValue)
   {
      return;
   }

   /* Just to be sure, if lets make sure the new value isn't null. */
   if (newValue == 0)
   {
      /* Then we want to just erase the old value. */
      cleanChar (entry->info, entry->infoWidth, '\0');

      /* Set the pointers back to zero. */
      entry->leftChar = 0;
      entry->screenCol = 0;
      return;
   }

   /* Determine how many characters we need to copy. */
   copychars = MINIMUM ((int)strlen(newValue), entry->max);

   /* OK, erase the old value, and copy in the new value. */
   cleanChar (entry->info, entry->max, '\0');
   strncpy (entry->info, newValue, copychars);
   stringLen = (int)strlen (entry->info);

   /* Now determine the values of leftChar and screenCol. */
   if (stringLen >= entry->fieldWidth)
   {
      charCount		= (int)(entry->fieldWidth * .8);
      entry->leftChar	= stringLen - charCount;
      entry->screenCol	= charCount;
   }
   else
   {
      entry->leftChar = 0;
      entry->screenCol = stringLen;
   }
}
char *getCDKEntryValue (CDKENTRY *entry)
{
   return entry->info;
}

/*
 * This sets the maximum length of the string that will be accepted.
 */
void setCDKEntryMax (CDKENTRY *entry, int max)
{
   entry->max = max;
}
int getCDKEntryMax (CDKENTRY *entry)
{
   return entry->max;
}

/*
 * This sets the minimum length of the string that will
 * be accepted.
 */
void setCDKEntryMin (CDKENTRY *entry, int min)
{
   entry->min = min;
}
int getCDKEntryMin (CDKENTRY *entry)
{
   return entry->min;
}

/*
 * This sets the filler character to be used in the entry field.
 */
void setCDKEntryFillerChar (CDKENTRY *entry, chtype fillerCharacter)
{
   entry->filler = fillerCharacter;
}
chtype getCDKEntryFillerChar (CDKENTRY *entry)
{
   return entry->filler;
}

/*
 * This sets the character to use when a hidden type is used.
 */
void setCDKEntryHiddenChar (CDKENTRY *entry, chtype hiddenCharacter)
{
   entry->hidden = hiddenCharacter;
}
chtype getCDKEntryHiddenChar (CDKENTRY *entry)
{
   return entry->hidden;
}

/*
 * This sets whether or not the entry field will be boxed.
 */
void setCDKEntryBox (CDKENTRY *entry, boolean Box)
{
   ObjOf(entry)->box = Box;
}
boolean getCDKEntryBox (CDKENTRY *entry)
{
   return ObjOf(entry)->box;
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKEntryULChar (CDKENTRY *entry, chtype character)
{
   entry->ULChar = character;
}
void setCDKEntryURChar (CDKENTRY *entry, chtype character)
{
   entry->URChar = character;
}
void setCDKEntryLLChar (CDKENTRY *entry, chtype character)
{
   entry->LLChar = character;
}
void setCDKEntryLRChar (CDKENTRY *entry, chtype character)
{
   entry->LRChar = character;
}
void setCDKEntryVerticalChar (CDKENTRY *entry, chtype character)
{
   entry->VChar = character;
}
void setCDKEntryHorizontalChar (CDKENTRY *entry, chtype character)
{
   entry->HChar = character;
}
void setCDKEntryBoxAttribute (CDKENTRY *entry, chtype character)
{
   entry->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKEntryBackgroundColor (CDKENTRY *entry, char *color)
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
   wbkgd (entry->win, holder[0]);
   wbkgd (entry->fieldWin, holder[0]);
   if (entry->label != 0)
   {
      wbkgd (entry->labelWin, holder[0]);
   }

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This sets the entry field callback function.
 */
void setCDKEntryCB (CDKENTRY *entry, ENTRYCB callback)
{
   entry->callbackfn = (void *)callback;
}

/*
 * This function sets the pre-process function.
 */
void setCDKEntryPreProcess (CDKENTRY *entry, PROCESSFN callback, void *data)
{
   entry->preProcessFunction = callback;
   entry->preProcessData = data;
}

/*
 * This function sets the post-process function.
 */
void setCDKEntryPostProcess (CDKENTRY *entry, PROCESSFN callback, void *data)
{
   entry->postProcessFunction = callback;
   entry->postProcessData = data;
}
