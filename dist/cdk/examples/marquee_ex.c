#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="marquee_ex";
#endif

void help (char *programName);

int main (int argc, char **argv)
{
   /* Declare vars. */
   CDKSCREEN	*cdkscreen;
   CDKMARQUEE	*scrollMessage;
   WINDOW	*cursesWin;
   char		message[1024];
   char		startAttr[10];
   char		endAttr[10];
   char		*currentTime;
   char		*mesg;
   time_t	clck;
   int delay, repeat, ret, tmp;

   /* Set some variables. */
   repeat	= 3;
   delay	= 5;
   mesg		= 0;

   /* Clean up the strings. */
   cleanChar (message, sizeof(message), '\0');
   cleanChar (startAttr, sizeof(startAttr), '\0');
   cleanChar (endAttr, sizeof(endAttr), '\0');

   /* Check the command line for options. */
   while (1)
   {
      ret = getopt (argc, argv, "brkud:R:m:h");

      /* Are there any more command line options to parse. */
      if (ret == -1)
      {
	 break;
      }

      switch (ret)
      {
	 case 'b':
	      if (startAttr[0] == '\0')
	      {
		 startAttr[0] = '<';
		 endAttr[0] = '<';
	      }
	      strcat (startAttr, "/B");
	      strcat (endAttr, "!B");
	      break;

	 case 'r':
	      if (startAttr[0] == '\0')
	      {
		 startAttr[0] = '<';
		 endAttr[0] = '<';
	      }
	      strcat (startAttr, "/R");
	      strcat (endAttr, "!R");
	      break;

	 case 'k':
	      if (startAttr[0] == '\0')
	      {
		 startAttr[0] = '<';
		 endAttr[0] = '<';
	      }
	      strcat (startAttr, "/K");
	      strcat (endAttr, "!K");
	      break;

	 case 'u':
	      if (startAttr[0] == '\0')
	      {
		 startAttr[0] = '<';
		 endAttr[0] = '<';
	      }
	      strcat (startAttr, "/U");
	      strcat (endAttr, "!U");
	      break;

	 case 'R':
	      repeat = atoi (optarg);
	      break;

	 case 'd':
	      tmp = atoi (optarg);
	      if (tmp > 0)
	      {
		 delay = tmp;
	      }
	      break;

	 case 'm':
	      mesg = copyChar (optarg);
	      break;

	 case 'h':
	      help (argv[0]);
	      freeChar (mesg);
	      exit (0);
	      break;
      }
   }

   /* Put the end of the attributes if they asked for then. */
   if (startAttr[0] == '<')
   {
      strcat (startAttr, ">");
      strcat (endAttr, ">");
   }

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Create the marquee. */
   scrollMessage = newCDKMarquee (cdkscreen, CENTER, TOP, 30, FALSE, TRUE);

   /* Check if the marquee is null. */
   if (scrollMessage == 0)
   {
      /* Exit Cdk. */
      destroyCDKScreen (cdkscreen);
      endCDK();

      /* Print out a message. */
      printf ("Oops. Can't seem to create the marquee window. Is the window too small?\n");
      exit (1);
   }

   /* Draw the CDK screen. */
   refreshCDKScreen (cdkscreen);

   /* Create the marquee message. */
   if (mesg == 0)
   {
      /* Get the current time. */
      time (&clck);
      currentTime = ctime (&clck);

      currentTime[strlen(currentTime)-1] = '\0';
      sprintf (message, "%s%s%s (This Space For Rent) ", startAttr, currentTime, endAttr);
   }
   else
   {
      sprintf (message, "%s%s%s ", startAttr, mesg, endAttr);
   }

   /* Run the marquee. */
   activateCDKMarquee (scrollMessage, message, delay, repeat, TRUE);

   /* Clean up. */
   destroyCDKMarquee (scrollMessage);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   exit (0);
}

/*
 * This spits out help about this demo program.
 */
void help (char *programName)
{
char *USAGE = "[-m Message] [-R repeat value] [-d delay value] [-b|r|u|k] [-h]";

   printf ("Usage: %s %s\n", programName, USAGE);
   printf ("     -m Message - Sets the message to display in the marquee\n");
   printf ("                  If no message is provided, one will be created.\n");
   printf ("     -R Repeat  - Tells the marquee how many time to repeat the message.\n");
   printf ("                  A value of -1 tells the marquee to repeat the message forever.\n");
   printf ("     -d Delay   - Sets the number of milli seconds to delay beyween repeats.\n");
   printf ("     -b         - Tells the marquee to display the message with the bold attribute.\n");
   printf ("     -r         - Tells the marquee to display the message with a revered attribute.\n");
   printf ("     -u         - Tells the marquee to display the message with an underline attribute.\n");
   printf ("     -k         - Tells the marquee to display the message with the blinking attribute.\n");
}
