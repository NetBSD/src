#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="subwindow_ex";
#endif

/*
 * This demo displays the ability to put widgets within a curses subwindow.
 */
int main (void)
{
   /* Declare vars. */
   CDKSCREEN *cdkscreen;
   CDKSCROLL *dowList;
   CDKLABEL *title;
   WINDOW *mainWindow, *subWindow;
   char *dow[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
   char *mesg[5];
   int exitType;
   int pick;

   /* Start curses. */
   mainWindow = initscr();

   /* Create a basic window. */
   subWindow = newwin (LINES-5, COLS-10, 2, 5);

   /* Start Cdk. */
   cdkscreen = initCDKScreen (subWindow);

   /* Box our window. */
   box (subWindow, ACS_VLINE, ACS_HLINE);
   wrefresh (subWindow);

   /* Create a basic scrolling list inside the window. */
   dowList = newCDKScroll (cdkscreen, CENTER, CENTER, RIGHT, 17, 7,
			"<C></U>Pick a Day", dow, 7, NONUMBERS,
			A_REVERSE, TRUE, TRUE);

   /* Put a title within the window. */
   mesg[0] = "<C><#HL(30)>";
   mesg[1] = "<C>This is a Cdk scrolling list";
   mesg[2] = "<C>inside a curses window.";
   mesg[3] = "<C><#HL(30)>";
   title = newCDKLabel (cdkscreen, CENTER, 0, mesg, 4, FALSE, FALSE);

   /* Refresh the screen. */
   refreshCDKScreen (cdkscreen);

   /* Let the user play. */
   pick = activateCDKScroll (dowList, 0);
   exitType = dowList->exitType;

   /* Clean up. */
   destroyCDKScroll (dowList);
   destroyCDKLabel (title);
   werase (subWindow);
   wrefresh (subWindow);
   delwin (subWindow);
   endCDK();

   /* Tell them what they picked. */
   if (exitType == vESCAPE_HIT)
   {
      printf ("You pressed escape.\n");
   }
   else
   {
      printf ("You picked %s.\n", dow[pick]);
   }
   exit (0);
}
