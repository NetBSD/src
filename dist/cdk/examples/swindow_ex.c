#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="swindow_ex";
#endif

int main (void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKSWINDOW *swindow	= 0;
   WINDOW *cursesWin	= 0;
   char *title		= "<C></5>Error Log";
   char *mesg[5];

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK colors. */
   initCDKColor();

   /* Create the scrolling window. */
   swindow = newCDKSwindow (cdkscreen, CENTER, CENTER, 9, 65,
				title, 100, TRUE, TRUE);

   /* Is the window null. */
   if (swindow == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message and exit. */
      printf ("Oops. Can not seem to create the scrolling window. Is the window too small??\n");
      exit (1);
   }

   /* Draw the scrolling window. */
   drawCDKSwindow (swindow, ObjOf(swindow)->box);

   /* Load up the scrolling window. */
   addCDKSwindow (swindow, "<C></11>TOP: This is the first line.", BOTTOM);
   addCDKSwindow (swindow, "<C>Sleeping for 1 second.", BOTTOM);
   wrefresh (swindow->win);
   sleep (1);

   addCDKSwindow (swindow, "<L></11>1: This is another line.", BOTTOM);
   addCDKSwindow (swindow, "<C>Sleeping for 1 second.", BOTTOM);
   wrefresh (swindow->win);
   sleep (1);

   addCDKSwindow (swindow, "<C></11>2: This is another line.", BOTTOM);
   addCDKSwindow (swindow, "<C>Sleeping for 1 second.", BOTTOM);
   wrefresh (swindow->win);
   sleep (1);

   addCDKSwindow (swindow, "<R></11>3: This is another line.", BOTTOM);
   addCDKSwindow (swindow, "<C>Sleeping for 1 second.", BOTTOM);
   wrefresh (swindow->win);
   sleep (1);

   addCDKSwindow (swindow, "<C></11>4: This is another line.", BOTTOM);
   addCDKSwindow (swindow, "<C>Sleeping for 1 second.", BOTTOM);
   wrefresh (swindow->win);
   sleep (1);

   addCDKSwindow (swindow, "<L></11>5: This is another line.", BOTTOM);
   addCDKSwindow (swindow, "<C>Sleeping for 1 second.", BOTTOM);
   wrefresh (swindow->win);
   sleep (1);

   addCDKSwindow (swindow, "<C></11>6: This is another line.", BOTTOM);
   addCDKSwindow (swindow, "<C>Sleeping for 1 second.", BOTTOM);
   wrefresh (swindow->win);
   sleep (1);

   addCDKSwindow (swindow, "<C>Done. You can now play.", BOTTOM);

   addCDKSwindow (swindow, "<C>This is being added to the top.", TOP);

   /* Activate the scrolling window. */
   activateCDKSwindow (swindow, 0);

   /* Check how the user exited this widget. */
   if (swindow->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape to leave this widget.";
      mesg[1] = "";
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (swindow->exitType == vNORMAL)
   {
      mesg[0] = "<C>You hit return to exit this widget.";
      mesg[1] = "";
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }

   /* Clean up. */
   destroyCDKSwindow (swindow);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
