#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="slider_ex";
#endif

/*
 * This program demonstrates the Cdk slider widget.
 */
int main (int argc, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKSLIDER *slider	= 0;
   WINDOW *cursesWin	= 0;
   char *title		= "<C></U>Enter a value:\n<R>This is a slider box.\n<L>This is a slider box.";
   char *label		= "</B>Current Value:";
   int low		= 0;
   int high		= 100;
   int inc		= 1;
   int fieldWidth	= 50;
   char temp[256], *mesg[5];
   int selection, ret;

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Parse up the command line. */
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

   /* Start CDK Colors. */
   initCDKColor();

   /* Create the slider. */
   slider = newCDKSlider (cdkscreen, CENTER, CENTER, title, label,
				A_REVERSE | COLOR_PAIR (29) | ' ', fieldWidth, low,
				low, high, inc, (inc*2), TRUE, TRUE);

   /* Is the slider null? */
   if (slider == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message. */
      printf ("Oops. Can't make the slider widget. Is the window too small?\n");
      exit (1);
   }

   /* Activate the slider. */
   selection = activateCDKSlider (slider, 0);

   /* Check the exit value of the slider widget. */
   if (slider->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No value selected.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (slider->exitType == vNORMAL)
   {
      sprintf (temp, "<C>You selected %d", selection);
      mesg[0] = copyChar (temp);
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
      freeChar (mesg[0]);
   }

   /* Clean up.*/
   destroyCDKSlider (slider);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
