#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="fselect_ex";
#endif

void
doView (CDKSCREEN *cdkscreen, char *filename)
{
   CDKVIEWER *example	= 0;
   char *info[MAX_LINES];
   char *button[5], vTitle[256], *mesg[4], temp[256];
   int selected, lines;

   /* Create the viewer buttons. */
   button[0]	= "</5><OK><!5>";
   button[1]	= "</5><Cancel><!5>";

   /* Create the file viewer to view the file selected.*/
   example = newCDKViewer (cdkscreen, CENTER, CENTER, -2, -2,
				button, 2, A_REVERSE, TRUE, TRUE);

   /* Could we create the viewer widget? */
   if (example == 0)
   {
      endCDK();
      printf ("Oops. Can't seem to create viewer. Is the window too small?\n");
      exit (1);
   }

   /* Open the file and read the contents. */
   lines = readFile (filename, info, MAX_LINES);
   if (lines == -1)
   {
      endCDK();
      printf ("Could not open %s\n", filename);
      exit (1);
   }

   /* Set up the viewer title, and the contents to the widget. */
   sprintf (vTitle, "<C></B/21>Filename:<!21></22>%20s<!22!B>", filename);
   setCDKViewer (example, vTitle, info, lines, A_REVERSE, TRUE, TRUE, TRUE);

   /* Clean up. */
   freeCharList (info, lines);

   /* Activate the viewer widget. */
   selected = activateCDKViewer (example, 0);

   /* Check how the person exited from the widget.*/
   if (example->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>Escape hit. No Button selected.";
      mesg[1] = "";
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (example->exitType == vNORMAL)
   {
      sprintf (temp, "<C>You selected button %d", selected);
      mesg[0] = copyChar (temp);
      mesg[1] = "";
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }

   /* Clean up. */
   destroyCDKViewer (example);
}

/*
 * This program demonstrates the file selector and the viewer widget.
 */
int main (int argc, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen = 0;
   CDKFSELECT *fSelect	= 0;
   WINDOW *cursesWin	= 0;
   char *title		= "<C>Pick a file.";
   char *label		= "File: ";
   char *directory	= ".";
   char *filename, *mesg[4];
   int ret;

   /* Parse up the command line. */
   filename = 0;
   while (1)
   {
      ret = getopt (argc, argv, "d:f:");
      if (ret == -1)
      {
	 break;
      }
      switch (ret)
      {
	 case 'd' :
	      directory = strdup (optarg);
	      break;

	 case 'f' :
	      filename = strdup (optarg);
	      break;
      }
   }

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start color. */
   initCDKColor();

   if (filename == 0)
   {
      /* Get the filename. */
      fSelect = newCDKFselect (cdkscreen, CENTER, CENTER, 20, 65,
				title, label, A_NORMAL, '_', A_REVERSE,
				"</5>", "</48>", "</N>", "</N>", TRUE, TRUE);

      /*
       * Set the starting directory. This is not neccessary because when
       * the file selector starts it uses the present directory as a default.
       */
      setCDKFselect (fSelect, directory, A_NORMAL, '.', A_REVERSE,
			"</5>", "</48>", "</N>", "</N>", ObjOf(fSelect)->box);

      for (;;)
      {
	 /* Activate the file selector. */
	 filename = activateCDKFselect (fSelect, 0);

	 /* Check how the person exited from the widget. */
	 if (fSelect->exitType == vESCAPE_HIT)
	 {
	    /* Pop up a message for the user. */
	    mesg[0] = "<C>Escape hit. No file selected.";
	    mesg[1] = "";
	    mesg[2] = "<C>Press any key to continue.";
	    popupLabel (cdkscreen, mesg, 3);

	    /* Exit CDK. */
	    destroyCDKFselect (fSelect);
	    break;
	 }

	 doView (cdkscreen, filename);
	 touchwin (fSelect->entryField->win);
	 touchwin (fSelect->scrollField->win);
      }
   }
   else
   {
      doView (cdkscreen, filename);
   }

   /* Clean up. */
   destroyCDKScreen (cdkscreen);
   endCDK();
   exit (0);
}
