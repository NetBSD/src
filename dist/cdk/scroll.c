#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/09 18:41:54 $
 * $Revision: 1.3 $
 */

/*
 * Declare file local prototypes.
 */
static void drawCDKScrollList (CDKSCROLL *scrollp);
static void createCDKScrollItemList (CDKSCROLL *scrollp, boolean numbers,
				char **list, int listSize);

DeclareCDKObjects(my_funcs,Scroll);

/*
 * This function creates a new scrolling list widget.
 */
CDKSCROLL *newCDKScroll (CDKSCREEN *cdkscreen, int xplace, int yplace, int splace, int height, int width, char *title, char **list, int listSize, boolean numbers, chtype highlight, boolean Box, boolean shadow)
{
   /* Declare local variables. */
   CDKSCROLL *scrollp		= newCDKObject(CDKSCROLL, &my_funcs);
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

   if ((splace == LEFT) || (splace == RIGHT))
   {
      scrollp->scrollbar = TRUE;
   }
   else
   {
      scrollp->scrollbar = FALSE;
   }

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
      scrollp->titleLines = CDKcountStrings (temp);

      /* We need to determine the widest title line. */
      for (x=0; x < scrollp->titleLines; x++)
      {
	 holder = char2Chtype (temp[x], &len, &junk2);
	 maxWidth = MAXIMUM (maxWidth, len);
	 freeChtype (holder);
      }
      boxWidth = MAXIMUM (boxWidth, maxWidth + 2);

      /* For each line in the title, convert from char * to chtype * */
      for (x=0; x < scrollp->titleLines; x++)
      {
	 scrollp->title[x]	= char2Chtype (temp[x],
					&scrollp->titleLen[x],
					&scrollp->titlePos[x]);
	 scrollp->titlePos[x]	= justifyString (boxWidth - 2,
					scrollp->titleLen[x],
					scrollp->titlePos[x]);
      }

      CDKfreeStrings(temp);
   }
   else
   {
      /* No title? Set the required variables. */
      scrollp->titleLines = 0;
   }

   /* Set the box height. */
   if (scrollp->titleLines > boxHeight)
   {
      if (listSize > 8)
      {
	 boxHeight = scrollp->titleLines + 10;
      }
      else
      {
	 boxHeight = scrollp->titleLines + listSize + 2;
      }
   }
   scrollp->fieldWidth  = boxWidth - 2 - scrollp->scrollbar;
   scrollp->viewSize	= boxHeight - 2 - scrollp->titleLines;

   /*
    * Make sure we didn't extend beyond the dimensions of the window.
    */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the scrolling window */
   scrollp->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   /* Is the scrolling window null?? */
   if (scrollp->win == 0)
   {
      /* Clean up any memory. */
      for (x=0; x < scrollp->titleLines; x++)
      {
	 freeChtype (scrollp->title[x]);
      }
      free(scrollp);

      /* Return a null pointer. */
      return (0);
   }
   keypad (scrollp->win, TRUE);
   leaveok (scrollp->win, TRUE);

   if (scrollp->titleLines > 0)
   {
      /* Make the title window. */
      scrollp->titleWin = subwin (scrollp->win,
				scrollp->titleLines, boxWidth - 2,
				ypos + 1, xpos + 1);
   }

   /* Create the scrollbar window. */
   if (splace == RIGHT)
   {
      scrollp->fieldWin = subwin (scrollp->win,
				scrollp->viewSize, scrollp->fieldWidth,
				ypos + scrollp->titleLines + 1,
				xpos + 1);
      scrollp->scrollbarWin = subwin (scrollp->win,
				scrollp->viewSize, 1,
				ypos + scrollp->titleLines + 1,
				xpos + 1 + scrollp->fieldWidth);
   }
   else if (splace == LEFT)
   {
      scrollp->scrollbarWin = subwin (scrollp->win,
				scrollp->viewSize, 1,
				ypos + scrollp->titleLines + 1,
				xpos + 1);
      scrollp->fieldWin = subwin (scrollp->win,
				scrollp->viewSize, scrollp->fieldWidth,
				ypos + scrollp->titleLines + 1,
				xpos + 2);
   }
   else
   {
      scrollp->fieldWin = subwin (scrollp->win,
				scrollp->viewSize, scrollp->fieldWidth,
				ypos + scrollp->titleLines + 1,
				xpos + 1);
      scrollp->scrollbarWin = 0;
   }

   /* Set the rest of the variables */
   ScreenOf(scrollp)		= cdkscreen;
   scrollp->parent		= cdkscreen->window;
   scrollp->boxHeight		= boxHeight;
   scrollp->boxWidth		= boxWidth;
   scrollp->maxLeftChar		= 0;
   scrollp->currentTop		= 0;
   scrollp->currentItem		= 0;
   scrollp->currentHigh		= 0;
   scrollp->leftChar		= 0;
   scrollp->highlight		= highlight;
   scrollp->exitType		= vNEVER_ACTIVATED;
   ObjOf(scrollp)->box		= Box;
   scrollp->shadow		= shadow;
   scrollp->preProcessFunction	= 0;
   scrollp->preProcessData	= 0;
   scrollp->postProcessFunction = 0;
   scrollp->postProcessData	= 0;
   scrollp->ULChar		= ACS_ULCORNER;
   scrollp->URChar		= ACS_URCORNER;
   scrollp->LLChar		= ACS_LLCORNER;
   scrollp->LRChar		= ACS_LRCORNER;
   scrollp->HChar		= ACS_HLINE;
   scrollp->VChar		= ACS_VLINE;
   scrollp->BoxAttrib		= A_NORMAL;

   /* Create the scrolling list item list and needed variables. */
   createCDKScrollItemList (scrollp, numbers, list, listSize);

   /* Clean the key bindings. */
   cleanCDKObjectBindings (vSCROLL, scrollp);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vSCROLL, scrollp);

   /* Return the scrolling list */
   return (scrollp);
}

/*
 * This actually does all the 'real' work of managing the scrolling list.
 */
int activateCDKScroll (CDKSCROLL *scrollp, chtype *actions)
{
   /* Draw the scrolling list */
   drawCDKScroll (scrollp, ObjOf(scrollp)->box);

   /* Check if actions is null. */
   if (actions == 0)
   {
      /* Declare some local variables. */
      chtype input;
      int ret;

      for (;;)
      {
	 /* Get the input. */
	 wrefresh (scrollp->win);
	 input = wgetch (scrollp->win);

	 /* Inject the character into the widget. */
	 ret = injectCDKScroll (scrollp, input);
	 if (scrollp->exitType != vEARLY_EXIT)
	 {
	    return ret;
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
	 ret = injectCDKScroll (scrollp, actions[x]);
	 if (scrollp->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }

   /* Set the exit type for the widget and return. */
   scrollp->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This injects a single character into the widget.
 */
int injectCDKScroll (CDKSCROLL *scrollp, chtype input)
{
   /* Declare local variables. */
   int ppReturn = 1;

   /* Set the exit type for the widget. */
   scrollp->exitType = vEARLY_EXIT;

   /* Draw the scrolling list */
   drawCDKScrollList (scrollp);

   /* Check if there is a pre-process function to be called. */
   if (scrollp->preProcessFunction != 0)
   {
      /* Call the pre-process function. */
      ppReturn = ((PROCESSFN)(scrollp->preProcessFunction)) (vSCROLL, scrollp, scrollp->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check for a predefined key binding. */
      if (checkCDKObjectBind (vSCROLL, scrollp, input) != 0)
      {
	 return -1;
      }
      else
      {
	 switch (input)
	 {
	    case KEY_UP :
		 if (scrollp->currentItem > 0)
		 {
		    scrollp->currentItem--;
		    if (scrollp->currentHigh == 0)
		    {
		       scrollp->currentTop--;
		    }
		    else
		    {
		       scrollp->currentHigh--;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_DOWN :
		 if (scrollp->currentItem < scrollp->listSize - 1)
		 {
		    scrollp->currentItem++;
		    if (scrollp->currentHigh == scrollp->viewSize - 1)
		    {
		       scrollp->currentTop++;
		    }
		    else
		    {
		       scrollp->currentHigh++;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_LEFT :
		 if (scrollp->leftChar > 0)
		 {
		    scrollp->leftChar--;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_RIGHT :
		 if (scrollp->leftChar < scrollp->maxLeftChar)
		 {
		    scrollp->leftChar++;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_PPAGE :
	    case CONTROL('B') :
		 if (scrollp->currentItem > 0)
		 {
		    scrollp->currentTop =
			MAXIMUM (scrollp->currentTop - scrollp->viewSize + 1,
				 0);
		    scrollp->currentItem =
			MAXIMUM (scrollp->currentItem - scrollp->viewSize + 1,
				 0);
		    scrollp->currentHigh =
			scrollp->currentItem - scrollp->currentTop;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_NPAGE :
	    case CONTROL('F') :
		 if (scrollp->currentItem < scrollp->listSize - 1)
		 {
		    scrollp->currentTop =
			MINIMUM (scrollp->currentTop + scrollp->viewSize - 1,
				 scrollp->maxTopItem);
		    scrollp->currentItem =
			MINIMUM (scrollp->currentItem + scrollp->viewSize - 1,
				 scrollp->listSize-1);
		    scrollp->currentHigh =
			scrollp->currentItem - scrollp->currentTop;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case 'g' :
	    case '1' :
	    case KEY_HOME :
		 scrollp->currentTop	= 0;
		 scrollp->currentItem	= 0;
		 scrollp->currentHigh	= 0;
		 break;

	    case 'G' :
	    case KEY_END :
		 scrollp->currentTop	= scrollp->maxTopItem;
		 scrollp->currentItem	= scrollp->listSize-1;
		 scrollp->currentHigh	= scrollp->currentItem - scrollp->currentTop;
		 break;

	    case '|' :
		 scrollp->leftChar	= 0;
		 break;

	    case '$' :
		 scrollp->leftChar	= scrollp->maxLeftChar;
		 break;

	    case KEY_ESC :
		 scrollp->exitType = vESCAPE_HIT;
		 return -1;

	    case KEY_RETURN :
	    case KEY_TAB :
	    case KEY_ENTER :
	    case KEY_CR :
		 scrollp->exitType = vNORMAL;
		 return scrollp->currentItem;

	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(scrollp));
		 refreshCDKScreen (ScreenOf(scrollp));
		 break;

	    default :
		 Beep();
		 break;
	 }
      }

      /* Should we call a post-process? */
      if (scrollp->postProcessFunction != 0)
      {
	 ((PROCESSFN)(scrollp->postProcessFunction)) (vSCROLL, scrollp, scrollp->postProcessData, input);
      }
   }

   /* Redraw the list */
   drawCDKScrollList (scrollp);

   /* Set the exit type and return. */
   scrollp->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This allows the user to accelerate to a position in the scrolling list.
 */
void setCDKScrollPosition (CDKSCROLL *scrollp, int item)
{
   if (item < scrollp->currentTop)
   {
      item = MAXIMUM (item, 0);
      scrollp->currentTop	= item;
   }
   else if (item > scrollp->currentTop + (scrollp->viewSize - 1))
   {
      item = MINIMUM (item, scrollp->listSize - 1);
      scrollp->currentTop	= item - (scrollp->viewSize - 1);
   }
   scrollp->currentItem	= item;
   scrollp->currentHigh	= item - scrollp->currentTop;
}

/*
 * This moves the scroll field to the given location.
 */
static void _moveCDKScroll (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKSCROLL *scrollp = (CDKSCROLL *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(scrollp->win);
      yplace += getbegy(scrollp->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(scrollp), &xplace, &yplace, scrollp->boxWidth, scrollp->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(scrollp->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKScroll (scrollp, ObjOf(scrollp)->box);
   }
}

/*
 * This function draws the scrolling list widget.
 */
static void _drawCDKScroll (CDKOBJS *object, boolean Box)
{
   CDKSCROLL *scrollp = (CDKSCROLL *)object;
   int x;

   /* Box it if needed. */
   if (Box)
   {
      attrbox (scrollp->win,
		scrollp->ULChar, scrollp->URChar,
		scrollp->LLChar, scrollp->LRChar,
		scrollp->HChar,	 scrollp->VChar,
		scrollp->BoxAttrib,
		scrollp->shadow);
   }

   if (scrollp->titleLines > 0)
   {
      /* Draw in the title if there is one. */
      for (x=0; x < scrollp->titleLines; x++)
      {
	 writeChtype (scrollp->titleWin,
			scrollp->titlePos[x], x,
			scrollp->title[x],
			HORIZONTAL, 0,
			scrollp->titleLen[x]);
      }
      wnoutrefresh (scrollp->titleWin);
   }

   /* Draw in the scolling list items. */
   drawCDKScrollList (scrollp);
}

/*
 * This redraws the scrolling list.
 */
static void drawCDKScrollList (CDKSCROLL *scrollp)
{
   /* Declare some local vars */
   int screenPos, x, y;

   werase (scrollp->fieldWin);

   /* Redraw the list. */
   for (x=0, y=scrollp->currentTop; x < scrollp->viewSize; x++, y++)
   {
      if (y >= scrollp->listSize)
      {
	 break;
      }

      screenPos = scrollp->itemPos[y] - scrollp->leftChar;

      /* Write in the correct line. */
      if (y == scrollp->currentItem)
      {
	 if (screenPos >= 0)
	 {
	    writeChtypeAttrib (scrollp->fieldWin,
			screenPos, x,
			scrollp->item[y],
			scrollp->highlight,
			HORIZONTAL, 0,
			scrollp->itemLen[y]);
	 }
	 else
	 {
	    writeChtypeAttrib (scrollp->fieldWin,
			0, x,
			scrollp->item[y],
			scrollp->highlight,
			HORIZONTAL, -screenPos,
			scrollp->itemLen[y]);
	 }
      }
      else
      {
	 if (screenPos >= 0)
	 {
	    writeChtype (scrollp->fieldWin,
			screenPos, x,
			scrollp->item[y],
			HORIZONTAL, 0,
			scrollp->itemLen[y]);
	 }
	 else
	 {
	    writeChtype (scrollp->fieldWin,
			0, x,
			scrollp->item[y],
			HORIZONTAL, -screenPos,
			scrollp->itemLen[y]);
	 }
      }
   }

   if (scrollp->scrollbar)
   {
      int togglePos;

      /* Determine where the toggle is supposed to be. */
      if (scrollp->listSize >= scrollp->viewSize)
	 togglePos = floorCDK ((float)scrollp->viewSize * (float)scrollp->currentItem / (float)scrollp->listSize);
      else
	 togglePos = floorCDK ((float)(scrollp->viewSize - scrollp->toggleSize) * (float)scrollp->currentItem / (float)(scrollp->listSize - 1));

      /* Make sure the toggle button doesn't go out of bounds. */
      togglePos = MINIMUM (togglePos, scrollp->viewSize - scrollp->toggleSize);

      /* Draw the scrollbar. */
      mvwvline (scrollp->scrollbarWin, 0, 0, ACS_CKBOARD, scrollp->viewSize);
      mvwvline (scrollp->scrollbarWin, togglePos, 0, ' ' | A_REVERSE, scrollp->toggleSize);
      wnoutrefresh (scrollp->scrollbarWin);
   }

   /* Refresh the window. */
   wnoutrefresh (scrollp->fieldWin);
   wnoutrefresh (scrollp->win);
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKScrollULChar (CDKSCROLL *scrollp, chtype character)
{
   scrollp->ULChar = character;
}
void setCDKScrollURChar (CDKSCROLL *scrollp, chtype character)
{
   scrollp->URChar = character;
}
void setCDKScrollLLChar (CDKSCROLL *scrollp, chtype character)
{
   scrollp->LLChar = character;
}
void setCDKScrollLRChar (CDKSCROLL *scrollp, chtype character)
{
   scrollp->LRChar = character;
}
void setCDKScrollVerticalChar (CDKSCROLL *scrollp, chtype character)
{
   scrollp->VChar = character;
}
void setCDKScrollHorizontalChar (CDKSCROLL *scrollp, chtype character)
{
   scrollp->HChar = character;
}
void setCDKScrollBoxAttribute (CDKSCROLL *scrollp, chtype character)
{
   scrollp->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKScrollBackgroundColor (CDKSCROLL *scrollp, char *color)
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
   wbkgd (scrollp->win, holder[0]);
   if (scrollp->scrollbar)
   {
      wbkgd (scrollp->scrollbarWin, holder[0]);
   }

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This function destroys
 */
void destroyCDKScroll (CDKSCROLL *scrollp)
{
   /* Declare local variables. */
   int x;

   /* Erase the object. */
   eraseCDKScroll (scrollp);

   /* Clean up the char pointers. */
   for (x=0; x < scrollp->titleLines; x++)
   {
      freeChtype (scrollp->title[x]);
   }
   for (x=0; x < scrollp->listSize; x++)
   {
      freeChtype (scrollp->item[x]);
   }

   /* Clean up the windows. */
   deleteCursesWindow (scrollp->win);

   /* Unregister this object. */
   unregisterCDKObject (vSCROLL, scrollp);

   /* Finish cleaning up. */
   free (scrollp);
}

/*
 * This function erases the scrolling list from the screen.
 */
static void _eraseCDKScroll (CDKOBJS *object)
{
   CDKSCROLL *scrollp = (CDKSCROLL *)object;

   eraseCursesWindow (scrollp->win);
}

/*
 * This function creates massages the scrolling list information and
 * sets up the needed variables for the scrolling list to work correctly.
 */
static void createCDKScrollItemList (CDKSCROLL *scrollp, boolean numbers, char **list, int listSize)
{
   /* Declare local variables. */
   int widestItem, x;
   char temp[100];

   /* Is the view size larger than the list? */
   scrollp->listSize	= listSize;
   scrollp->maxTopItem	= MAXIMUM (0, listSize - scrollp->viewSize);

   /* Set the information for the scroll bar. */
   if (listSize >= scrollp->viewSize)
      scrollp->toggleSize = 1;
   else
      scrollp->toggleSize = floorCDK ((float)scrollp->viewSize / (float)listSize + 0.5);

   /* Create the items in the scrolling list. */
   widestItem		= 0;
   if (numbers == NUMBERS)
   {
      for (x=0 ; x < listSize; x++)
      {
	 sprintf (temp, "%4d. %s", x + 1, list[x]);
	 scrollp->item[x]	= char2Chtype (temp, &scrollp->itemLen[x], &scrollp->itemPos[x]);
	 scrollp->itemPos[x]	= justifyString (scrollp->fieldWidth, scrollp->itemLen[x], scrollp->itemPos[x]);
	 widestItem		= MAXIMUM (widestItem, scrollp->itemLen[x]);
      }
   }
   else
   {
      for (x=0 ; x < listSize; x++)
      {
	 scrollp->item[x]	= char2Chtype (list[x], &scrollp->itemLen[x], &scrollp->itemPos[x]);
	 scrollp->itemPos[x]	= justifyString (scrollp->fieldWidth, scrollp->itemLen[x], scrollp->itemPos[x]);
	 widestItem		= MAXIMUM (widestItem, scrollp->itemLen[x]);
      }
   }

   /* Determine how many characters we can shift to the right */
   /* before all the items have been scrolled off the screen. */
   scrollp->maxLeftChar = MAXIMUM (0, widestItem - scrollp->fieldWidth);

   /* Keep the boolean flag 'numbers' */
   scrollp->numbers = numbers;
}

/*
 * This sets certain attributes of the scrolling list.
 */
void setCDKScroll (CDKSCROLL *scrollp, char **list, int listSize, boolean numbers, chtype highlight, boolean Box)
{
   setCDKScrollItems (scrollp, list, listSize, numbers);
   setCDKScrollHighlight (scrollp, highlight);
   setCDKScrollBox (scrollp, Box);
}

/*
 * This sets the scrolling list items.
 */
void setCDKScrollItems (CDKSCROLL *scrollp, char **list, int listSize, boolean numbers)
{
   /* Declare some wars. */
   int x;

   /* Clean out the old list. */
   for (x=0; x < scrollp->listSize; x++)
   {
      freeChtype (scrollp->item[x]);
      scrollp->itemLen[x] = 0;
      scrollp->itemPos[x] = 0;
   }

   /* Set some vars. */
   scrollp->currentTop	= 0;
   scrollp->currentItem = 0;
   scrollp->currentHigh = 0;
   scrollp->leftChar	= 0;

   /* Set up the new list. */
   createCDKScrollItemList (scrollp, numbers, list, listSize);
}
int getCDKScrollItems (CDKSCROLL *scrollp, char *list[])
{
   int x;

   for (x=0; x < scrollp->listSize; x++)
   {
      list[x] = chtype2Char (scrollp->item[x]);
   }
   return scrollp->listSize;
}

/*
 * This sets the highlight of the scrolling list.
 */
void setCDKScrollHighlight (CDKSCROLL *scrollp, chtype highlight)
{
   scrollp->highlight = highlight;
}
chtype getCDKScrollHighlight (CDKSCROLL *scrollp, chtype highlight GCC_UNUSED)
{
   return scrollp->highlight;
}

/*
 * This sets the box attribute of the scrolling list.
 */
void setCDKScrollBox (CDKSCROLL *scrollp, boolean Box)
{
   ObjOf(scrollp)->box = Box;
}
boolean getCDKScrollBox (CDKSCROLL *scrollp)
{
   return ObjOf(scrollp)->box;
}

/*
 * This adds a single item to a scrolling list.
 */
void addCDKScrollItem (CDKSCROLL *scrollp, char *item)
{
   /* Declare some local variables. */
   int itemNumber = scrollp->listSize;
   int widestItem = scrollp->maxLeftChar + scrollp->fieldWidth;
   char temp[256];

   /*
    * If the scrolling list has numbers, then add the new item
    * with a numeric value attached.
     */
   if (scrollp->numbers == NUMBERS)
   {
      sprintf (temp, "%4d. %s", itemNumber + 1, item);
      scrollp->item[itemNumber] = char2Chtype (temp, &scrollp->itemLen[itemNumber], &scrollp->itemPos[itemNumber]);
      scrollp->itemPos[itemNumber] = justifyString (scrollp->fieldWidth, scrollp->itemLen[itemNumber], scrollp->itemPos[itemNumber]);
   }
   else
   {
      scrollp->item[itemNumber] = char2Chtype (item, &scrollp->itemLen[itemNumber], &scrollp->itemPos[itemNumber]);
      scrollp->itemPos[itemNumber] = justifyString (scrollp->fieldWidth, scrollp->itemLen[itemNumber], scrollp->itemPos[itemNumber]);
   }

   /* Determine the size of the widest item. */
   widestItem = MAXIMUM (scrollp->itemLen[itemNumber], widestItem);
   scrollp->maxLeftChar = MAXIMUM (0, widestItem - scrollp->fieldWidth);

   /* Increment the list size. */
   scrollp->listSize++;
   scrollp->maxTopItem = MAXIMUM (0, scrollp->listSize - scrollp->viewSize);

   /* Reset some variables. */
   scrollp->currentTop	= 0;
   scrollp->currentItem = 0;
   scrollp->currentHigh = 0;
   scrollp->leftChar	= 0;
}

/*
 * This adds a single item to a scrolling list.
 */
void deleteCDKScrollItem (CDKSCROLL *scrollp, int position)
{
   /* Declare some local variables. */
   int x;

   /* Nuke the current item. */
   freeChtype (scrollp->item[position]);

   /* Adjust the list. */
   for (x=position; x < scrollp->listSize-1; x++)
   {
      scrollp->item[x]		= scrollp->item[x + 1];
      scrollp->itemLen[x]	= scrollp->itemLen[x + 1];
      scrollp->itemPos[x]	= scrollp->itemPos[x + 1];
   }

   /* Decrement the list size. */
   scrollp->listSize--;
   scrollp->maxTopItem = MAXIMUM (0, scrollp->listSize - scrollp->viewSize);

   /* Reset some variables. */
   scrollp->currentTop	= 0;
   scrollp->currentItem	= 0;
   scrollp->currentHigh	= 0;
   scrollp->leftChar	= 0;
}

/*
 * This function sets the pre-process function.
 */
void setCDKScrollPreProcess (CDKSCROLL *scrollp, PROCESSFN callback, void *data)
{
   scrollp->preProcessFunction = callback;
   scrollp->preProcessData = data;
}

/*
 * This function sets the post-process function.
 */
void setCDKScrollPostProcess (CDKSCROLL *scrollp, PROCESSFN callback, void *data)
{
   scrollp->postProcessFunction = callback;
   scrollp->postProcessData = data;
}
