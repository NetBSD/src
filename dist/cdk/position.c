#include <cdk.h>

/*
 * $Author: garbled $
 * $Date: 2001/01/04 19:58:00 $
 * $Revision: 1.1.1.1 $
 */

#undef	ObjOf
#define ObjOf(ptr)    (ptr)

/*
 * This allows the user to use the cursor keys to adjust the
 * position of the widget.
 */
void positionCDKObject (CDKOBJS *obj, WINDOW *win)
{
   int origX	= getbegx(win);
   int origY	= getbegy(win);
   chtype key;

   /* Let them move the widget around until they hit return. */
   for (;;)
   {
      wrefresh (win);
      key = wgetch (win);
      switch (key)
      {
	 case KEY_UP :
	 case '8' :
	      if (getbegy(win) > 0)
	      {
	         moveCDKObject (obj, 0, -1, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case KEY_DOWN :
	 case '2' :
	      if (getendy(win) < getmaxy(WindowOf(obj)))
	      {
	         moveCDKObject (obj, 0, 1, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case KEY_LEFT :
	 case '4' :
	      if (getbegx(win) > 0)
	      {
	         moveCDKObject (obj, -1, 0, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case KEY_RIGHT :
	 case '6' :
	      if (getendx(win) < getmaxx(WindowOf(obj)))
	      {
	         moveCDKObject (obj, 1, 0, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case '7' :
	      if (getbegy(win) > 0 && getbegx(win) > 0)
	      {
	         moveCDKObject (obj, -1, -1, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case '9' :
	      if (getendx(win) < getmaxx(WindowOf(obj)) && getbegy(win) > 0)
	      {
	         moveCDKObject (obj, 1, -1, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case '1' :
	      if (getbegx(win) > 0 && getendy(win) < getmaxy(WindowOf(obj)))
	      {
	         moveCDKObject (obj, -1, 1, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case '3' :
	      if (getendx(win) < getmaxx(WindowOf(obj))
	       && getendy(win) < getmaxy(WindowOf(obj)))
	      {
	         moveCDKObject (obj, 1, 1, TRUE, TRUE);
	      }
	      else
	      {
	         Beep();
	      }
	      break;

	 case '5' :
	      moveCDKObject (obj, CENTER, CENTER, FALSE, TRUE);
	      break;

	 case 't' :
	      moveCDKObject (obj, getbegx(win), TOP, FALSE, TRUE);
	      break;

	 case 'b' :
	      moveCDKObject (obj, getbegx(win), BOTTOM, FALSE, TRUE);
	      break;

	 case 'l' :
	      moveCDKObject (obj, LEFT, getbegy(win), FALSE, TRUE);
	      break;

	 case 'r' :
	      moveCDKObject (obj, RIGHT, getbegy(win), FALSE, TRUE);
	      break;

	 case 'c' :
	      moveCDKObject (obj, CENTER, getbegy(win), FALSE, TRUE);
	      break;

	 case 'C' :
	      moveCDKObject (obj, getbegx(win), CENTER, FALSE, TRUE);
	      break;

	 case KEY_ESC :
	      moveCDKObject (obj, origX, origY, FALSE, TRUE);
	      break;

	 case KEY_RETURN : case KEY_ENTER :
	      return;

	 case CDK_REFRESH :
	      eraseCDKScreen (ScreenOf(obj));
	      refreshCDKScreen (ScreenOf(obj));
	      break;

	 default :
	      Beep();
	      break;
      }
   }
}
