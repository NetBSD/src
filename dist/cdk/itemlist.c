#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/09 18:41:53 $
 * $Revision: 1.3 $
 */

DeclareCDKObjects(my_funcs,Itemlist);

/*
 * This creates a pointer to an itemlist widget.
 */
CDKITEMLIST *newCDKItemlist (CDKSCREEN *cdkscreen, int xplace, int yplace, char *title, char *label, char **item, int count, int defaultItem, boolean Box, boolean shadow)
{
   /* Set up some variables.  */
   CDKITEMLIST *itemlist = newCDKObject(CDKITEMLIST, &my_funcs);
   chtype *holder	= 0;
   int parentWidth	= getmaxx(cdkscreen->window);
   int parentHeight	= getmaxy(cdkscreen->window);
   int boxWidth		= 0;
   int boxHeight	= 3;
   int maxWidth		= INT_MIN;
   int fieldWidth	= 0;
   int xpos		= xplace;
   int ypos		= yplace;
   int horizontalAdjust = 0;
   char **temp		= 0;
   int x, len, junk, junk2;

   /* Set some basic values of the itemlist. */
   itemlist->label	= 0;
   itemlist->titleLines = 0;
   itemlist->labelLen	= 0;
   itemlist->labelWin	= 0;

   /* Translate the label char *pointer to a chtype pointer. */
   if (label != 0)
   {
      itemlist->label = char2Chtype (label, &itemlist->labelLen, &junk);
   }

   /* Go through the list and determine the widest item. */
   for (x=0; x < count; x++)
   {
      /* Copy the item to the list. */
      itemlist->item[x] = char2Chtype (item[x], &itemlist->itemLen[x], &itemlist->itemPos[x]);
      maxWidth = MAXIMUM (maxWidth, itemlist->itemLen[x]);
   }

   /* Set the field width and the box width. */
   fieldWidth = maxWidth;
   boxWidth = fieldWidth + itemlist->labelLen + 2;

   /* Now we need to justify the strings. */
   for (x=0; x < count; x++)
   {
      itemlist->itemPos[x] = justifyString (fieldWidth, itemlist->itemLen[x], itemlist->itemPos[x]);
   }

   /* Translate the char * items to chtype * */
   if (title != 0)
   {
      temp = CDKsplitString (title, '\n');
      itemlist->titleLines = CDKcountStrings (temp);

      /* We need to determine the widest title line. */
      for (x=0; x < itemlist->titleLines; x++)
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
      for (x=0; x < itemlist->titleLines; x++)
      {
	 itemlist->title[x]	= char2Chtype (temp[x], &itemlist->titleLen[x], &itemlist->titlePos[x]);
	 itemlist->titlePos[x]	= justifyString (boxWidth - 2, itemlist->titleLen[x], itemlist->titlePos[x]);
      }
      CDKfreeStrings(temp);
   }
   else
   {
      /* No title? Set the required variables. */
      itemlist->titleLines = 0;
   }
   boxHeight += itemlist->titleLines;

  /*
   * Make sure we didn't extend beyond the dimensions of the window.
   */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);
   fieldWidth = (fieldWidth > (boxWidth - itemlist->labelLen - 2) ? (boxWidth - itemlist->labelLen - 2) : fieldWidth);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Make the label window. */
   itemlist->win = newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);

   /* Is the window null ??? */
   if (itemlist->win == 0)
   {
      /* Clean up the pointers. */
      freeChtype (itemlist->label);
      free (itemlist);

      /* Exit with null. */
      return (0);
   }
   keypad (itemlist->win, TRUE);
   leaveok (itemlist->win, TRUE);

   if (itemlist->titleLines > 0)
   {
      /* Make the title window. */
      itemlist->titleWin = subwin (itemlist->win,
				itemlist->titleLines, boxWidth - 2,
				ypos + 1, xpos + 1);
   }

   /* Make the label window if there was a label. */
   if (itemlist->label != 0)
   {
      itemlist->labelWin = subwin (itemlist->win, 1, itemlist->labelLen,
					ypos + itemlist->titleLines + 1,
					xpos + horizontalAdjust + 1);
   }

   /* Make the field window. */
   itemlist->fieldWin = subwin (itemlist->win, 1, fieldWidth,
				ypos + itemlist->titleLines + 1,
				xpos + itemlist->labelLen + horizontalAdjust + 1);

   /* Set up the rest of the structure. */
   ScreenOf(itemlist)			= cdkscreen;
   itemlist->parent			= cdkscreen->window;
   itemlist->boxHeight			= boxHeight;
   itemlist->boxWidth			= boxWidth;
   itemlist->fieldWidth			= fieldWidth;
   itemlist->itemCount			= count-1;
   itemlist->exitType			= vNEVER_ACTIVATED;
   ObjOf(itemlist)->box			= Box;
   itemlist->shadow			= shadow;
   itemlist->ULChar			= ACS_ULCORNER;
   itemlist->URChar			= ACS_URCORNER;
   itemlist->LLChar			= ACS_LLCORNER;
   itemlist->LRChar			= ACS_LRCORNER;
   itemlist->HChar			= ACS_HLINE;
   itemlist->VChar			= ACS_VLINE;
   itemlist->BoxAttrib			= A_NORMAL;
   itemlist->preProcessFunction		= 0;
   itemlist->preProcessData		= 0;
   itemlist->postProcessFunction	= 0;
   itemlist->postProcessData		= 0;

   /* Set then default item. */
   if (defaultItem >= 0 && defaultItem <= itemlist->itemCount)
   {
      itemlist->currentItem	= defaultItem;
      itemlist->defaultItem	= defaultItem;
   }
   else
   {
      itemlist->currentItem	= 0;
      itemlist->defaultItem	= 0;
   }

   /* Clean the key bindings. */
   cleanCDKObjectBindings (vITEMLIST, itemlist);

   /* Register this baby.		 */
   registerCDKObject (cdkscreen, vITEMLIST, itemlist);

   /* Return the pointer to the structure */
   return (itemlist);
}

/*
 * This allows the user to play with the widget.
 */
int activateCDKItemlist (CDKITEMLIST *itemlist, chtype *actions)
{
   /* Declare local variables. */
   int ret = -1;

   /* Draw the widget. */
   drawCDKItemlist (itemlist, ObjOf(itemlist)->box);

   /* Check if actions is null. */
   if (actions == 0)
   {
      chtype input = 0;
      for (;;)
      {
	 /* Get the input. */
	 wrefresh (itemlist->win);
	 input = wgetch (itemlist->win);

	 /* Inject the character into the widget. */
	 ret = injectCDKItemlist (itemlist, input);
	 if (itemlist->exitType != vEARLY_EXIT)
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
	 ret = injectCDKItemlist (itemlist, actions[x]);
	 if (itemlist->exitType != vEARLY_EXIT)
	 {
	    return ret;
	 }
      }
   }

   /* Set the exit type and exit. */
   itemlist->exitType = vEARLY_EXIT;
   return ret;
}

/*
 * This injects a single character into the widget.
 */
int injectCDKItemlist (CDKITEMLIST *itemlist, chtype input)
{
   /* Declare local variables. */
   int ppReturn = 1;

   /* Set the exit type. */
   itemlist->exitType = vEARLY_EXIT;

   /* Draw the itemlist field. */
   drawCDKItemlistField (itemlist);

   /* Check if there is a pre-process function to be called. */
   if (itemlist->preProcessFunction != 0)
   {
      /* Call the pre-process function. */
      ppReturn = ((PROCESSFN)(itemlist->preProcessFunction)) (vITEMLIST, itemlist, itemlist->preProcessData, input);
   }

   /* Should we continue? */
   if (ppReturn != 0)
   {
      /* Check a predefined binding. */
      if (checkCDKObjectBind (vITEMLIST, itemlist, input) != 0)
      {
	 return -1;
      }
      else
      {
	 switch (input)
	 {
	    case KEY_UP : case KEY_RIGHT : case ' ' : case '+' : case 'n' :
		 if (itemlist->currentItem < itemlist->itemCount)
		 {
		    itemlist->currentItem++;
		 }
		 else
		 {
		    itemlist->currentItem = 0;
		 }
		 break;

	    case KEY_DOWN : case KEY_LEFT : case '-' : case 'p' :
		 if (itemlist->currentItem > 0)
		 {
		    itemlist->currentItem--;
		 }
		 else
		 {
		    itemlist->currentItem = itemlist->itemCount;
		 }
		 break;

	    case 'd' : case 'D' :
		 itemlist->currentItem = itemlist->defaultItem;
		 break;

	    case '0' :
		 itemlist->currentItem = 0;
		 break;

	    case '$' :
		 itemlist->currentItem = itemlist->itemCount;
		 break;

	    case KEY_ESC :
		 itemlist->exitType = vESCAPE_HIT;
		 return -1;

	    case KEY_RETURN : case KEY_TAB : case KEY_ENTER : case KEY_CR :
		 itemlist->exitType = vNORMAL;
		 return itemlist->currentItem;

	    case CDK_REFRESH :
		 eraseCDKScreen (ScreenOf(itemlist));
		 refreshCDKScreen (ScreenOf(itemlist));
		 break;

	    default :
		 Beep();
		 break;
	 }
      }

      /* Should we call a post-process? */
      if (itemlist->postProcessFunction != 0)
      {
	 ((PROCESSFN)(itemlist->postProcessFunction)) (vITEMLIST, itemlist, itemlist->postProcessData, input);
      }
   }

   /* Redraw the field. */
   drawCDKItemlistField (itemlist);

   /* Set the exit type and leave. */
   itemlist->exitType = vEARLY_EXIT;
   return -1;
}

/*
 * This moves the itemlist field to the given location.
 */
static void _moveCDKItemlist (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKITEMLIST *itemlist = (CDKITEMLIST *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(itemlist->win);
      yplace += getbegy(itemlist->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(itemlist), &xplace, &yplace, itemlist->boxWidth, itemlist->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(itemlist->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKItemlist (itemlist, ObjOf(itemlist)->box);
   }
}

/*
 * This draws the widget on the screen.
 */
static void _drawCDKItemlist (CDKOBJS *object, int Box)
{
   CDKITEMLIST *itemlist = (CDKITEMLIST *)object;
   int x;

   /* Erase the widget from the screen. */
   /*eraseCDKItemlist (itemlist); */

   /* Box the widget if asked. */
   if (Box)
   {
      attrbox (itemlist->win,
		itemlist->ULChar, itemlist->URChar,
		itemlist->LLChar, itemlist->LRChar,
		itemlist->HChar,  itemlist->VChar,
		itemlist->BoxAttrib,
		itemlist->shadow);
   }

   if (itemlist->titleLines > 0)
   {
      /* Draw in the title if there is one. */
      for (x=0; x < itemlist->titleLines; x++)
      {
	 writeChtype (itemlist->titleWin,
			itemlist->titlePos[x], x,
			itemlist->title[x],
			HORIZONTAL, 0,
			itemlist->titleLen[x]);
      }
      wnoutrefresh (itemlist->titleWin);
   }

   /* Draw in the label to the widget. */
   if (itemlist->label != 0)
   {
      writeChtype (itemlist->labelWin, 0, 0,
		itemlist->label,
		HORIZONTAL, 0,
		itemlist->labelLen);
      wnoutrefresh (itemlist->labelWin);
   }

   /* Draw in the field. */
   drawCDKItemlistField (itemlist);
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKItemlistULChar (CDKITEMLIST *itemlist, chtype character)
{
   itemlist->ULChar = character;
}
void setCDKItemlistURChar (CDKITEMLIST *itemlist, chtype character)
{
   itemlist->URChar = character;
}
void setCDKItemlistLLChar (CDKITEMLIST *itemlist, chtype character)
{
   itemlist->LLChar = character;
}
void setCDKItemlistLRChar (CDKITEMLIST *itemlist, chtype character)
{
   itemlist->LRChar = character;
}
void setCDKItemlistVerticalChar (CDKITEMLIST *itemlist, chtype character)
{
   itemlist->VChar = character;
}
void setCDKItemlistHorizontalChar (CDKITEMLIST *itemlist, chtype character)
{
   itemlist->HChar = character;
}
void setCDKItemlistBoxAttribute (CDKITEMLIST *itemlist, chtype character)
{
   itemlist->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKItemlistBackgroundColor (CDKITEMLIST *itemlist, char *color)
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
   wbkgd (itemlist->win, holder[0]);
   wbkgd (itemlist->fieldWin, holder[0]);
   if (itemlist->label != 0)
   {
      wbkgd (itemlist->labelWin, holder[0]);
   }

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This function draws the contents of the field.
 */
void drawCDKItemlistField (CDKITEMLIST *itemlist)
{
   /* Declare local vars. */
   int currentItem = itemlist->currentItem;

   /* Erase the field window. */
   werase (itemlist->fieldWin);

   /* Draw in the current item in the field. */
   writeChtype (itemlist->fieldWin,
		itemlist->itemPos[currentItem], 0,
		itemlist->item[currentItem],
		HORIZONTAL, 0,
		itemlist->itemLen[currentItem]);

   /* Redraw the field window. */
   wnoutrefresh (itemlist->fieldWin);
   wnoutrefresh (itemlist->win);
}

/*
 * This function removes the widget from the screen.
 */
static void _eraseCDKItemlist (CDKOBJS *object)
{
   CDKITEMLIST *itemlist = (CDKITEMLIST *)object;

   eraseCursesWindow (itemlist->win);
}

/*
 * This function destroys the widget and all the memory it used.
 */
void destroyCDKItemlist (CDKITEMLIST *itemlist)
{
   /* Declare local variables. */
   int x;

   /* Erase the object. */
   eraseCDKItemlist (itemlist);

   /* Clear out the character pointers. */
   freeChtype (itemlist->label);
   for (x=0; x < itemlist->titleLines; x++)
   {
      freeChtype (itemlist->title[x]);
   }
   for (x=0; x <= itemlist->itemCount; x++)
   {
      freeChtype (itemlist->item[x]);
   }

   /* Delete the windows. */
   deleteCursesWindow (itemlist->win);

   /* Unregister this object. */
   unregisterCDKObject (vITEMLIST, itemlist);

   /* Finish cleaning up. */
   free (itemlist);
}

/*
 * This sets multiple attributes of the widget.
 */
void setCDKItemlist (CDKITEMLIST *itemlist, char **list, int count, int current, boolean Box)
{
   setCDKItemlistValues (itemlist, list, count, current);
   setCDKItemlistBox (itemlist, Box);
}

/*
 * This function sets the contents of the list.
 */
void setCDKItemlistValues (CDKITEMLIST *itemlist, char **item, int count, int defaultItem)
{
   /* Declare local variables. */
   int x;

   /* Free up the old memory. */
   for (x=0; x <= itemlist->itemCount; x++)
   {
      freeChtype (itemlist->item[x]);
   }

   /* Copy in the new information. */
   itemlist->itemCount = count-1;
   for (x=0; x <= itemlist->itemCount; x++)
   {
      /* Copy the new stuff in. */
      itemlist->item[x] = char2Chtype (item[x], &itemlist->itemLen[x], &itemlist->itemPos[x]);
      itemlist->itemPos[x] = justifyString (itemlist->fieldWidth, itemlist->itemLen[x], itemlist->itemPos[x]);
   }

   /* Set the default item. */
   if ((defaultItem >= 0) && (defaultItem <= itemlist->itemCount))
   {
      itemlist->currentItem = defaultItem;
      itemlist->defaultItem = defaultItem;
   }

   /* Draw the field. */
   eraseCDKItemlist (itemlist);
   drawCDKItemlist (itemlist, ObjOf(itemlist)->box);
}
chtype **getCDKItemlistValues (CDKITEMLIST *itemlist, int *size)
{
   (*size) = itemlist->itemCount;
   return itemlist->item;
}

/*
 * This sets the default/current item of the itemlist.
 */
void setCDKItemlistCurrentItem (CDKITEMLIST *itemlist, int currentItem)
{
   /* Set the default item. */
   if ((currentItem >= 0) && (currentItem <= itemlist->itemCount))
   {
      itemlist->currentItem = currentItem;
   }
}
int getCDKItemlistCurrentItem (CDKITEMLIST *itemlist)
{
   return itemlist->currentItem;
}

/*
 * This sets the default item in the list.
 */
void setCDKItemlistDefaultItem (CDKITEMLIST *itemlist, int defaultItem)
{
   /* Make sure the item is in the correct range. */
   if (defaultItem < 0)
   {
      itemlist->defaultItem = 0;
   }
   else if (defaultItem > itemlist->itemCount)
   {
      itemlist->defaultItem = itemlist->itemCount-1;
   }
   else
   {
      itemlist->defaultItem = defaultItem;
   }
}
int getCDKItemlistDefaultItem (CDKITEMLIST *itemlist)
{
   return itemlist->defaultItem;
}

/*
 * This sets the box attribute of the itemlist widget.
 */
void setCDKItemlistBox (CDKITEMLIST *itemlist, boolean Box)
{
   ObjOf(itemlist)->box = Box;
}
boolean getCDKItemlistBox (CDKITEMLIST *itemlist)
{
   return ObjOf(itemlist)->box;
}

/*
 * This function sets the pre-process function.
 */
void setCDKItemlistPreProcess (CDKITEMLIST *itemlist, PROCESSFN callback, void *data)
{
   itemlist->preProcessFunction = callback;
   itemlist->preProcessData = data;
}

/*
 * This function sets the post-process function.
 */
void setCDKItemlistPostProcess (CDKITEMLIST *itemlist, PROCESSFN callback, void *data)
{
   itemlist->postProcessFunction = callback;
   itemlist->postProcessData = data;
}
