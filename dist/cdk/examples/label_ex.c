#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="label_ex";
#endif

int main (void)
{
   /* Declare variables. */
   CDKSCREEN	*cdkscreen;
   CDKLABEL	*demo;
   WINDOW	*cursesWin;
   char		*mesg[10];

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Set the labels up. */
   mesg[0] = "</29/B>This line should have a yellow foreground and a blue background.";
   mesg[1] = "</5/B>This line should have a white  foreground and a blue background.";
   mesg[2] = "</26/B>This line should have a yellow foreground and a red  background.";
   mesg[3] = "<R>This is a label box.";
   mesg[4] = "<L>This is a label box.";
   mesg[5] = "<C>This line should be set to whatever the screen default is.";

   /* Declare the labels. */
   demo = newCDKLabel (cdkscreen, CENTER, CENTER, mesg, 6, TRUE, TRUE);

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
