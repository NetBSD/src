#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/09 18:41:54 $
 * $Revision: 1.3 $
 */

/*
 * Declare file local prototypes.
 */
static void drawCDKSelectionList (CDKSELECTION *selection);
static void createCDKSelectionItemList (CDKSELECTION *selection,
				char **list, int listSize);

DeclareCDKObjects(my_funcs,Selection);

/*
 * This function creates a selection widget.
 */
CDKSELECTION *newCDKSelection (CDKSCREEN *cdkscreen, int xplace, int yplace, int splace, int height, int width, char *title, char **list, int listSize, char **choices, int choiceCount, chtype highlight, boolean Box, boolean shadow)
{
   /* Declare local variables. */
   CDKSELECTION *selection	= newCDKObject(CDKSELECTION, &my_funcs);
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
      selection->scrollbar = TRUE;
   }
   else
   {
      selection->scrollbar = FALSE;
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

   /* Translate the char * title to a chtype * */
   if (title != 0)
   {
      temp = CDKsplitString (title, '\n');
      selection->titleLines = CDKcountStrings (temp);

      /* We need to determine the widest title line. */
      for (x=0; x < selection->titleLines; x++)
      {
	 holder = char2Chtype (temp[x], &len, &junk2);
	 maxWidth = MAXIMUM (maxWidth, len);
	 freeChtype (holder);
      }
      boxWidth = MAXIMUM (boxWidth, maxWidth + 2);

      /* For each line in the title, convert from char * to chtype * */
      for (x=0; x < selection->titleLines; x++)
      {
	 selection->title[x]	= char2Chtype (temp[x], 
					&selection->titleLen[x], 
					&selection->titlePos[x]);
	 selection->titlePos[x]	= justifyString (boxWidth - 2,
					selection->titleLen[x], 
					selection->titlePos[x]);
      }

      CDKfreeStrings(temp);
   }
   else
   {
      /* No title? Set the required variables. */
      selection->titleLines = 0;
   }

   /* Set the box height. */
   if (selection->titleLines > boxHeight)
   {
      if (listSize > 8)
      {
	 boxHeight = selection->titleLines + 10;
      }
      else
      {
	 boxHeight = selection->titleLines + listSize + 2;
      }
   }
   selection->fieldWidth	= boxWidth - 2 - selection->scrollbar;
   selection->viewSize		= boxHeight - 2 - selection->titleLines;
   
   /*
    * Make sure we didn't extend beyond the dimensions of the window.
    */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);
   
   /* Make the selection window */
   selection->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);
   
   /* Is the window null?? */
   if (selection->win == 0)
   {
      /* Clean up any memory used. */
      for (x=0; x < selection->titleLines; x++)
      {
	 freeChtype (selection->title[x]);
      }
      free (selection);
   
      /* Return a null pointer. */
      return (0);
   }
   keypad (selection->win, TRUE);
   leaveok (selection->win, TRUE);

   if (selection->titleLines > 0)
   {
      /* Make the title window. */
      selection->titleWin = subwin (selection->win,
				selection->titleLines, boxWidth - 2,
				ypos + 1, xpos + 1);
   }

   /* Create the scrollbar window. */
   if (splace == RIGHT)
   {
      selection->fieldWin = subwin (selection->win,
				selection->viewSize, selection->fieldWidth,
				ypos + selection->titleLines + 1,
				xpos + 1);
      selection->scrollbarWin = subwin (selection->win,
				selection->viewSize, 1,
				ypos + selection->titleLines + 1,
				xpos + 1 + selection->fieldWidth);
   }
   else if (splace == LEFT)
   {
      selection->scrollbarWin = subwin (selection->win,
				selection->viewSize, 1,
				ypos + selection->titleLines + 1,
				xpos + 1);
      selection->fieldWin = subwin (selection->win,
				selection->viewSize, selection->fieldWidth,
				ypos + selection->titleLines + 1,
				xpos + 2);
   }
   else
   {
      selection->fieldWin = subwin (selection->win,
				selection->viewSize, selection->fieldWidth,
				ypos + selection->titleLines + 1,
				xpos + 1);
      selection->scrollbarWin = 0;
   }

   /* Set the rest of the variables */
   ScreenOf(selection)			= cdkscreen;
   selection->parent			= cdkscreen->window;
   selection->boxHeight			= boxHeight;
   selection->boxWidth			= boxWidth;
   selection->maxLeftChar		= 0;
   selection->currentTop		= 0;
   selection->currentHigh		= 0;
   selection->currentItem		= 0;
   selection->leftChar			= 0;
   selection->highlight			= highlight;
   selection->choiceCount		= choiceCount;
   selection->exitType			= vNEVER_ACTIVATED;
   ObjOf(selection)->box		= Box;
   selection->shadow			= shadow;
   selection->preProcessFunction	= 0;
   selection->preProcessData		= 0;
   selection->postProcessFunction	= 0;
   selection->postProcessData		= 0;
   selection->ULChar			= ACS_ULCORNER;
   selection->URChar			= ACS_URCORNER;
   selection->LLChar			= ACS_LLCORNER;
   selection->LRChar			= ACS_LRCORNER;
   selection->HChar			= ACS_HLINE;
   selection->VChar			= ACS_VLINE;
   selection->BoxAttrib			= A_NORMAL;
   
   /* Each choice has to be converted from char * to chtype * */
   selection->maxchoicelen	= 0;
   for (x=0; x < choiceCount; x++)
   {
      selection->choice[x]	= char2Chtype (choices[x], &selection->choicelen[x], &junk2);
      selection->maxchoicelen	= MAXIMUM (selection->maxchoicelen, selection->choicelen[x]);
   }
   
   createCDKSelectionItemList (selection, list, listSize);

   /* Clean the key bindings. */
   cleanCDKObjectBindings (vSELECTION, selection);
   
   /* Register this baby. */
   registerCDKObject (cdkscreen, vSELECTION, selection);
   
   /* Return the selection list */
   return (selection);
}
   
/*
 * This actually manages the selection widget...
 */
int activateCDKSelection (CDKSELECTION *selection, chtype *actions)
{
   /* Draw the selection list */
   drawCDKSelection (selection, ObjOf(selection)->box);
   
   /* Check if actions is null. */
   if (actions == 0)
   {
      /* Declare some local variables. */
      chtype input;
      int ret;
   
      for (;;)
      {
	 /* Get the input. */
	 wrefresh (selection->win);
	 input = wgetch (selection->win);
   
	 /* Inject the character into the widget. */
	 ret = injectCDKSelection (selection, input);
	 if (selection->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }
   else
   {
      /* Declare some local variables. */
      int length = chlen (actions);
      int x = 0;
      int ret;
   
      /* Inject each character one at a time. */
      for (x=0; x < length; x++)
      {
	 ret = injectCDKSelection (selection, actions[x]);
	 if (selection->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }

   /* Set the exit type and return. */
   selection->exitType = vEARLY_EXIT;
   return 0;
}
   
/*
 * This injects a single character into the widget.
 */
int injectCDKSelection (CDKSELECTION *selection, chtype input)
{
   /* Declare local variables. */
   int ppReturn = 1;
   
   /* Set the exit type. */
   selection->exitType = vEARLY_EXIT;

   /* Draw the selection list */
   drawCDKSelectionList (selection);
   
   /* Check if there is a pre-process function to be called. */
   if (selection->preProcessFunction != 0)
   {
      /* Call the pre-process function. */
      ppReturn = ((PROCESSFN)(selection->preProcessFunction)) (vSELECTION, selection, selection->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check for a predefined binding. */
      if (checkCDKObjectBind (vSELECTION, selection, input) != 0)
      {
	 return -1;
      }
      else
      {
	 switch (input)
	 {
	    case KEY_UP :
		 if (selection->currentItem > 0)
		 {
		    selection->currentItem--;
		    if (selection->currentHigh == 0)
		    {
		       selection->currentTop--;
		    }
		    else
		    {
		       selection->currentHigh--;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;
	   
	    case KEY_DOWN :
		 if (selection->currentItem < selection->listSize - 1)
		 {
		    selection->currentItem++;
		    if (selection->currentHigh == selection->viewSize - 1)
		    {
		       selection->currentTop++;
		    }
		    else
		    {
		       selection->currentHigh++;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;
	 
	    case KEY_LEFT :
		 if (selection->leftChar > 0)
		 {
		    selection->leftChar--;
		 }
		 else
		 {
		    Beep();
		 }
		 break;
	 
	    case KEY_RIGHT :
		 if (selection->leftChar < selection->maxLeftChar)
		 {
		    selection->leftChar++;
		 }
		 else
		 {
		    Beep();
		 }
		 break;
	 
	    case KEY_PPAGE :
	    case CONTROL('B') :
		 if (selection->currentItem > 0)
		 {
		    selection->currentTop =
			MAXIMUM (selection->currentTop - selection->viewSize + 1,
				 0);
		    selection->currentItem =
			MAXIMUM (selection->currentItem - selection->viewSize + 1,
				 0);
		    selection->currentHigh =
			selection->currentItem - selection->currentTop;
		 }
		 else
		 {
		    Beep();
		 }
		 break;
	 
	    case KEY_NPAGE :
	    case CONTROL('F') :
		 if (selection->currentItem < selection->listSize - 1)
		 {
		    selection->currentTop =
			MINIMUM (selection->currentTop + selection->viewSize - 1,
				 selection->maxTopItem);
		    selection->currentItem =
			MINIMUM (selection->currentItem + selection->viewSize - 1,
				 selection->listSize-1);
		    selection->currentHigh =
			selection->currentItem - selection->currentTop;
		 }
		 else
		 {
		    Beep();
		 }
		 break;
	 
	    case 'g' :
	    case '1' :
	    case KEY_HOME :
		 selection->currentTop	= 0;
		 selection->currentItem	= 0;
		 selection->currentHigh	= 0;
		 break;
	 
	    case 'G' :
	    case KEY_END :
		 selection->currentTop	= selection->maxTopItem;
		 selection->currentItem	= selection->listSize-1;
		 selection->currentHigh	= selection->currentItem - selection->currentTop;
		 break;
	 
	    case '|' :
		 selection->leftChar	= 0;
		 break;
	 
	    case '$' :
		 selection->leftChar	= selection->maxLeftChar;
		 break;
	 
	    case SPACE :
		 if (selection->mode[selection->currentItem] == 0)
		 {
		    if (selection->selections[selection->currentItem] == selection->choiceCount-1)
		    {
		       selection->selections[selection->currentItem] = 0;
		    }
		    else
		    {
		       selection->selections[selection->currentItem]++;
		    }
		 }
		 else
		 {
		    Beep();
		 }
		 break;
	 
	    case KEY_ESC :
		 selection->exitType = vESCAPE_HIT;
		 return -1;
	 
	    case KEY_RETURN :
	    case KEY_TAB :
	    case KEY_ENTER :
	    case KEY_CR :
		 selection->exitType = vNORMAL;
		 return 1;
	 
	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(selection));
		 refreshCDKScreen (ScreenOf(selection));
		 break;
	 
	    default :
		 Beep();
		 break;
	 }
      }

      /* Should we call a post-process? */
      if (selection->postProcessFunction != 0)
      {
	 ((PROCESSFN)(selection->postProcessFunction)) (vSELECTION, selection, selection->postProcessData, input);
      }
   }
      
   /* Redraw the list */
   drawCDKSelectionList (selection);

   /* Set the exit type and return. */
   selection->exitType = vEARLY_EXIT;
   return -1;
}
   
/*
 * This moves the selection field to the given location.
 */
static void _moveCDKSelection (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKSELECTION *selection = (CDKSELECTION *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(selection->win);
      yplace += getbegy(selection->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(selection), &xplace, &yplace, selection->boxWidth, selection->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(selection->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKSelection (selection, ObjOf(selection)->box);
   }
}

/*
 * This function draws the selection list.
 */
static void _drawCDKSelection (CDKOBJS *object, boolean Box)
{
   CDKSELECTION *selection = (CDKSELECTION *)object;
   int x;

   /* Box it if needed */
   if (Box)
   {
      attrbox (selection->win,
		selection->ULChar, selection->URChar,
		selection->LLChar, selection->LRChar,
		selection->HChar,  selection->VChar,
		selection->BoxAttrib,
		selection->shadow);
   }

   if (selection->titleLines > 0)
   {
      /* Draw in the title if there is one. */
      for (x=0; x < selection->titleLines; x++)
      {
	 writeChtype (selection->titleWin, 
			selection->titlePos[x], x,
			selection->title[x],
			HORIZONTAL, 0,
			selection->titleLen[x]);
      }
      wnoutrefresh (selection->titleWin);
   }
   
   /* Redraw the list */
   drawCDKSelectionList (selection);
}
  
/*
 * This function draws the selection list window.
 */
static void drawCDKSelectionList (CDKSELECTION *selection)
{
   /* Declare local variables. */
   int screenPos, x, y;
   
   werase (selection->fieldWin);

   /* Redraw the list. */
   for (x=0, y=selection->currentTop; x < selection->viewSize; x++, y++)
   {
      if (y >= selection->listSize)
      {
	 break;
      }

      screenPos = selection->itemPos[y] - selection->leftChar;

      /* Draw in the selection item. */
      if (y == selection->currentItem)
      {
	 if (screenPos >= 0)
	 {
	    writeChtypeAttrib (selection->fieldWin,
			screenPos, x,
			selection->item[y],
			selection->highlight,
			HORIZONTAL, 0,
			selection->itemLen[y]);
	 }
	 else
	 {
	    writeChtypeAttrib (selection->fieldWin,
			0, x,
			selection->item[y],
			selection->highlight,
			HORIZONTAL, -screenPos,
			selection->itemLen[y]);
	 }
      }
      else
      {
	 if (screenPos >= 0)
	 {
	    writeChtype (selection->fieldWin,
			screenPos, x,
			selection->item[y],
			HORIZONTAL, 0,
			selection->itemLen[y]);
	 }
	 else
	 {
	    writeChtype (selection->fieldWin,
			0, x,
			selection->item[y],
			HORIZONTAL, -screenPos,
			selection->itemLen[y]);
	 }
      }

      screenPos = - selection->leftChar;

      /* Draw in the choice value. */
      writeChtype (selection->fieldWin,
			0, x,
			selection->choice[selection->selections[y]],
			HORIZONTAL, -screenPos,
			selection->choicelen[selection->selections[y]]);
   }
   
   if (selection->scrollbar)
   {
      int togglePos;

      /* Determine where the toggle is supposed to be. */
      if (selection->listSize >= selection->viewSize)
	 togglePos = floorCDK ((float)selection->viewSize * (float)selection->currentItem / (float)selection->listSize);
      else
	 togglePos = floorCDK ((float)(selection->viewSize - selection->toggleSize) * (float)selection->currentItem / (float)(selection->listSize - 1));

      /* Make sure the toggle button doesn't go out of bounds. */
      togglePos = MINIMUM (togglePos, selection->viewSize - selection->toggleSize);

      /* Draw the scrollbar. */
      mvwvline (selection->scrollbarWin, 0, 0, ACS_CKBOARD, selection->viewSize);
      mvwvline (selection->scrollbarWin, togglePos, 0, ' ' | A_REVERSE, selection->toggleSize);
      wnoutrefresh (selection->scrollbarWin);
   }

   wnoutrefresh (selection->fieldWin);
   wnoutrefresh (selection->win);
}
   
/*
 * These functions set the drawing characters of the widget.
 */
void setCDKSelectionULChar (CDKSELECTION *selection, chtype character)
{
   selection->ULChar = character;
}
void setCDKSelectionURChar (CDKSELECTION *selection, chtype character)
{
   selection->URChar = character;
}
void setCDKSelectionLLChar (CDKSELECTION *selection, chtype character)
{
   selection->LLChar = character;
}
void setCDKSelectionLRChar (CDKSELECTION *selection, chtype character)
{
   selection->LRChar = character;
}
void setCDKSelectionVerticalChar (CDKSELECTION *selection, chtype character)
{
   selection->VChar = character;
}
void setCDKSelectionHorizontalChar (CDKSELECTION *selection, chtype character)
{
   selection->HChar = character;
}
void setCDKSelectionBoxAttribute (CDKSELECTION *selection, chtype character)
{
   selection->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */ 
void setCDKSelectionBackgroundColor (CDKSELECTION *selection, char *color)
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
   wbkgd (selection->win, holder[0]);
   if (selection->scrollbar)
   {
      wbkgd (selection->scrollbarWin, holder[0]);
   }

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This function destroys the selection list.
 */
void destroyCDKSelection (CDKSELECTION *selection)
{
   /* Declare local variables. */
   int x;
   
   /* Erase the object. */
   eraseCDKSelection (selection);
   
   /* Clean up the char pointers. */
   for (x=0; x < selection->titleLines; x++)
   {
      freeChtype (selection->title[x]);
   }
   for (x=0; x < selection->listSize; x++)
   {
      freeChtype (selection->item[x]);
   }
   for (x=0; x < selection->choiceCount; x++)
   {
      freeChtype (selection->choice[x]);
   }
   
   /* Clean up the windows. */
   deleteCursesWindow (selection->win);
   
   /* Unregister this object. */
   unregisterCDKObject (vSELECTION, selection);
   
   /* Finish cleaning up. */
   free (selection);
}
   
/*
 * This function erases the selection list from the screen.
 */
static void _eraseCDKSelection (CDKOBJS *object)
{
   CDKSELECTION *selection = (CDKSELECTION *)object;

   eraseCursesWindow (selection->win);
}

static void createCDKSelectionItemList (CDKSELECTION *selection, char **list, int listSize)
{
   int widestItem, x;

   /* Is the view size smaller then the window??? */
   selection->listSize		= listSize;
   selection->maxTopItem	= MAXIMUM (0, listSize - selection->viewSize);
   
   /* Determine the size of the scrollbar toggle and the step. */
   if (listSize >= selection->viewSize)
      selection->toggleSize = 1;
   else
      selection->toggleSize = floorCDK ((float)selection->viewSize / (float)listSize + 0.5);

   /* Each item in the needs to be converted to chtype * */
   widestItem			= 0;
   for (x=0; x < listSize; x++)
   {
      selection->item[x]	= char2Chtype (list[x], 
					&selection->itemLen[x],
					&selection->itemPos[x]);
      selection->itemPos[x]	= justifyString (
					selection->fieldWidth - selection->maxchoicelen, 
					selection->itemLen[x], 
					selection->itemPos[x]) + selection->maxchoicelen;
      selection->selections[x]	= 0;
      widestItem = MAXIMUM (widestItem, selection->itemLen[x]);

      /* Set the mode of the selection lists. */
      selection->mode[x] = 0;
   }
   
   /*
    * Determine how many characters we can shift to the right
    * before all the items have been scrolled off the screen.
    */
   selection->maxLeftChar = MAXIMUM (0, widestItem - (selection->fieldWidth - selection->maxchoicelen));
}
   
/*
 * This function sets a couple of the selection list attributes.
 */
void setCDKSelection (CDKSELECTION *selection, chtype highlight, int choices[], boolean Box)
{
   setCDKSelectionChoices (selection, choices);
   setCDKSelectionHighlight(selection, highlight);
   setCDKSelectionBox (selection, Box);
}

/*
 * This sets the selection list items.
 */
void setCDKSelectionItems (CDKSELECTION *selection, char **list, int listSize)
{
   /* Declare some wars. */
   int x;

   /* Clean out the old list. */
   for (x=0; x < selection->listSize; x++)
   {
      freeChtype (selection->item[x]);
      selection->itemLen[x] = 0;
      selection->itemPos[x] = 0;
   }

   /* Set some vars. */
   selection->currentTop	= 0;
   selection->currentItem	= 0;
   selection->currentHigh	= 0;
   selection->leftChar		= 0;

   createCDKSelectionItemList (selection, list, listSize);
}
int getCDKSelectionItems (CDKSELECTION *selection, char *list[])
{
   int x;

   for (x=0; x < selection->listSize; x++)
   {
      list[x] = chtype2Char (selection->item[x]);
   }
   return selection->listSize;
}

/*
 * This sets the highlight bar.
 */
void setCDKSelectionHighlight (CDKSELECTION *selection, chtype highlight)
{
   selection->highlight = highlight;
}
chtype getCDKSelectionHighlight (CDKSELECTION *selection)
{
   return selection->highlight;
}

/*
 * This sets the default choices for the selection list.
 */
void setCDKSelectionChoices (CDKSELECTION *selection, int choices[])
{
   /* Declare local variables. */
   int x;
   
   /* Set the choice values in the selection list. */
   for (x=0; x < selection->listSize; x++)
   {
      if (choices[x] < 0)
      {
	 selection->selections[x] = 0;
      }
      else if (choices[x] > selection->choiceCount)
      {
	 selection->selections[x] = selection->choiceCount-1;
      }
      else
      {
	 selection->selections[x] = choices[x];
      }
   }
}
int *getCDKSelectionChoices (CDKSELECTION *selection)
{
   return selection->selections;
}

/*
 * This sets a single item's choice value.
 */
void setCDKSelectionChoice (CDKSELECTION *selection, int Index, int choice)
{
   int correctChoice = choice;
   int correctIndex = Index;

   /* Verify that the choice value is in range. */
   if (choice < 0)
   {
      correctChoice = 0;
   }
   else if (choice > selection->choiceCount)
   {
      correctChoice = selection->choiceCount-1;
   }

   /* Make sure the index isn't out of range. */
   if (Index < 0)
   {
      Index = 0;
   }
   else if (Index > selection->listSize)
   {
      Index = selection->listSize-1;
   }

   /* Set the choice value. */
   selection->selections[correctIndex] = correctChoice;
}
int getCDKSelectionChoice (CDKSELECTION *selection, int Index)
{
   /* Make sure the index isn't out of range. */
   if (Index < 0)
   {
      return selection->selections[0];
   }
   else if (Index > selection->listSize)
   {
      return selection->selections[selection->listSize-1];
   }
   else
   {
      return selection->selections[Index];
   }
}

/*
 * This sets the modes of the items in the selection list. Currently
 * there are only two: editable=0 and read-only=1
 */
void setCDKSelectionModes (CDKSELECTION *selection, int modes[])
{
   int x;

   /* Make sure the widget pointer is not null. */
   if (selection == 0)
   {
      return;
   }

   /* Set the modes. */
   for (x=0; x < selection->listSize; x++)
   {
      selection->mode[x] = modes[x];
   }
}
int *getCDKSelectionModes (CDKSELECTION *selection)
{
   return selection->mode;
}

/*
 * This sets a single mode of an item in the selection list.
 */
void setCDKSelectionMode (CDKSELECTION *selection, int Index, int mode)
{
   /* Make sure the widget pointer is not null. */
   if (selection == 0)
   {
      return;
   }
  
   /* Make sure the index isn't out of range. */
   if (Index < 0)
   {
      selection->mode[0] = mode;
   }
   else if (Index > selection->listSize)
   {
      selection->mode[selection->listSize-1] = mode;
   }
   else
   {
      selection->mode[Index] = mode;
   }
}
int getCDKSelectionMode (CDKSELECTION *selection, int Index)
{
   /* Make sure the index isn't out of range. */
   if (Index < 0)
   {
      return selection->mode[0];
   }
   else if (Index > selection->listSize)
   {
      return selection->mode[selection->listSize-1];
   }
   else
   {
      return selection->mode[Index];
   }
}

/*
 * This sets the box attribute of the widget.
 */
void setCDKSelectionBox (CDKSELECTION *selection, boolean Box)
{
   ObjOf(selection)->box = Box;
}
boolean getCDKSelectionBox (CDKSELECTION *selection)
{
   return ObjOf(selection)->box;
}

/*
 * This function sets the pre-process function.
 */
void setCDKSelectionPreProcess (CDKSELECTION *selection, PROCESSFN callback, void *data)
{
   selection->preProcessFunction = callback;
   selection->preProcessData = data;
}
 
/*
 * This function sets the post-process function.
 */
void setCDKSelectionPostProcess (CDKSELECTION *selection, PROCESSFN callback, void *data)
{
   selection->postProcessFunction = callback;
   selection->postProcessData = data;
}
