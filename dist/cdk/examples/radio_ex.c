#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="radio_ex";
#endif

/*
 * This program demonstrates the Cdk radio widget.
 */
int main (void)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKRADIO *radio	= 0;
   WINDOW *cursesWin	= 0;
   char *title		= "<C></5>Select a filename\n<R>radio\n<L>radio";
   char *item[200], *mesg[5], temp[100];
   int selection, count;

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Set up CDK Colors. */
   initCDKColor();

   /* Use the current diretory list to fill the radio list. */
   count = getDirectoryContents (".", item, 200);

   /* Create the radio list. */
   radio = newCDKRadio (cdkscreen, CENTER, CENTER, RIGHT,
			10, 15, title, item, count,
			'#'|A_REVERSE, 2,
			A_REVERSE, TRUE, TRUE);

   /* Check if the radio list is null. */
   if (radio == 0)
   {
      /* Exit CDK. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message and exit. */
      printf ("Oops. Can't seem to create the radio widget. Is the window too small??\n");
      exit (1);
   }

   /* Activate the radio list. */
   selection = activateCDKRadio (radio, 0);

   /* Check the exit status of the widget. */
   if (radio->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No item selected.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (radio->exitType == vNORMAL)
   {
      mesg[0] = "<C>You selected the filename";
      sprintf (temp, "<C>%s", item[selection]);
      mesg[1] = copyChar (temp);
      mesg[2] = "";
      mesg[3] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 4);
      freeChar (mesg[1]);
   }

   /* Clean up. */
   freeCharList (item, count);
   destroyCDKRadio (radio);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
