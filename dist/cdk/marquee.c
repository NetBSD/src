#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/04 19:58:05 $
 * $Revision: 1.1.1.1 $
 */

DeclareCDKObjects(my_funcs,Marquee);

/*
 * This creates a marquee widget.
 */
CDKMARQUEE *newCDKMarquee (CDKSCREEN *cdkscreen, int xplace, int yplace, int width, boolean Box, boolean shadow)
{
   CDKMARQUEE *marquee	= newCDKObject(CDKMARQUEE, &my_funcs);
   int parentWidth	= getmaxx(cdkscreen->window);
   int xpos		= xplace;
   int ypos		= yplace;
   int boxHeight	= 3;
   int boxWidth		= width;

  /*
   * If the width is a negative value, the width will
   * be COLS-width, otherwise, the width will be the
   * given width.
   */
   boxWidth = setWidgetDimension (parentWidth, width, 0);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Create the marquee pointer. */
   ScreenOf(marquee)	= cdkscreen;
   ObjOf(marquee)->box	= Box;
   marquee->parent	= cdkscreen->window;
   marquee->win		= newwin (boxHeight + !!shadow, boxWidth + !!shadow, ypos, xpos);
   marquee->boxHeight	= boxHeight;
   marquee->boxWidth	= boxWidth;
   marquee->active	= TRUE;
   marquee->width	= width;
   marquee->shadow	= shadow;
   marquee->ULChar	= ACS_ULCORNER;
   marquee->URChar	= ACS_URCORNER;
   marquee->LLChar	= ACS_LLCORNER;
   marquee->LRChar	= ACS_LRCORNER;
   marquee->HChar	= ACS_HLINE;
   marquee->VChar	= ACS_VLINE;
   marquee->BoxAttrib	= A_NORMAL;

   /* Is the window null??? */
   if (marquee->win == 0)
   {
      /* Clean up any memory. */
      free (marquee);

      /* Return a null pointer. */
      return ( 0 );
   }
   keypad (marquee->win, TRUE);
   leaveok (marquee->win, TRUE);

   /* Register this baby. */
   registerCDKObject (cdkscreen, vMARQUEE, marquee);

   /* Return the marquee pointer. */
   return(marquee);
}

/*
 * This activates the marquee.
 */
int activateCDKMarquee (CDKMARQUEE *marquee, char *mesg, int delay, int repeat, boolean Box)
{
   /* Declear local variables. */
   chtype *message;
   int mesgLength	= 0;
   int startPos		= 0;
   int firstChar	= 0;
   int lastChar		= 1;
   int repeatCount	= 0;
   int viewSize		= 0;
   int x, y, junk;

   /* Keep the box info. */
   ObjOf(marquee)->box = Box;

   /* Make sure the message has some content. */
   if (mesg == 0)
   {
      return (-1);
   }

   /* Translate the char * to a chtype * */
   message = char2Chtype (mesg, &mesgLength, &junk);

   /* Draw in the marquee. */
   drawCDKMarquee (marquee, ObjOf(marquee)->box);

   /* Set up the variables. */
   viewSize = lastChar - firstChar;
   startPos = marquee->width - viewSize;
   if (ObjOf(marquee)->box == TRUE)
   {
      startPos--;
   }

   /* Start doing the marquee thing... */
   for (;;)
   {
      if (marquee->active)
      {
	 /* Draw in the characters. */
	 writeChtype (marquee->win, startPos, 1, message, HORIZONTAL,
			firstChar, lastChar);

	 /* Set my variables. */
	 if (mesgLength < (marquee->width-2))
	 {
	    if (lastChar < mesgLength)
	    {
	       startPos = marquee->width - viewSize + 1;
	       lastChar ++;
	       viewSize ++;
	    }
	    else if (lastChar == mesgLength)
	    {
	       if (startPos > 1)
	       {
		  /* This means the whole string is visible. */
		  startPos --;
		  viewSize = mesgLength - 1;
	       }
	       else
	       {
		 /* We have to start chopping the viewSize */
		 startPos = 1;
		 firstChar++;
		 viewSize--;
	       }
	    }
	 }
	 else
	 {
	    if (startPos > 1)
	    {
	       startPos --;
	       lastChar ++;
	       viewSize ++;
	    }
	    else
	    {
	       startPos = 1;
	       firstChar ++;
	       if (lastChar < mesgLength)
	       {
		  lastChar ++;
		  viewSize = marquee->width - 2;
	       }
	       else
	       {
		  viewSize --;
	       }
	    }
	 }

	 /* OK, lets check if we have to start over. */
	 if ( viewSize == 0 && firstChar == mesgLength)
	 {
	    /* Check if we need to repeat or not. */
	    repeatCount ++;
	    if (repeat > 0 && repeatCount == repeat)
	    {
	       freeChtype (message);
	       return (0);
	    }

	    /* Time to start over.  */
	    mvwaddch (marquee->win, 1, 1, ' '|A_NORMAL);
	    firstChar = 0;
	    lastChar = 1;
	    viewSize = lastChar - firstChar;
	    startPos = marquee->width - viewSize;
	    if (ObjOf(marquee)->box)
	    {
	       startPos--;
	    }
	 }

	 /* Now sleep */
	 wrefresh (marquee->win);
	 napms (delay * 10);
      }
   }
}

/*
 * This de-activates a marquee widget.
 */
void deactivateCDKMarquee (CDKMARQUEE *marquee)
{
   marquee->active = FALSE;
}

/*
 * This moves the marquee field to the given location.
 */
static void _moveCDKMarquee (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKMARQUEE *marquee = (CDKMARQUEE *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(marquee->win);
      yplace += getbegy(marquee->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(marquee), &xplace, &yplace, marquee->boxWidth, marquee->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(marquee->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKMarquee (marquee, ObjOf(marquee)->box);
   }
}

/*
 * This draws the marquee widget on the screen.
 */
static void _drawCDKMarquee (CDKOBJS *object, boolean Box)
{
   CDKMARQUEE *marquee = (CDKMARQUEE *)object;

   /* Keep the box information. */
   ObjOf(marquee)->box	= Box;

   /* Box it if needed. */
   if (Box)
   {
      attrbox (marquee->win,
		marquee->ULChar, marquee->URChar,
		marquee->LLChar, marquee->LRChar,
		marquee->HChar,	 marquee->VChar,
		marquee->BoxAttrib,
		marquee->shadow);
   }

   /* Refresh the window. */
   wnoutrefresh (marquee->win);
}

/*
 * This destroys the marquee.
 */
void destroyCDKMarquee (CDKMARQUEE *marquee)
{
   /* Erase the object. */
   eraseCDKMarquee (marquee);

   /* Clean up the windows. */
   deleteCursesWindow (marquee->win);

   /* Unregister this object. */
   unregisterCDKObject (vMARQUEE, marquee);

   /* Finish cleaning up. */
   free (marquee);
}

/*
 * This erases the marquee.
 */
static void _eraseCDKMarquee (CDKOBJS *object)
{
   CDKMARQUEE *marquee = (CDKMARQUEE *)object;

   eraseCursesWindow (marquee->win);
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKMarqueeULChar (CDKMARQUEE *marquee, chtype character)
{
   marquee->ULChar = character;
}
void setCDKMarqueeURChar (CDKMARQUEE *marquee, chtype character)
{
   marquee->URChar = character;
}
void setCDKMarqueeLLChar (CDKMARQUEE *marquee, chtype character)
{
   marquee->LLChar = character;
}
void setCDKMarqueeLRChar (CDKMARQUEE *marquee, chtype character)
{
   marquee->LRChar = character;
}
void setCDKMarqueeVerticalChar (CDKMARQUEE *marquee, chtype character)
{
   marquee->VChar = character;
}
void setCDKMarqueeHorizontalChar (CDKMARQUEE *marquee, chtype character)
{
   marquee->HChar = character;
}
void setCDKMarqueeBoxAttribute (CDKMARQUEE *marquee, chtype character)
{
   marquee->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKMarqueeBackgroundColor (CDKMARQUEE *marquee, char *color)
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
   wbkgd (marquee->win, holder[0]);

   /* Clean up. */
   freeChtype (holder);
}

