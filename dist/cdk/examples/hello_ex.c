#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="hello_ex";
#endif

int main (void)
{
   /* Declare variables. */
   CDKSCREEN	*cdkscreen;
   CDKLABEL	*demo;
   WINDOW	*cursesWin;
   char		*mesg[5];

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Set the labels up. */
   mesg[0] = "                               ";
   mesg[1] = "<C>Hello World!";
   mesg[2] = "<R>Hello World!";
   mesg[3] = "<L>Hello World!";
   mesg[4] = "                               ";

   /* Declare the labels. */
   demo = newCDKLabel (cdkscreen, CENTER, CENTER, mesg, 5, TRUE, TRUE);

   /* Is the label null? */
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

   /* Draw the CDK screen. */
   refreshCDKScreen (cdkscreen);
   waitCDKLabel (demo, ' ');

   /* Clean up. */
   destroyCDKLabel (demo);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}
