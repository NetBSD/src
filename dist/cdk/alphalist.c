#include <cdk.h>
 
/*
 * $Author: garbled $
 * $Date: 2001/01/04 19:58:30 $
 * $Revision: 1.1.1.1 $
 */
 
/*
 * Declare file local prototypes.
 */
static BINDFN_PROTO(adjustAlphalistCB);
static BINDFN_PROTO(completeWordCB);
static int preProcessEntryField (EObjectType cdktype, void *object, void *clientData, chtype input);

DeclareCDKObjects(my_funcs,Alphalist);

/*
 * This creates the alphalist widget.
 */
CDKALPHALIST *newCDKAlphalist (CDKSCREEN *cdkscreen, int xplace, int yplace, int height, int width, char *title, char *label, char *list[], int listSize, chtype fillerChar, chtype highlight, boolean Box, boolean shadow)
{
   /* Set up some variables. */
   CDKALPHALIST *alphalist	= newCDKObject(CDKALPHALIST, &my_funcs);
   chtype *chtypeLabel		= 0;
   int parentWidth		= getmaxx(cdkscreen->window);
   int parentHeight		= getmaxy(cdkscreen->window);
   int boxWidth			= width;
   int boxHeight		= height;
   int xpos			= xplace;
   int ypos			= yplace;
   int entryWidth		= 0;
   int labelLen			= 0;
   int x, junk2;

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

   /* Translate the label char *pointer to a chtype pointer. */
   if (label != 0)
   {
      chtypeLabel = char2Chtype (label, &labelLen, &junk2);
      freeChtype (chtypeLabel);
   }

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the file selector window. */
   alphalist->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   if (alphalist->win == 0)
   {
      return (0);
   }
   keypad (alphalist->win, TRUE);

   /* Set some variables. */
   ScreenOf(alphalist)		= cdkscreen;
   alphalist->parent		= cdkscreen->window;
   alphalist->highlight		= highlight;
   alphalist->fillerChar	= fillerChar;
   alphalist->boxHeight		= boxHeight;
   alphalist->boxWidth		= boxWidth;
   alphalist->exitType		= vNEVER_ACTIVATED;
   ObjOf(alphalist)->box	= Box;
   alphalist->shadow		= shadow;

   /* We need to sort the list before we use it. */
   quickSort (list, 0, listSize-1);

   /* Copy the list information. */
   for (x=0; x < listSize; x++)
   {
      alphalist->list[x] = copyChar (list[x]);
   }
   alphalist->listSize = listSize;

   /* Create the entry field. */
   entryWidth = boxWidth - (labelLen + 2);
   alphalist->entryField = newCDKEntry (cdkscreen,
					getbegx(alphalist->win),
					getbegy(alphalist->win),
					title, label,
					A_NORMAL, fillerChar, 
					vMIXED, entryWidth, 0, 512,
					Box, FALSE);
   setCDKEntryLLChar (alphalist->entryField, ACS_LTEE);
   setCDKEntryLRChar (alphalist->entryField, ACS_RTEE);

   /* Set the key bindings for the entry field. */
   bindCDKObject (vENTRY, alphalist->entryField, KEY_UP, adjustAlphalistCB, alphalist);
   bindCDKObject (vENTRY, alphalist->entryField, KEY_DOWN, adjustAlphalistCB, alphalist);
   bindCDKObject (vENTRY, alphalist->entryField, KEY_NPAGE, adjustAlphalistCB, alphalist);
   bindCDKObject (vENTRY, alphalist->entryField, CONTROL('F'), adjustAlphalistCB, alphalist);
   bindCDKObject (vENTRY, alphalist->entryField, KEY_PPAGE, adjustAlphalistCB, alphalist);
   bindCDKObject (vENTRY, alphalist->entryField, CONTROL('B'), adjustAlphalistCB, alphalist);
   bindCDKObject (vENTRY, alphalist->entryField, KEY_TAB, completeWordCB, alphalist);

   /* Set up the post-process function for the entry field. */
   setCDKEntryPreProcess (alphalist->entryField, preProcessEntryField, alphalist);

   /* Create the scrolling list. */
   alphalist->scrollField = newCDKScroll (cdkscreen, 
					  getbegx(alphalist->win),
					  getbegy(alphalist->win) + (alphalist->entryField)->titleLines + 2,
					  RIGHT,
					  boxHeight-((alphalist->entryField)->titleLines + 2),
					  boxWidth,
					  0, list, listSize,
					  NONUMBERS, A_REVERSE,
					  Box, FALSE);
   setCDKScrollULChar (alphalist->scrollField, ACS_LTEE);
   setCDKScrollURChar (alphalist->scrollField, ACS_RTEE);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vALPHALIST, alphalist);

   /* Return the file selector pointer. */
   return (alphalist);
}

/*
 * This erases the file selector from the screen.
 */
static void _eraseCDKAlphalist (CDKOBJS *obj)
{
   CDKALPHALIST *alphalist = (CDKALPHALIST *)obj;

   eraseCDKScroll (alphalist->scrollField);
   eraseCDKEntry (alphalist->entryField);

   eraseCursesWindow (alphalist->win);
}

/*
 * This moves the alphalist field to the given location.
 */
static void _moveCDKAlphalist (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKALPHALIST *alphalist = (CDKALPHALIST *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(alphalist->win);
      yplace += getbegy(alphalist->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(alphalist), &xplace, &yplace, alphalist->boxWidth, alphalist->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(alphalist->win, xplace, yplace);

   /* Move the sub-widgets. */
   /* XXXX */
   moveCDKEntry (alphalist->entryField, xplace, yplace, relative, FALSE);
   moveCDKScroll (alphalist->scrollField, xplace, yplace, relative, FALSE);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKAlphalist (alphalist, ObjOf(alphalist)->box);
   }
}

/*
 * This draws the file selector widget.
 */
static void _drawCDKAlphalist (CDKOBJS *obj, boolean Box GCC_UNUSED)
{
   CDKALPHALIST * alphalist = (CDKALPHALIST *)obj;

   /* Draw in the entry field. */
   drawCDKEntry (alphalist->entryField, ObjOf(alphalist->entryField)->box);

   /* Draw in the scroll field. */
   drawCDKScroll (alphalist->scrollField, ObjOf(alphalist->scrollField)->box);
}

/*
 * This activates the file selector.
 */
char *activateCDKAlphalist (CDKALPHALIST *alphalist, chtype *actions)
{
   char *ret = 0;

   /* Draw the widget. */
   drawCDKAlphalist (alphalist, ObjOf(alphalist)->box);

   /* Activate the widget. */
   ret = activateCDKEntry (alphalist->entryField, actions);

   /* Copy the exit type from the entry field. */
   alphalist->exitType = alphalist->entryField->exitType;

   /* Determine the exit status. */
   if (alphalist->exitType != vEARLY_EXIT)
   {
      return ret;
   }
   return 0;
}

/*
 * This injects a single character into the alphalist.
 */
char *injectCDKAlphalist (CDKALPHALIST *alphalist, chtype input)
{
   char *ret = 0;

   /* Draw the widget. */
   drawCDKAlphalist (alphalist, ObjOf(alphalist)->box);

   /* Inject a character into the widget. */
   ret = injectCDKEntry (alphalist->entryField, input);

   /* Copy the exit type from the entry field. */
   alphalist->exitType = alphalist->entryField->exitType;

   /* Determine the exit status. */
   if (alphalist->exitType != vEARLY_EXIT)
   {
      return ret;
   }
   return 0;
}

/*
 * This sets multiple attributes of the widget.
 */
void setCDKAlphalist (CDKALPHALIST *alphalist, char *list[], int listSize, chtype fillerChar, chtype highlight, boolean Box)
{
   setCDKAlphalistContents (alphalist, list, listSize);
   setCDKAlphalistFillerChar (alphalist, fillerChar);
   setCDKAlphalistHighlight (alphalist, highlight);
   setCDKAlphalistBox (alphalist, Box);
}

/*
 * This function sets the information inside the file selector.
 */
void setCDKAlphalistContents (CDKALPHALIST *alphalist, char *list[], int listSize)
{
   /* Declare local variables. */
   CDKSCROLL *scrollp	= (CDKSCROLL *)alphalist->scrollField;
   CDKENTRY *entry	= (CDKENTRY *)alphalist->entryField;
   int x;

   freeCharList (alphalist->list, alphalist->listSize);

   /* We need to sort the list before we use it. */
   quickSort (list, 0, listSize-1);

   /* Copy in the new information. */
   alphalist->listSize		= listSize;
   for (x=0; x < listSize; x++)
   {
      alphalist->list[x] = copyChar (list[x]);
   }

   /* Set the information in the scrolling list. */
   setCDKScroll (scrollp, list, listSize, NONUMBERS, scrollp->highlight, ObjOf(scrollp)->box);

   /* Clean out the entry field. */
   cleanCDKEntry (entry);

   /* Redraw the alphalist. */
   eraseCDKAlphalist (alphalist);
   drawCDKAlphalist (alphalist, ObjOf(alphalist)->box);
}

/*
 * This returns the contents of the alphalist.
 */
char **getCDKAlphalistContents (CDKALPHALIST *alphalist, int *size)
{
   (*size) = alphalist->listSize;
   return alphalist->list;
}

/*
 * This sets the filler character of the entry field of the alphalist.
 */
void setCDKAlphalistFillerChar (CDKALPHALIST *alphalist, chtype fillerCharacter)
{
   CDKENTRY *entry = (CDKENTRY *)alphalist->entryField;

   alphalist->fillerChar = fillerCharacter;

   setCDKEntryFillerChar (entry, fillerCharacter);
}
chtype getCDKAlphalistFillerChar (CDKALPHALIST *alphalist)
{
   return alphalist->fillerChar;
}

/*
 * This sets the highlight bar attributes.
 */
void setCDKAlphalistHighlight (CDKALPHALIST *alphalist, chtype highlight)
{
   alphalist->highlight = highlight;
}
chtype getCDKAlphalistHighlight (CDKALPHALIST *alphalist)
{
   return alphalist->highlight;
}

/*
 * This sets whether or not the widget will be drawn with a box.
 */
void setCDKAlphalistBox (CDKALPHALIST *alphalist, boolean Box)
{
   ObjOf(alphalist)->box = Box;
}

boolean getCDKAlphalistBox (CDKALPHALIST *alphalist)
{
   return ObjOf(alphalist)->box;
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKAlphalistULChar (CDKALPHALIST *alphalist, chtype character)
{
   setCDKEntryULChar (alphalist->entryField, character);
}
void setCDKAlphalistURChar (CDKALPHALIST *alphalist, chtype character)
{
   setCDKEntryURChar (alphalist->entryField, character);
}
void setCDKAlphalistLLChar (CDKALPHALIST *alphalist, chtype character)
{
   setCDKScrollLLChar (alphalist->scrollField, character);
}
void setCDKAlphalistLRChar (CDKALPHALIST *alphalist, chtype character)
{
   setCDKScrollLRChar (alphalist->scrollField, character);
}
void setCDKAlphalistVerticalChar (CDKALPHALIST *alphalist, chtype character)
{
   setCDKEntryVerticalChar (alphalist->entryField, character);
   setCDKScrollVerticalChar (alphalist->scrollField, character);
}
void setCDKAlphalistHorizontalChar (CDKALPHALIST *alphalist, chtype character)
{
   setCDKEntryHorizontalChar (alphalist->entryField, character);
   setCDKScrollHorizontalChar (alphalist->scrollField, character);
}
void setCDKAlphalistBoxAttribute (CDKALPHALIST *alphalist, chtype character)
{
   setCDKEntryBoxAttribute (alphalist->entryField, character);
   setCDKScrollBoxAttribute (alphalist->scrollField, character);
}

/*
 * This sets the background color of the widget.
 */ 
void setCDKAlphalistBackgroundColor (CDKALPHALIST *alphalist, char *color)
{
   if (color != 0)
   {
      setCDKEntryBackgroundColor (alphalist->entryField, color);
      setCDKScrollBackgroundColor (alphalist->scrollField, color);
   }
}

/*
 * This destroys the file selector.	
 */
void destroyCDKAlphalist (CDKALPHALIST *alphalist)
{
   /* Erase the file selector. */
   eraseCDKAlphalist (alphalist);

   freeCharList (alphalist->list, alphalist->listSize);

   /* Destroy the other Cdk objects. */
   destroyCDKEntry (alphalist->entryField);
   destroyCDKScroll (alphalist->scrollField);
 
   /* Free up the window pointers. */
   deleteCursesWindow (alphalist->win);

   /* Unregister the object. */
   unregisterCDKObject (vALPHALIST, alphalist);

   /* Free up the object pointer. */
   free (alphalist);
}

/*
 * This function sets the pre-process function.
 */
void setCDKAlphalistPreProcess (CDKALPHALIST *alphalist, PROCESSFN callback, void *data)
{
   setCDKEntryPreProcess (alphalist->entryField, callback, data);
}
 
/*
 * This function sets the post-process function.
 */
void setCDKAlphalistPostProcess (CDKALPHALIST *alphalist, PROCESSFN callback, void *data)
{
   setCDKEntryPostProcess (alphalist->entryField, callback, data);
}

/*
 * Start of callback functions.
 */
static void adjustAlphalistCB (EObjectType objectType GCC_UNUSED, void *object GCC_UNUSED, void *clientData, chtype key)
{
   CDKALPHALIST *alphalist	= (CDKALPHALIST *)clientData;
   CDKSCROLL *scrollp		= (CDKSCROLL*)alphalist->scrollField;
   CDKENTRY *entry		= (CDKENTRY*)alphalist->entryField;
   char *current		= 0;

   /* Adjust the scrolling list. */
   injectCDKScroll (alphalist->scrollField, (chtype)key);

   /* Set the value in the entry field. */
   current = chtype2Char (scrollp->item[scrollp->currentItem]);
   setCDKEntryValue (entry, current);
   drawCDKEntry (entry, ObjOf(entry)->box);
   freeChar (current);
}

/*
 * This is the heart-beat of the widget.
 */
static int preProcessEntryField (EObjectType cdktype GCC_UNUSED, void *object GCC_UNUSED, void *clientData, chtype input)
{
   CDKALPHALIST *alphalist	= (CDKALPHALIST *)clientData;
   CDKSCROLL *scrollp		= alphalist->scrollField;
   CDKENTRY *entry		= alphalist->entryField;
   int infoLen			= 0;
   int Index;
   char pattern[5000];

   infoLen = (int)strlen (entry->info);

   /* Check the input. */
   if (isalnum (input) || ispunct (input) ||
	input == DELETE || input == CONTROL('H') || input == KEY_DC)
   {
      /* Copy the information from the entry field. */
      strcpy (pattern, entry->info);

      /* Truncate/Concatenate to the information in the entry field. */	 
      if (input == DELETE || input == CONTROL('H') || input == KEY_DC)
      {
	 /* If we had only 1 item in the list; jump back to the top. */
	 if (infoLen <= 1)
	 {
	    setCDKScrollPosition (scrollp, 0);
	    drawCDKScroll (scrollp, ObjOf(scrollp)->box);
	    return 1;
	 }

	 pattern[infoLen - 1] = '\0';
      }
      else
      {
	 pattern[infoLen] = (char)input;
	 pattern[infoLen + 1] = '\0';
      }

      /* Look for the pattern in the list. */
      Index = searchList (alphalist->list, alphalist->listSize, pattern);
      if (Index >= 0)
      {
	 setCDKScrollPosition (scrollp, Index);
	 drawCDKScroll (scrollp, ObjOf(scrollp)->box);
      }
      else
      {
	 Beep();
	 return 0;
      }
   }
   return 1;
}

/*
 * This tries to complete the word in the entry field.
 */
static void completeWordCB (EObjectType objectType GCC_UNUSED, void *object GCC_UNUSED, void *clientData, chtype key GCC_UNUSED)
{
   CDKALPHALIST *alphalist	= (CDKALPHALIST *)clientData;
   CDKENTRY *entry		= (CDKENTRY*)alphalist->entryField;
   CDKSCROLL *scrollp		= 0;
   int currentIndex		= 0;
   int wordLength		= 0;
   int selected			= -1;
   int altCount			= 0;
   int height			= 0;
   int match			= 0;
   int Index			= 0;
   int ret			= 0;
   int x			= 0;
   char *altWords[MAX_LINES];

   if (entry->info == 0)
   {
      Beep();
      return;
   }
   wordLength = (int)strlen (entry->info);

   /* If the word length is equal to zero, just leave. */
   if (wordLength == 0)
   {
      Beep();
      return;
   }

   /* Look for a unique word match. */
   Index = searchList (alphalist->list, alphalist->listSize, entry->info);

   /* If the index is less than zero, return we didn't find a match. */
   if (Index < 0)
   {
      Beep();
      return;
   }

   /* Did we find the last word in the list? */
   if (Index == alphalist->listSize-1)
   {
      setCDKEntryValue (entry, alphalist->list[Index]);
      drawCDKEntry (entry, ObjOf(entry)->box);
      return;
   }

   /* Ok, we found a match, is the next item similar? */
   ret = strncmp (alphalist->list[Index + 1], entry->info, wordLength);
   if (ret == 0)
   {
      currentIndex	= Index;
      altCount		= 0;
      height		= 0;
      match		= 0;
      selected		= -1;

      /* Start looking for alternate words. */
      while ((currentIndex < alphalist->listSize)
	  && (strncmp (alphalist->list[currentIndex], entry->info, wordLength) == 0))
      {
	 altWords[altCount] = copyChar (alphalist->list[currentIndex]);
	 currentIndex++;
	 altCount++;
      }
      
      /* Determine the height of the scrolling list. */
      height = MINIMUM (altCount + 3, 11);

      /* Create a scrolling list of close matches. */
      scrollp = newCDKScroll (entry->obj.screen, CENTER, CENTER, RIGHT, height, -30,
					"<C></B/5>Possible Matches.",
					altWords, altCount, NUMBERS,
					A_REVERSE, TRUE, FALSE);

      /* Allow them to select a close match. */
      match = activateCDKScroll (scrollp, 0);
      selected = scrollp->currentItem;
      
      /* Check how they exited the list. */
      if (scrollp->exitType == vESCAPE_HIT)
      {
	 /* Beep at the user. */
	 Beep();

         /* Destroy the scrolling list. */
         destroyCDKScroll (scrollp);
      }
      else
      {
         /* Destroy the scrolling list. */
         destroyCDKScroll (scrollp);

	 /* Set the entry field to the selected value. */
	 setCDKEntryValue (entry, altWords[match]);

	 /* Move the highlight bar down to the selected value. */
	 for (x=0; x < selected; x++)
	 {
	    injectCDKScroll (alphalist->scrollField, KEY_DOWN);
	 }
      }

      /* Clean up. */
      freeCharList (altWords, altCount);

      /* Redraw the alphalist. */
      drawCDKAlphalist (alphalist, ObjOf(alphalist)->box);
   }
   else
   {
      /* Set the entry field with the found item. */
      setCDKEntryValue (entry, alphalist->list[Index]);
      drawCDKEntry (entry, ObjOf(entry)->box);
   }
}
