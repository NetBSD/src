#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="histogram_ex";
#endif

int main (void)
{
   /* Declare vars. */
   CDKSCREEN *cdkscreen = 0;
   CDKHISTOGRAM *volume = 0;
   CDKHISTOGRAM *bass	= 0;
   CDKHISTOGRAM *treble = 0;
   WINDOW *cursesWin	= 0;
   char *volumeTitle	= "<C></5>Volume<!5>";
   char *bassTitle	= "<C></5>Bass  <!5>";
   char *trebleTitle	= "<C></5>Treble<!5>";

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Color. */
   initCDKColor();

   /* Create the histogram objects. */
   volume = newCDKHistogram (cdkscreen, 1, 8, 1, -2,
				HORIZONTAL, volumeTitle,
				TRUE, TRUE);
   if (volume == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message and exit. */
      printf ("Oops. Can not make volume histogram. Is the window big enough??\n");
      exit (1);
   }

   bass = newCDKHistogram (cdkscreen, 1, 13, 1, -2,
				HORIZONTAL, bassTitle,
				TRUE, TRUE);
   if (bass == 0)
   {
      /* Exit CDK. */
      destroyCDKHistogram (volume);
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message and exit. */
      printf ("Oops. Can not make bass histogram. Is the window big enough??\n");
      exit (1);
   }

   treble = newCDKHistogram (cdkscreen, 1, 18, 1, -2,
				HORIZONTAL, trebleTitle,
				TRUE, TRUE);
   if (treble == 0)
   {
      /* Exit CDK. */
      destroyCDKHistogram (volume);
      destroyCDKHistogram (bass);
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message and exit. */
      printf ("Oops. Can not make treble histogram. Is the window big enough??\n");
      exit (1);
   }

   /* Set the histogram values. */
   setCDKHistogram (volume, vPERCENT, CENTER, A_BOLD, 0, 10, 6, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (bass  , vPERCENT, CENTER, A_BOLD, 0, 10, 3, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (treble, vPERCENT, CENTER, A_BOLD, 0, 10, 7, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   refreshCDKScreen (cdkscreen);
   wrefresh (volume->win);
   sleep (4);

   /* Set the histogram values. */
   setCDKHistogram (volume, vPERCENT, CENTER, A_BOLD, 0, 10, 8, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (bass  , vPERCENT, CENTER, A_BOLD, 0, 10, 1, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (treble, vPERCENT, CENTER, A_BOLD, 0, 10, 9, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   refreshCDKScreen (cdkscreen);
   wrefresh (volume->win);
   sleep (4);

   /* Set the histogram values. */
   setCDKHistogram (volume, vPERCENT, CENTER, A_BOLD, 0, 10, 10, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (bass  , vPERCENT, CENTER, A_BOLD, 0, 10, 7,	 ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (treble, vPERCENT, CENTER, A_BOLD, 0, 10, 10, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   refreshCDKScreen (cdkscreen);
   wrefresh (volume->win);
   sleep (4);

   /* Set the histogram values. */
   setCDKHistogram (volume, vPERCENT, CENTER, A_BOLD, 0, 10, 1, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (bass  , vPERCENT, CENTER, A_BOLD, 0, 10, 8, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (treble, vPERCENT, CENTER, A_BOLD, 0, 10, 3, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   refreshCDKScreen (cdkscreen);
   wrefresh (volume->win);
   sleep (4);

   /* Set the histogram values. */
   setCDKHistogram (volume, vPERCENT, CENTER, A_BOLD, 0, 10, 3, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (bass  , vPERCENT, CENTER, A_BOLD, 0, 10, 3, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (treble, vPERCENT, CENTER, A_BOLD, 0, 10, 3, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   refreshCDKScreen (cdkscreen);
   wrefresh (volume->win);
   sleep (4);

   /* Set the histogram values. */
   setCDKHistogram (volume, vPERCENT, CENTER, A_BOLD, 0, 10, 10, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (bass  , vPERCENT, CENTER, A_BOLD, 0, 10, 10, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   setCDKHistogram (treble, vPERCENT, CENTER, A_BOLD, 0, 10, 10, ' '|A_REVERSE|COLOR_PAIR(3), TRUE);
   refreshCDKScreen (cdkscreen);
   wrefresh (volume->win);
   sleep (4);

   /* Clean up. */
   destroyCDKHistogram (volume);
   destroyCDKHistogram (bass);
   destroyCDKHistogram (treble);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
