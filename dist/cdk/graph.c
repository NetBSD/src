#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/04 19:58:09 $
 * $Revision: 1.1.1.1 $
 */

DeclareCDKObjects(my_funcs,Graph);

/*
 * This creates a graph widget.
 */
CDKGRAPH *newCDKGraph (CDKSCREEN *cdkscreen, int xplace, int yplace, int height, int width, char *title, char *xtitle, char *ytitle)
{
   /* Declare local variables. */
   CDKGRAPH *graph	= newCDKObject(CDKGRAPH, &my_funcs);
   int parentWidth	= getmaxx(cdkscreen->window);
   int parentHeight	= getmaxy(cdkscreen->window);
   int boxWidth		= width;
   int boxHeight	= height;
   int xpos		= xplace;
   int ypos		= yplace;
   char **temp		= 0;
   int x;

  /*
   * If the height is a negative value, the height will
   * be ROWS-height, otherwise, the height will be the
   * given height.
   */
   boxHeight = setWidgetDimension (parentHeight, height, 3);

  /*
   * If the width is a negative value, the width will
   * be COLS-width, otherwise, the width will be the
   * given width.
   */
   boxWidth = setWidgetDimension (parentWidth, width, 0);

  /*
   * If the width is a negative value, the width will
   * be COLS-width, otherwise, the width will be the
   * given width.
   */
   boxWidth = setWidgetDimension (parentWidth, width, 0);

   /* Translate the char * items to chtype * */
   if (title != 0)
   {
      /* We need to split the title on \n. */
      temp = CDKsplitString (title, '\n');
      graph->titleLines = CDKcountStrings (temp);

      /* For each line in the title, convert from char * to chtype * */
      for (x=0; x < graph->titleLines; x++)
      {
	 graph->title[x]	= char2Chtype (temp[x], &graph->titleLen[x], &graph->titlePos[x]);
	 graph->titlePos[x]	= justifyString (boxWidth, graph->titleLen[x], graph->titlePos[x]);
      }

      CDKfreeStrings(temp);
   }
   else
   {
      /* No title? Set the required variables. */
      graph->titleLines = 0;
   }
   boxHeight += graph->titleLines;

  /*
   * Make sure we didn't extend beyond the dimensions of the window.
   */
   boxWidth = MINIMUM (boxWidth, parentWidth);
   boxHeight = MINIMUM (boxHeight, parentHeight);

   /* Rejustify the x and y positions if we need to. */
   alignxy (cdkscreen->window, &xpos, &ypos, boxWidth, boxHeight);

   /* Create the graph pointer. */
   ScreenOf(graph)	= cdkscreen;
   graph->parent	= cdkscreen->window;
   graph->win		= newwin (boxHeight, boxWidth, ypos, xpos);
   graph->boxHeight	= boxHeight;
   graph->boxWidth	= boxWidth;
   ObjOf(graph)->box	= FALSE;
   graph->minx		= 0;
   graph->maxx		= 0;
   graph->xscale	= 0;
   graph->yscale	= 0;
   graph->count		= 0;
   graph->displayType	= vLINE;
   graph->ULChar	= ACS_ULCORNER;
   graph->URChar	= ACS_URCORNER;
   graph->LLChar	= ACS_LLCORNER;
   graph->LRChar	= ACS_LRCORNER;
   graph->HChar		= ACS_HLINE;
   graph->VChar		= ACS_VLINE;
   graph->BoxAttrib	= A_NORMAL;

   /* Is the graph pointer null? */
   if (graph->win == 0)
   {
      /* Clean up any memory used. */
      free (graph);

      /* Return a null pointer. */
      return ( 0 );
   }
   keypad (graph->win, TRUE);
   leaveok (graph->win, TRUE);

   /* Translate the X Axis title char * to a chtype * */
   if (xtitle != 0)
   {
      graph->xtitle	= char2Chtype (xtitle, &graph->xtitleLen, &graph->xtitlePos);
      graph->xtitlePos	= justifyString (graph->boxHeight, graph->xtitleLen, graph->xtitlePos);
   }
   else
   {
      graph->xtitle	= char2Chtype ("<C></5>X Axis", &graph->xtitleLen, &graph->xtitlePos);
      graph->xtitlePos	= justifyString (graph->boxHeight, graph->xtitleLen, graph->xtitlePos);
   }

   /* Translate the Y Axis title char * to a chtype * */
   if (ytitle != 0)
   {
      graph->ytitle	= char2Chtype (ytitle, &graph->ytitleLen, &graph->ytitlePos);
      graph->ytitlePos	= justifyString (graph->boxWidth, graph->ytitleLen, graph->ytitlePos);
   }
   else
   {
      graph->ytitle	= char2Chtype ("<C></5>Y Axis", &graph->ytitleLen, &graph->ytitlePos);
      graph->ytitlePos	= justifyString (graph->boxWidth, graph->ytitleLen, graph->ytitlePos);
   }

   /* Set some values of the graph structure. */
   graph->graphChar = 0;

   /* Register this baby. */
   registerCDKObject (cdkscreen, vGRAPH, graph);

   /* Return the graph pointer. */
   return (graph);
}

/*
 * This was added for the builder.
 */
void activateCDKGraph (CDKGRAPH *graph, chtype *actions GCC_UNUSED)
{
   drawCDKGraph (graph, ObjOf(graph)->box);
}

/*
 * This sets multiple attributes of the widget.
 */
int setCDKGraph (CDKGRAPH *graph, int *values, int count, char *graphChar, boolean startAtZero, EGraphDisplayType displayType)
{
   int ret;

   ret = setCDKGraphValues (graph, values, count, startAtZero);
   setCDKGraphCharacters (graph, graphChar);
   setCDKGraphDisplayType (graph, displayType);
   return ret;
}

/*
 * This sets the values of the graph.
 */
int setCDKGraphValues (CDKGRAPH *graph, int *values, int count, boolean startAtZero)
{
   /* Declare local variables. */
   int min		= INT_MAX;
   int max		= INT_MIN;
   int x;

   /* Make sure everything is happy. */
   if (count < 0)
   {
      return (FALSE);
   }

   /* Copy the X values. */
   for (x=0; x < count; x++)
   {
      /* Determine the min/max values of the graph. */
      min = MINIMUM (values[x], graph->minx);
      max = MAXIMUM (values[x], graph->maxx);

      /* Copy the value. */
      graph->values[x]	= values[x];
   }

   /* Keep the count and min/max values. */
   graph->count = count;
   graph->minx = min;
   graph->maxx = max;

   /* Check the start at zero status. */
   if (startAtZero)
   {
      graph->minx = 0;
   }

   /* Determine the scales. */
   graph->xscale = ((graph->maxx - graph->minx) / (graph->boxHeight - graph->titleLines - 5));
   graph->yscale = ((graph->boxWidth-4) / count);
   return (TRUE);
}
int *getCDKGraphValues (CDKGRAPH *graph, int *size)
{
   (*size) = graph->count;
   return graph->values;
}

/*
 * This sets the value of the graph at the given index.
 */
int setCDKGraphValue (CDKGRAPH *graph, int Index, int value, boolean startAtZero)
{
   /* Make sure the index is within range. */
   if (Index < 0 || Index > graph->count)
   {
      return (FALSE);
   }

   /* Set the min, max, and value for the graph. */
   graph->minx = MINIMUM (value, graph->minx);
   graph->maxx = MAXIMUM (value, graph->maxx);
   graph->values[Index] = value;

   /* Check the start at zero status. */
   if (startAtZero)
   {
      graph->minx = 0;
   }

   /* Determine the scales. */
   graph->xscale = ((graph->maxx - graph->minx) / (graph->boxHeight - 5));
   graph->yscale = ((graph->boxWidth-4) / graph->count);
   return (TRUE);
}
int getCDKGraphValue (CDKGRAPH *graph, int Index)
{
   return graph->values[Index];
}

/*
 * This sets the characters of the graph widget.
 */
int setCDKGraphCharacters (CDKGRAPH *graph, char *characters)
{
   /* Declare local variables. */
   chtype *newTokens = 0;
   int charCount, junk;

   /* Convert the string given to us. */
   newTokens = char2Chtype (characters, &charCount, &junk);

  /*
   * Check if the number of characters back is the same as the number
   * of elements in the list.
   */
   if (charCount != graph->count)
   {
      freeChtype (newTokens);
      return (FALSE);
   }

   /* Evrything OK so far. Nuke the old pointer and use the new one. */
   freeChtype (graph->graphChar);
   graph->graphChar = newTokens;
   return (TRUE);
}
chtype *getCDKGraphCharacters (CDKGRAPH *graph)
{
   return graph->graphChar;
}

/*
 * This sets the character of the graph widget of the given index.
 */
int setCDKGraphCharacter (CDKGRAPH *graph, int Index, char *character)
{
   /* Declare local variables. */
   chtype *newTokens = 0;
   int charCount, junk;

   /* Make sure the index is within range. */
   if (Index < 0 || Index > graph->count)
   {
      return (FALSE);
   }

   /* Convert the string given to us. */
   newTokens = char2Chtype (character, &charCount, &junk);

  /*
   * Check if the number of characters back is the same as the number
   * of elements in the list.
   */
   if (charCount != graph->count)
   {
      freeChtype (newTokens);
      return (FALSE);
   }

   /* Evrything OK so far. Set the value of the array. */
   graph->graphChar[Index] = newTokens[0];
   freeChtype (newTokens);
   return (TRUE);
}
chtype getCDKGraphCharacter (CDKGRAPH *graph, int Index)
{
   return graph->graphChar[Index];
}

/*
 * This sets the display type of the graph.
 */
void setCDKGraphDisplayType (CDKGRAPH *graph, EGraphDisplayType type)
{
   graph->displayType = type;
}
EGraphDisplayType getCDKGraphDisplayType (CDKGRAPH *graph)
{
   return graph->displayType;
}

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKGraphULChar (CDKGRAPH *graph, chtype character)
{
   graph->ULChar = character;
}
void setCDKGraphURChar (CDKGRAPH *graph, chtype character)
{
   graph->URChar = character;
}
void setCDKGraphLLChar (CDKGRAPH *graph, chtype character)
{
   graph->LLChar = character;
}
void setCDKGraphLRChar (CDKGRAPH *graph, chtype character)
{
   graph->LRChar = character;
}
void setCDKGraphVerticalChar (CDKGRAPH *graph, chtype character)
{
   graph->VChar = character;
}
void setCDKGraphHorizontalChar (CDKGRAPH *graph, chtype character)
{
   graph->HChar = character;
}
void setCDKGraphBoxAttribute (CDKGRAPH *graph, chtype character)
{
   graph->BoxAttrib = character;
}

/*
 * This sets the background color of the widget.
 */
void setCDKGraphBackgroundColor (CDKGRAPH *graph, char *color)
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
   wbkgd (graph->win, holder[0]);

   /* Clean up. */
   freeChtype (holder);
}

/*
 * This moves the graph field to the given location.
 */
static void _moveCDKGraph (CDKOBJS *object, int xplace, int yplace, boolean relative, boolean refresh_flag)
{
   CDKGRAPH *graph = (CDKGRAPH *)object;

   /*
    * If this is a relative move, then we will adjust where we want
    * to move to.
    */
   if (relative)
   {
      xplace += getbegx(graph->win);
      yplace += getbegy(graph->win);
   }

   /* Adjust the window if we need to. */
   alignxy (WindowOf(graph), &xplace, &yplace, graph->boxWidth, graph->boxHeight);

   /* Move the window to the new location. */
   moveCursesWindow(graph->win, xplace, yplace);

   /* Redraw the window, if they asked for it. */
   if (refresh_flag)
   {
      drawCDKGraph (graph, ObjOf(graph)->box);
   }
}

/*
 * This sets whether or not the graph will be boxed.
 */
void setCDKGraphBox (CDKGRAPH *graph, boolean Box)
{
   ObjOf(graph)->box = Box;
}
boolean getCDKGraphBox (CDKGRAPH *graph)
{
   return ObjOf(graph)->box;
}

/*
 * This function draws the graph widget.
 */
static void _drawCDKGraph (CDKOBJS *object, boolean Box)
{
   CDKGRAPH *graph = (CDKGRAPH *)object;
   int adj		= 2 + (graph->xtitle == 0 ? 0 : 1);
   int spacing		= 0;
   chtype attrib	= ' '|A_REVERSE;
   char temp[100];
   int x, y, xpos, ypos, len;

   /* Box it if needed. */
   if (Box)
   {
      attrbox (graph->win,
		graph->ULChar, graph->URChar,
		graph->LLChar, graph->LRChar,
		graph->HChar,  graph->VChar,
		graph->BoxAttrib,
		FALSE);
   }

   /* Draw in the vertical axis. */
   drawLine (graph->win, 2, graph->titleLines + 1, 2, graph->boxHeight-3, ACS_VLINE);

   /* Draw in the horizontal axis. */
   drawLine (graph->win, 3, graph->boxHeight-3, graph->boxWidth, graph->boxHeight-3, ACS_HLINE);

   /* Draw in the title if there is one. */
   for (x=0; x < graph->titleLines; x++)
   {
      writeChtype (graph->win,
			graph->titlePos[x],
			x + 1,
			graph->title[x],
			HORIZONTAL, 0,
			graph->titleLen[x]);
   }

   /* Draw in the X axis title. */
   if (graph->xtitle != 0)
   {
      writeChtype (graph->win, 0, graph->xtitlePos, graph->xtitle, VERTICAL, 0, graph->xtitleLen);
      attrib	= graph->xtitle[0] & A_ATTRIBUTES;
   }

   /* Draw in the X axis high value. */
   sprintf (temp, "%d", graph->maxx);
   len = (int)strlen (temp);
   writeCharAttrib (graph->win, 1, graph->titleLines + 1, temp, attrib, VERTICAL, 0, len);

   /* Draw in the X axis low value. */
   sprintf (temp, "%d", graph->minx);
   len = (int)strlen (temp);
   writeCharAttrib (graph->win, 1, graph->boxHeight-2-len, temp, attrib, VERTICAL, 0, len);

   /* Draw in the Y axis title. */
   if (graph->ytitle != 0)
   {
      writeChtype (graph->win, graph->ytitlePos, graph->boxHeight-1, graph->ytitle, HORIZONTAL, 0, graph->ytitleLen);
      attrib	= graph->ytitle[0] & A_ATTRIBUTES;
   }

   /* Draw in the Y axis high value. */
   sprintf (temp, "%d", graph->count);
   len = (int)strlen (temp);
   writeCharAttrib (graph->win, graph->boxWidth-len-adj, graph->boxHeight-2, temp, attrib, HORIZONTAL, 0, len);

   /* Draw in the Y axis low value. */
   sprintf (temp, "0");
   writeCharAttrib (graph->win, 3, graph->boxHeight-2, temp, attrib, HORIZONTAL, 0, (int)strlen(temp));

   /* If the count is zero, then there aren't any points. */
   if (graph->count == 0)
   {
      wnoutrefresh (graph->win);
      return;
   }

   spacing = (graph->boxWidth - 3) / graph->count;

   /* Draw in the graph line/plot points. */
   for (y=0; y < graph->count; y++)
   {
       int colheight = (graph->values[y] / graph->xscale) - 1;

       /* Add the marker on the Y axis. */
       mvwaddch (graph->win, graph->boxHeight-3, (y + 1)*spacing + adj, ACS_TTEE);

       /* If this is a plot graph, all we do is draw a dot. */
       if (graph->displayType == vPLOT)
       {
	  xpos = graph->boxHeight-4-colheight;
	  ypos = (y + 1)*spacing + adj;
	  mvwaddch (graph->win, xpos, ypos, graph->graphChar[y]);
       }
       else
       {
	  for (x=0; x <= graph->yscale; x++)
	  {
	     xpos = graph->boxHeight-3;
	     ypos = (y + 1)*spacing + adj;
	     drawLine (graph->win, ypos, xpos-colheight, ypos, xpos, graph->graphChar[y]);
	  }
       }
   }

   /* Draw in the axis corners. */
   mvwaddch (graph->win, graph->titleLines, 2, ACS_URCORNER);
   mvwaddch (graph->win, graph->boxHeight-3, 2, ACS_LLCORNER);
   mvwaddch (graph->win, graph->boxHeight-3, graph->boxWidth, ACS_URCORNER);

   /* Refresh and lets see 'er. */
   wnoutrefresh (graph->win);
}

/*
 * This function destroys the graph widget.
 */
void destroyCDKGraph (CDKGRAPH *graph)
{
   int x;

   /* Erase the object. */
   eraseCDKGraph (graph);

   /* Clear up the char pointers. */
   for (x=0; x < graph->titleLines; x++)
   {
      freeChtype (graph->title[x]);
   }
   freeChtype (graph->xtitle);
   freeChtype (graph->ytitle);
   freeChtype (graph->graphChar);

   /* Clean up the windows. */
   deleteCursesWindow (graph->win);

   /* Unregister this object. */
   unregisterCDKObject (vGRAPH, graph);

   /* Finish cleaning up. */
   free (graph);
}

/*
 * This function erases the graph widget from the screen.
 */
static void _eraseCDKGraph (CDKOBJS *object)
{
   CDKGRAPH *graph = (CDKGRAPH *)object;

   eraseCursesWindow (graph->win);
}
