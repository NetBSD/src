#include <cdk.h>

/*
 * $Author: christos $
 * $Date: 2004/04/02 16:11:17 $
 * $Revision: 1.2 $
 */

static boolean validObjType(EObjectType type)
{
   switch (type) {
   case vALPHALIST:
   case vBUTTONBOX:
   case vCALENDAR:
   case vDIALOG:
   case vENTRY:
   case vFSELECT:
   case vGRAPH:
   case vHISTOGRAM:
   case vITEMLIST:
   case vLABEL:
   case vMARQUEE:
   case vMATRIX:
   case vMENTRY:
   case vMENU:
   case vRADIO:
   case vSCALE:
   case vSCROLL:
   case vSELECTION:
   case vSLIDER:
   case vSWINDOW:
   case vTEMPLATE:
   case vVIEWER:
      return TRUE;
   default:
      return FALSE;
   }
}

void *_newCDKObject(unsigned size, CDKFUNCS *funcs)
{
   void *result = malloc(size);
   memset(result, 0, sizeof(CDKOBJS));
   ((CDKOBJS *)result)->fn = funcs;
   return result;
}

/*
 * This creates a new CDK screen.
 */
CDKSCREEN *initCDKScreen(WINDOW *window)
{
   CDKSCREEN *screen = (CDKSCREEN *)malloc (sizeof(CDKSCREEN));
   int x;

   /* Set up basic curses settings. */
   noecho();
   cbreak();

   /* Initialize the CDKSCREEN pointer. */
   screen->objectCount	= 0;
   screen->window	= window;
   for (x=0; x < MAX_OBJECTS; x++)
   {
      screen->object[x] = 0;
      screen->cdktype[x] = vNULL;
   }

   /* OK, we are done. */
   return (screen);
}

/*
 * This registers a CDK object with a screen.
 */
void registerCDKObject (CDKSCREEN *screen, EObjectType cdktype, void *object)
{
   /* Set some basic vars. */
   int objectNumber			= screen->objectCount;
   CDKOBJS *obj				= (CDKOBJS *)object;

   screen->object[objectNumber]		= obj;

   if (validObjType(cdktype)) {
      (obj)->screenIndex		= objectNumber;
      (obj)->screen			= screen;
      screen->cdktype[objectNumber]	= cdktype;
      screen->objectCount++;
   }
}

/*
 * This removes an object from the CDK screen.
 */
void unregisterCDKObject (EObjectType cdktype, void *object)
{
   /* Declare some vars. */
   CDKSCREEN *screen;
   int Index, x;

   if (validObjType(cdktype)) {
      CDKOBJS *obj = (CDKOBJS *)object;

      screen	= (obj)->screen;
      Index	= (obj)->screenIndex;

      /*
       * If this is the last object -1 then this is the last. If not
       * we have to shuffle all the other objects to the left.
       */
      for (x=Index; x < screen->objectCount-1; x++)
      {
	 cdktype		= screen->cdktype[x+1];
	 screen->cdktype[x]	= cdktype;
	 screen->object[x]	= screen->object[x+1];
	 (screen->object[x])->screenIndex = x;
      }

      /* Clear out the last widget on the screen list. */
      x = screen->objectCount--;
      screen->object[x]		= 0;
      screen->cdktype[x]	= vNULL;
   }
}

/*
 * This 'brings' a CDK object to the top of the stack.
 */
void raiseCDKObject (EObjectType cdktype, void *object)
{
   CDKOBJS *swapobject;
   int toppos		= -1;
   int swapindex	= -1;
   EObjectType swaptype;

   if (validObjType(cdktype)) {
      CDKOBJS *obj = (CDKOBJS *)object;

      toppos		= (obj)->screen->objectCount;
      swapobject	= (obj)->screen->object[toppos];
      swaptype		= (obj)->screen->cdktype[toppos];
      swapindex		= (obj)->screenIndex;

      (obj)->screenIndex		= toppos;
      (obj)->screen->object[toppos]	= obj;
      (obj)->screen->cdktype[toppos]	= cdktype;

      (obj)->screenIndex		= swapindex;
      (obj)->screen->object[swapindex]	= swapobject;
      (obj)->screen->cdktype[swapindex] = swaptype;
   }
}

/*
 * This 'lowers' an object.
*/
void lowerCDKObject (EObjectType cdktype, void *object)
{
   CDKOBJS *swapobject;
   int toppos		= 0;
   int swapindex	= -1;
   EObjectType swaptype;

   if (validObjType(cdktype)) {
      CDKOBJS *obj = (CDKOBJS *)object;

      swapobject	= (obj)->screen->object[toppos];
      swaptype		= (obj)->screen->cdktype[toppos];
      swapindex		= (obj)->screenIndex;

      (obj)->screenIndex		= toppos;
      (obj)->screen->object[toppos]	= obj;
      (obj)->screen->cdktype[toppos]	= cdktype;

      (obj)->screenIndex		= swapindex;
      (obj)->screen->object[swapindex]	= swapobject;
      (obj)->screen->cdktype[swapindex] = swaptype;
   }
}

/*
 * This calls refreshCDKScreen. (made consistent with widgets)
 */
void drawCDKScreen (CDKSCREEN *cdkscreen)
{
    refreshCDKScreen (cdkscreen);
}

/*
 * This refreshes all the objects in the screen.
 */
void refreshCDKScreen (CDKSCREEN *cdkscreen)
{
   int objectCount = cdkscreen->objectCount;
   int x;

   /* Refresh the screen. */
   wnoutrefresh (cdkscreen->window);

   /* We just call the drawObject function. */
   for (x=0; x < objectCount; x++)
   {
      CDKOBJS *obj = cdkscreen->object[x];

      if (validObjType (cdkscreen->cdktype[x]))
	 obj->fn->drawObj(obj, obj->box);
   }
}

/*
 * This clears all the objects in the screen.
 */
void eraseCDKScreen (CDKSCREEN *cdkscreen)
{
   int objectCount = cdkscreen->objectCount;
   int x;

   /* Refresh the screen. */
   wnoutrefresh (cdkscreen->window);

   /* We just call the drawObject function. */
   for (x=0; x < objectCount; x++)
   {
      CDKOBJS *obj = cdkscreen->object[x];

      if (validObjType (cdkscreen->cdktype[x]))
	 obj->fn->eraseObj(obj);
   }
}

/*
 * This destroys a CDK screen.
 */
void destroyCDKScreen (CDKSCREEN *screen)
{
   free (screen);
}

/*
 * This is added to remain consistent.
 */
void endCDK(void)
{
   /* Turn echoing back on. */
   echo();

   /* Turn off cbreak. */
   nocbreak();

   /* End the curses windows. */
   endwin();

#ifdef HAVE_XCURSES
   XCursesExit();
#endif
}
