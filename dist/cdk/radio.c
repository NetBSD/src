#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/09 18:41:53 $
 * $Revision: 1.3 $
 */

/*
 * Declare file local prototypes.
 */
static void drawCDKRadioList (CDKRADIO *radio);
static void createCDKRadioItemList (CDKRADIO *radio,
				char **list, int listSize);

DeclareCDKObjects(my_funcs,Radio);

/*
 * This function creates the radio widget.
 */
CDKRADIO *newCDKRadio (CDKSCREEN *cdkscreen, int xplace, int yplace, int splace, int height, int width, char *title, char **list, int listSize, chtype choiceChar, int defItem, chtype highlight, boolean Box, boolean shadow)
{
   /* Declare local variables. */
   CDKRADIO *radio	= newCDKObject(CDKRADIO, &my_funcs);
   chtype *holder	= 0;
   int parentWidth	= getmaxx(cdkscreen->window);
   int parentHeight	= getmaxy(cdkscreen->window);
   int boxWidth		= width;
   int boxHeight	= height;
   int maxWidth		= INT_MIN;
   int xpos		= xplace;
   int ypos		= yplace;
   char **temp		= 0;
   int x, len, junk2;

   if (splace == LEFT || splace == RIGHT)
   {
      radio->scrollbar = TRUE;
   }
   else
   {
      radio->scrollbar = FALSE;
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
   boxWidth = setWidgetDimension (parentWidth, width, 5);

   /* Translate the char * title to a chtype * */
   if (title != 0)
   {
      temp = CDKsplitString (title, '\n');
      radio->titleLines = CDKcountStrings (temp);

      /* We need to determine the widest title line. */
      for (x=0; x < radio->titleLines; x++)
      {
	 holder = char2Chtype (temp[x], &len, &junk2);
	 maxWidth = MAXIMUM (maxWidth, len);
	 freeChtype (holder);
      }
      boxWidth = MAXIMUM (boxWidth, maxWidth + 2);

      /* For each line in the title, convert from a char * to a chtype * */
      for (x=0; x < radio->titleLines; x++)
      {
	 radio->title[x]	= char2Chtype (temp[x],
					&radio->titleLen[x],
					&radio->titlePos[x]);
	 radio->titlePos[x]	= justifyString (boxWidth - 2,
					radio->titleLen[x],
					radio->titlePos[x]);
      }

      CDKfreeStrings(temp);
   }
   else
   {
      radio->titleLines = 0;
   }

   /* Set the box height. */
   if (radio->titleLines > boxHeight)
   {
      if (listSize > 8)
      {
	 boxHeight = radio->titleLines + 10;
      }
      else
      {
	 boxHeight = radio->titleLines + listSize + 2;
      }
   }
   radio->fieldWidth	= boxWidth - 2 - radio->scrollbar;
   radio->viewSize	= boxHeight - 2 - radio->titleLines;

   /*
    * Make sure we didn't extend beyond the dimensions of the window.
    */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the radio window */
   radio->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   /* Is the window null??? */
   if (radio->win == 0)
   {
      /* Clean up memory. */
      for (x=0; x < radio->titleLines; x++)
      {
	 freeChtype (radio->title[x]);
      }
      free (radio);

      /* Return a null pointer. */
      return (0);
   }
   keypad (radio->win, TRUE);
   leaveok (radio->win, TRUE);

   if (radio->titleLines > 0)
   {
      /* Make the title window. */
      radio->titleWin = subwin (radio->win,
				radio->titleLines, boxWidth - 2,
				ypos + 1, xpos + 1);
   }

   /* Create the scrollbar window. */
   if (splace == RIGHT)
   {
      radio->fieldWin = subwin (radio->win,
				radio->viewSize, radio->fieldWidth,
				ypos + radio->titleLines + 1,
				xpos + 1);
      radio->scrollbarWin = subwin (radio->win,
				radio->viewSize, 1,
				ypos + radio->titleLines + 1,
				xpos + 1 + radio->fieldWidth);
   }
   else if (splace == LEFT)
   {
      radio->scrollbarWin = subwin (radio->win,
				radio->viewSize, 1,
				ypos + radio->titleLines + 1,
				xpos + 1);
      radio->fieldWin = subwin (radio->win,
				radio->viewSize, radio->fieldWidth,
				ypos + radio->titleLines + 1,
				xpos + 2);
   }
   else
   {
      radio->fieldWin = subwin (radio->win,
				radio->viewSize, radio->fieldWidth,
				ypos + radio->titleLines + 1,
				xpos + 1);
      radio->scrollbarWin = 0;
   }

   /* Set the rest of the variables */
   ScreenOf(radio)		= cdkscreen;
   radio->parent		= cdkscreen->window;
   radio->boxHeight		= boxHeight;
   radio->boxWidth		= boxWidth;
   radio->currentTop		= 0;
   radio->currentItem		= 0;
   radio->currentHigh		= 0;
   radio->leftChar		= 0;
   radio->selectedItem		= defItem;
   radio->highlight		= highlight;
   radio->choiceChar		= choiceChar;
   radio->leftBoxChar		= (chtype)'[';
   radio->rightBoxChar		= (chtype)']';
   radio->exitType		= vNEVER_ACTIVATED;
   ObjOf(radio)->box		= Box;
   radio->shadow		= shadow;
   radio->preProcessFunction	= 0;
   radio->preProcessData	= 0;
   radio->postProcessFunction	= 0;
   radio->postProcessData	= 0;
   radio->ULChar		= ACS_ULCORNER;
   radio->URChar		= ACS_URCORNER;
   radio->LLChar		= ACS_LLCORNER;
   radio->LRChar		= ACS_LRCORNER;
   radio->HChar			= ACS_HLINE;
   radio->VChar			= ACS_VLINE;
   radio->BoxAttrib		= A_NORMAL;

   /* Create the scrolling list item list and needed variables. */
   createCDKRadioItemList (radio, list, listSize);

   /* Clean the key bindings. */
   cleanCDKObjectBindings (vRADIO, radio);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vRADIO, radio);

   /* Return the radio list */
   return (radio);
}

/*
 * This actually manages the radio widget.
 */
int activateCDKRadio (CDKRADIO *radio, chtype *actions)
{
   /* Draw the radio list. */
   drawCDKRadio (radio, ObjOf(radio)->box);

   /* Check if actions is null. */
   if (actions == 0)
   {
      /* Declare some local variables. */
      chtype input;
      int ret;

      for (;;)
      {
	 /* Get the input. */
	 wrefresh (radio->win);
	 input = wgetch (radio->win);

	 /* Inject the character into the widget. */
	 ret = injectCDKRadio (radio, input);
	 if (radio->exitType != vEARLY_EXIT)
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
	 ret = injectCDKRadio (radio, actions[x]);
	 if (radio->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }

   /* Set the exit type and return. */
   radio->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This injects a single character into the widget.
 */
int injectCDKRadio (CDKRADIO *radio, chtype input)
{
   /* Declare local variables. */
   int ppReturn = 1;

   /* Set the exit type. */
   radio->exitType = vEARLY_EXIT;

   /* Draw the radio list */
   drawCDKRadioList (radio);

   /* Check if there is a pre-process function to be called. */
   if (radio->preProcessFunction != 0)
   {
      /* Call the pre-process function. */
      ppReturn = ((PROCESSFN)(radio->preProcessFunction)) (vRADIO, radio, radio->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check for a predefined key binding. */
      if (checkCDKObjectBind (vRADIO, radio, input) != 0)
      {
	 return -1;
      }
      else
      {
	 switch (input)
	 {
	    case KEY_UP :
		 if (radio->currentItem > 0)
		 {
		    radio->currentItem--;
		    if (radio->currentHigh == 0)
		    {
		       radio->currentTop--;
		    }
		    else
		    {
		       radio->currentHigh--;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_DOWN :
		 if (radio->currentItem < radio->listSize - 1)
		 {
		    radio->currentItem++;
		    if (radio->currentHigh == radio->viewSize - 1)
		    {
		       radio->currentTop++;
		    }
		    else
		    {
		       radio->currentHigh++;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_LEFT :
		 if (radio->leftChar > 0)
		 {
		    radio->leftChar--;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_RIGHT :
		 if (radio->leftChar < radio->maxLeftChar)
		 {
		    radio->leftChar++;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_PPAGE :
	    case CONTROL('B') :
		 if (radio->currentItem > 0)
		 {
		    radio->currentTop =
			MAXIMUM (radio->currentTop - radio->viewSize + 1,
				 0);
		    radio->currentItem =
			MAXIMUM (radio->currentItem - radio->viewSize + 1,
				 0);
		    radio->currentHigh =
			radio->currentItem - radio->currentTop;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case KEY_NPAGE :
	    case CONTROL('F') :
		 if (radio->currentItem < radio->listSize - 1)
		 {
		    radio->currentTop =
			MINIMUM (radio->currentTop + radio->viewSize - 1,
				 radio->maxTopItem);
		    radio->currentItem =
			MINIMUM (radio->currentItem + radio->viewSize - 1,
				 radio->listSize-1);
		    radio->currentHigh =
			radio->currentItem - radio->currentTop;
		 }
		 else
		 {
		    Beep();
		 }
		 break;

	    case 'g' :
	    case '1' :
	    case KEY_HOME :
		 radio->currentTop	= 0;
		 radio->currentItem	= 0;
		 radio->currentHigh	= 0;
		 break;

	    case 'G' :
	    case KEY_END :
		 radio->currentTop	= radio->maxTopItem;
		 radio->currentItem	= radio->listSize-1;
		 radio->currentHigh	= radio->currentItem - radio->currentTop;
		 break;

	    case '|' :
		 radio->leftChar	= 0;
		 break;

	    case '$' :
		 radio->leftChar	= radio->maxLeftChar;
		 break;

	    case SPACE :
		 radio->selectedItem	= radio->currentItem;
		 break;

	    case KEY_ESC :
		 radio->exitType = vESCAPE_HIT;
		 return -1;

	    case KEY_RETURN :
	    case KEY_TAB :
	    case KEY_ENTER :
	    case KEY_CR :
		 radio->exitType = vNORMAL;
		 return radio->selectedItem;

	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(radio));
		 refreshCDKScreen (ScreenOf(radio));
		 break;

	    default :
		 Beep();
		 break;
	 }
      }

      /* Should we call a post-process? */
      if (radio->postProcessFunction != 0)
      {
	 ((PROCESSFN)(radio->postProcessFunction)) (vRADIO, radio, radio->postProcessData, input);
      }
   }

   /* Draw the radio list */
   drawCDKRadioList (radio);

   /* Set the exit type and return. */
   radio->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This moves the radio field to the given location.
 */
static void _moveCDKRadio (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKRADIO *radio = (CDKRADIO *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(radio->win);
      yplace += getbegy(radio->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(radio), &xplace, &yplace, radio->boxWidth, radio->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(radio->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKRadio (radio, ObjOf(radio)->box);
   }
}

/*
 * This function draws the radio widget.
 */
static void _drawCDKRadio (CDKOBJS *object, boolean Box GCC_UNUSED)
{
   CDKRADIO *radio = (CDKRADIO *)object;
   int x;

   /* Box it if needed. */
   if (Box)
   {
      attrbox (radio->win,
		radio->ULChar, radio->URChar,
		radio->LLChar, radio->LRChar,
		radio->HChar,  radio->VChar,
		radio->BoxAttrib,
		radio->shadow);
   }

   if (radio->titleLines > 0)
   {
      /* Draw in the title if there is one */
      for (x=0; x < radio->titleLines; x++)
      {
	 writeChtype (radio->titleWin,
			radio->titlePos[x], x,
			radio->title[x],
			HORIZONTAL, 0,
			radio->titleLen[x]);
      }
      wnoutrefresh (radio->titleWin);
   }

   /* Draw in the radio list. */
   drawCDKRadioList (radio);
}

/*
 * This redraws the radio list.
 */
static void drawCDKRadioList (CDKRADIO *radio)
{
   /* Declare local variables. */
   int screenPos, x, y;
   chtype temp[3];

   werase (radio->fieldWin);

   /* Redraw the list */
   for (x=0, y=radio->currentTop; x < radio->viewSize; x++, y++)
   {
      if (y >= radio->listSize)
      {
	 break;
      }

      screenPos = radio->itemPos[y] - radio->leftChar;

      /* Draw in the line. */
      if (y == radio->currentItem)
      {
	 if (screenPos >= 0)
	 {
	    writeChtypeAttrib (radio->fieldWin,
			screenPos, x,
			radio->item[y],
			radio->highlight,
			HORIZONTAL, 0,
			radio->itemLen[y]);
	 }
	 else
	 {
	    writeChtypeAttrib (radio->fieldWin,
			0, x,
			radio->item[y],
			radio->highlight,
			HORIZONTAL, -screenPos,
			radio->itemLen[y]);
	 }
      }
      else
      {
	 if (screenPos >= 0)
	 {
	    writeChtype (radio->fieldWin,
			screenPos, x,
			radio->item[y],
			HORIZONTAL, 0,
			radio->itemLen[y]);
	 }
	 else
	 {
	    writeChtype (radio->fieldWin,
			0, x,
			radio->item[y],
			HORIZONTAL, -screenPos,
			radio->itemLen[y]);
	 }
      }

      screenPos = - radio->leftChar;

      temp[0] = radio->leftBoxChar;
      if (y == radio->selectedItem)
      {
         temp[1] = radio->choiceChar;
      }
      else
      {
         temp[1] = ' ';
      }
      temp[2] = radio->rightBoxChar;

      writeChtype (radio->fieldWin,
		0, x,
		temp,
		HORIZONTAL, -screenPos,
		3);
   }

   if (radio->scrollbar)
   {
      int togglePos;

      /* Determine where the toggle is supposed to be. */
      if (radio->listSize >= radio->viewSize)
	 togglePos = floorCDK ((float)radio->viewSize * (float)radio->currentItem / (float)radio->listSize);
      else
	 togglePos = floorCDK ((float)(radio->viewSize - radio->toggleSize) * (float)radio->currentItem / (float)(radio->listSize - 1));

      /* Make sure the toggle button doesn't go out of bounds. */
      togglePos = MINIMUM (togglePos, radio->viewSize - radio->toggleSize);

      /* Draw the scrollbar. */
      mvwvline (radio->scrollbarWin, 0, 0, ACS_CKBOARD, radio->viewSize);
      mvwvline (radio->scrollbarWin, togglePos, 0, ' ' | A_REVERSE, radio->toggleSize);
      wnoutrefresh (radio->scrollbarWin);
   }

   /* Refresh the window. */
   wnoutrefresh (radio->fieldWin);
   wnoutrefresh (radio->win);
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKRadioULChar (CDKRADIO *radio, chtype character)
{
   radio->ULChar = character;
}
void setCDKRadioURChar (CDKRADIO *radio, chtype character)
{
   radio->URChar = character;
}
void setCDKRadioLLChar (CDKRADIO *radio, chtype character)
{
   radio->LLChar = character;
}
void setCDKRadioLRChar (CDKRADIO *radio, chtype character)
{
   radio->LRChar = character;
}
void setCDKRadioVerticalChar (CDKRADIO *radio, chtype character)
{
   radio->VChar = character;
}
void setCDKRadioHorizontalChar (CDKRADIO *radio, chtype character)
{
   radio->HChar = character;
}
void setCDKRadioBoxAttribute (CDKRADIO *radio, chtype character)
{
   radio->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKRadioBackgroundColor (CDKRADIO *radio, char *color)
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
   wbkgd (radio->win, holder[0]);
   if (radio->scrollbar)
   {
      wbkgd (radio->scrollbarWin, holder[0]);
   }

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This function destroys the radio widget.
 */
void destroyCDKRadio (CDKRADIO *radio)
{
   /* Declare local variables. */
   int x;

   /* Erase the object. */
   eraseCDKRadio (radio);

   /* Clear up the char pointers. */
   for (x=0; x < radio->titleLines; x++)
   {
      freeChtype (radio->title[x]);
   }
   for (x=0; x < radio->listSize; x++)
   {
      freeChtype (radio->item[x]);
   }

   /* Clean up the windows. */
   deleteCursesWindow (radio->win);

   /* Unregister this object. */
   unregisterCDKObject (vRADIO, radio);

   /* Finish cleaning up. */
   free (radio);
}

/*
 * This function erases the radio widget.
 */
static void _eraseCDKRadio (CDKOBJS *object)
{
   CDKRADIO *radio = (CDKRADIO *)object;

   eraseCursesWindow (radio->win);
}

/*
 * This function creates massages the scrolling list information and
 * sets up the needed variables for the scrolling list to work correctly.
 */
static void createCDKRadioItemList (CDKRADIO *radio, char **list, int listSize)
{
   /* Declare local variables. */
   int widestItem, x;

   /* Is the view size larger than the list? */
   radio->listSize	= listSize;
   radio->maxTopItem	= MAXIMUM (0, listSize - radio->viewSize);

   /* Determine the size of the scrollbar toggle and the step. */
   if (listSize >= radio->viewSize)
      radio->toggleSize = 1;
   else
      radio->toggleSize = floorCDK ((float)radio->viewSize / (float)listSize + 0.5);

   /* Each item in the needs to be converted to chtype * */
   widestItem		= 0;
   for (x=0; x < listSize; x++)
   {
      radio->item[x]	= char2Chtype (list[x], &radio->itemLen[x], &radio->itemPos[x]);
      radio->itemPos[x] = justifyString (radio->fieldWidth, radio->itemLen[x], radio->itemPos[x]) + 3;
      widestItem	= MAXIMUM (widestItem, radio->itemLen[x]);
   }

   /*
    * Determine how many characters we can shift to the right
    * before all the items have been scrolled off the screen.
    */
   radio->maxLeftChar = MAXIMUM (0, widestItem - radio->fieldWidth + 3);
}

/*
 * This set various attributes of the radio list.
 */
void setCDKRadio (CDKRADIO *radio, chtype highlight, chtype choiceChar, int Box)
{
   setCDKRadioChoiceCharacter (radio, choiceChar);
   setCDKRadioHighlight (radio, highlight);
   setCDKRadioBox (radio, Box);
}

/*
 * This sets the radio list items.
 */
void setCDKRadioItems (CDKRADIO *radio, char **list, int listSize)
{
   /* Declare some wars. */
   int x;

   /* Clean out the old list. */
   for (x=0; x < radio->listSize; x++)
   {
      freeChtype (radio->item[x]);
      radio->itemLen[x] = 0;
      radio->itemPos[x] = 0;
   }

   /* Set some vars. */
   radio->currentTop	= 0;
   radio->currentItem	= 0;
   radio->currentHigh	= 0;
   radio->leftChar	= 0;
   radio->selectedItem	= 0;

   /* Set up the new list. */
   createCDKRadioItemList (radio, list, listSize);
}
int getCDKRadioItems (CDKRADIO *radio, char *list[])
{
   int x;

   for (x=0; x < radio->listSize; x++)
   {
      list[x] = chtype2Char (radio->item[x]);
   }
   return radio->listSize;
}

/*
 * This sets the highlight bar of the radio list.
 */
void setCDKRadioHighlight (CDKRADIO *radio, chtype highlight)
{
   radio->highlight = highlight;
}
chtype getCDKRadioHighlight (CDKRADIO *radio)
{
   return radio->highlight;
}

/*
 * This sets the character to use when selecting an item in the list.
 */
void setCDKRadioChoiceCharacter (CDKRADIO *radio, chtype character)
{
   radio->choiceChar = character;
}
chtype getCDKRadioChoiceCharacter (CDKRADIO *radio)
{
   return radio->choiceChar;
}

/*
 * This sets the character to use to draw the left side of the
 * choice box on the list.
 */
void setCDKRadioLeftBrace (CDKRADIO *radio, chtype character)
{
   radio->leftBoxChar = character;
}
chtype getCDKRadioLeftBrace (CDKRADIO *radio)
{
   return radio->leftBoxChar;
}

/*
 * This sets the character to use to draw the right side of the
 * choice box on the list.
 */
void setCDKRadioRightBrace (CDKRADIO *radio, chtype character)
{
   radio->rightBoxChar = character;
}
chtype getCDKRadioRightBrace (CDKRADIO *radio)
{
   return radio->rightBoxChar;
}

/*
 * This sets the box attribute of the widget.
 */
void setCDKRadioBox (CDKRADIO *radio, boolean Box)
{
   ObjOf(radio)->box = Box;
}
boolean getCDKRadioBox (CDKRADIO *radio)
{
   return ObjOf(radio)->box;
}

/*
 * This function sets the pre-process function.
 */
void setCDKRadioPreProcess (CDKRADIO *radio, PROCESSFN callback, void *data)
{
   radio->preProcessFunction = callback;
   radio->preProcessData = data;
}

/*
 * This function sets the post-process function.
 */
void setCDKRadioPostProcess (CDKRADIO *radio, PROCESSFN callback, void *data)
{
   radio->postProcessFunction = callback;
   radio->postProcessData = data;
}
