#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="label_ex";
#endif

int main (int argc, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKLABEL *demo	= 0;
   WINDOW *cursesWin	= 0;
   int boxLabel		= 0;
   char *mesg[4], temp[256];
   struct tm *currentTime;
   time_t clck;
   int ret;

   /* Parse up the command line. */
   while (1)
   {
      ret = getopt (argc, argv, "b");
      if (ret == -1)
      {
	 break;
      }

      switch (ret)
      {
	 case 'b' :
	      boxLabel = 1;
      }
   }

   /* Set up CDK */ 
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors */
   initCDKColor();

   /* Set the labels up. */
   mesg[0] = "</1/B>HH:MM:SS";

   /* Declare the labels. */
   demo = newCDKLabel (cdkscreen, CENTER, CENTER, mesg, 1, boxLabel, FALSE);

   /* Is the label null??? */
   if (demo == 0)
   {
      /* Clean up the memory. */
      destroyCDKScreen (cdkscreen);

      /* End curses... */
      endCDK();

      /* Spit out a message. */
      printf ("Oops. Can't seem to create the label. Is the window too small?\n");
      exit (1);
   }

   /* Do this for-ever... */
   for (;;)
   {
      /* Get the current time. */
      time(&clck);
      currentTime = localtime (&clck);

      /* Put the current time in a string. */
      sprintf (temp, "<C></B/29>%02d:%02d:%02d", currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec);
      mesg[0] = copyChar (temp);

      /* Set the label contents. */
      setCDKLabel (demo, mesg, 1, ObjOf(demo)->box);

      /* Clean up the memory used. */
      freeChar (mesg[0]);

      /* Draw the label, and sleep. */
      drawCDKLabel (demo, ObjOf(demo)->box);
      wrefresh (demo->win);
      sleep (1);
   }

   /* Clean up */
   destroyCDKLabel (demo);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
