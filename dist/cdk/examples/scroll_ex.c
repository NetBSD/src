#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="scroll_ex";
#endif

/*
 * This program demonstrates the Cdk scrolling list widget.
 */
int main (void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen		= 0;
   CDKSCROLL *scrollList	= 0;
   WINDOW *cursesWin		= 0;
   char *title			= "<C></5>Pick a file\n<R>scroll\n<L>scroll";
   char *item[200], *mesg[5], temp[256];
   int selection, count;

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Set up CDK Colors. */
   initCDKColor();

   /* Use the current diretory list to fill the radio list. */
   count = getDirectoryContents (".", item, 200);

   /* Create the scrolling list. */
   scrollList = newCDKScroll (cdkscreen, CENTER, CENTER, LEFT,
				10, 15, title, item, count,
				NONE, A_REVERSE, TRUE, TRUE);

   /* Is the scrolling list null? */
   if (scrollList == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message and exit. */
      printf ("Oops. Could not make scrolling list. Is the window too small?\n");
      exit (1);
   }

#if 0
   drawCDKScroll (scrollList, 1);

   setCDKScrollPosition (scrollList, 10);
   drawCDKScroll (scrollList, 1);
   sleep (3);

   setCDKScrollPosition (scrollList, 20);
   drawCDKScroll (scrollList, 1);
   sleep (3);

   setCDKScrollPosition (scrollList, 30);
   drawCDKScroll (scrollList, 1);
   sleep (3);

   setCDKScrollPosition (scrollList, 70);
   drawCDKScroll (scrollList, 1);
   sleep (3);
#endif

   /* Activate the scrolling list. */
   selection = activateCDKScroll (scrollList, 0);

   /* Determine how the widget was exited. */
   if (scrollList->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No file selected.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (scrollList->exitType == vNORMAL)
   {
      mesg[0] = "<C>You selected the following file";
      sprintf (temp, "<C>%s", item[selection]);
      mesg[1] = copyChar (temp);
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
      freeChar (mesg[1]);
   }

   /* Clean up. */
   destroyCDKScroll (scrollList);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
