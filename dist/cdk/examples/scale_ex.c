#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="scale_ex";
#endif

/*
 * This program demonstrates the Cdk scale widget.
 */
int main (int argc, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKSCALE *scale	= 0;
   WINDOW *cursesWin	= 0;
   char *title		= "<C>Select a value\n<R>scale\n<L>scale";
   char *label		= "</5>Current value";
   int low		= 0;
   int high		= 100;
   int inc		= 1;
   char temp[256], *mesg[5];
   int selection, ret;

   /* Parse up the command line.*/
   while (1)
   {
      ret = getopt (argc, argv, "l:h:i:");

      /* Are there any more command line options to parse. */
      if (ret == -1)
      {
	 break;
      }

      switch (ret)
      {
	 case 'l':
	      low = atoi (optarg);
	      break;

	 case 'h':
	      high = atoi (optarg);
	      break;

	 case 'i':
	      inc = atoi (optarg);
	      break;
      }
   }

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Create the scale. */
   scale = newCDKScale (cdkscreen, CENTER, CENTER,
			title, label, A_NORMAL,
			4, low, low, high,
			inc, (inc*2), TRUE, TRUE);

   /* Is the scale null? */
   if (scale == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message. */
      printf ("Oops. Can't make the scale widget. Is the window too small?\n");
      exit (1);
   }

   /* Activate the scale. */
   selection = activateCDKScale (scale, 0);

   /* Check the exit value of the scale widget. */
   if (scale->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No value selected.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (scale->exitType == vNORMAL)
   {
      sprintf (temp, "<C>You selected %d", selection);
      mesg[0] = copyChar (temp);
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
      freeChar (mesg[0]);
   }

   /* Clean up. */
   destroyCDKScale (scale);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
