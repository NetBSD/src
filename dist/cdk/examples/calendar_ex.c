#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="calendar_ex";
#endif

static BINDFN_PROTO(createCalendarMarkCB);
static BINDFN_PROTO(removeCalendarMarkCB);

/*
 * This program demonstrates the Cdk calendar widget.
 */
int main (int argc, char **argv)
{
   /* Declare variables. */
   CDKSCREEN *cdkscreen		= 0;
   CDKCALENDAR *calendar	= 0;
   WINDOW *cursesWin		= 0;
   char *title			= "<C></U>CDK Calendar Widget\n<C>Demo";
   int day, month, year, ret;
   char *mesg[5], temp[256];
   struct tm *dateInfo;
   time_t clck, retVal;

   /*
    * Get the current dates and set the default values for
    * the day/month/year values for the calendar.
    */
    time (&clck);
    dateInfo	= localtime (&clck);
    day		= dateInfo->tm_mday;
    month	= dateInfo->tm_mon + 1;
    year	= dateInfo->tm_year + 1900;

   /* Check the command line for options. */
   while (1)
   {
      ret = getopt (argc, argv, "d:m:y:t:");

      /* Are there any more command line options to parse. */
      if (ret == -1)
      {
	 break;
      }

      switch (ret)
      {
	 case 'd':
	      day = atoi (optarg);
	      break;

	 case 'm':
	      month = atoi (optarg);
	      break;

	 case 'y':
	      year = atoi (optarg);
	      break;

	 case 't':
	      title = copyChar (optarg);
	      break;
      }
   }

   /* Set up CDK. */
   cursesWin = initscr();
   cdkscreen = initCDKScreen (cursesWin);

   /* Start CDK Colors. */
   initCDKColor();

   /* Create the calendar widget. */
   calendar = newCDKCalendar (cdkscreen, CENTER, CENTER,
				title, day, month, year,
				COLOR_PAIR(16)|A_BOLD,
				COLOR_PAIR(24)|A_BOLD,
				COLOR_PAIR(32)|A_BOLD,
				COLOR_PAIR(40)|A_REVERSE,
				TRUE, TRUE);

   /* Is the widget null? */
   if (calendar == 0)
   {
      /* Clean up the memory. */
      destroyCDKScreen (cdkscreen);

      /* End curses... */
      endCDK();

      /* Spit out a message. */
      printf ("Oops. Can't seem to create the calendar. Is the window too small?\n");
      exit (1);
   }

   /* Create a key binding to mark days on the calendar. */
   bindCDKObject (vCALENDAR, calendar, 'm', createCalendarMarkCB, calendar);
   bindCDKObject (vCALENDAR, calendar, 'M', createCalendarMarkCB, calendar);
   bindCDKObject (vCALENDAR, calendar, 'r', removeCalendarMarkCB, calendar);
   bindCDKObject (vCALENDAR, calendar, 'R', removeCalendarMarkCB, calendar);

   /* Draw the calendar widget. */
   drawCDKCalendar (calendar, ObjOf(calendar)->box);

   /* Let the user play with the widget. */
   retVal = activateCDKCalendar (calendar, 0);

   /* Check which day they selected. */
   if (calendar->exitType == vESCAPE_HIT)
   {
      mesg[0] = "<C>You hit escape. No date selected.";
      mesg[1] = "",
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
   }
   else if (calendar->exitType == vNORMAL)
   {
      mesg[0] = "You selected the following date";
      sprintf (temp, "<C></B/16>%02d/%02d/%d (dd/mm/yyyy)", calendar->day, calendar->month, calendar->year);
      mesg[1] = copyChar (temp);
      mesg[2] = "<C>Press any key to continue.";
      popupLabel (cdkscreen, mesg, 3);
      freeChar (mesg[1]);
   }

   /* Clean up and exit. */
   destroyCDKCalendar (calendar);
   destroyCDKScreen (cdkscreen);
   delwin (cursesWin);
   endCDK();
   fflush (stdout);
   printf ("Selected Time: %s\n", ctime(&retVal));
   exit (0);
}

/*
 * This adds a marker to the calendar.
 */
static void createCalendarMarkCB (EObjectType objectType GCC_UNUSED, void *object, void *clientData GCC_UNUSED, chtype key GCC_UNUSED)
{
   CDKCALENDAR *calendar = (CDKCALENDAR *)object;

   setCDKCalendarMarker (calendar,
				calendar->day,
				calendar->month,
				calendar->year,
				COLOR_PAIR (5) | A_REVERSE);

   drawCDKCalendar (calendar, ObjOf(calendar)->box);
}

/*
 * This removes a marker from the calendar.
 */
static void removeCalendarMarkCB (EObjectType objectType GCC_UNUSED, void *object, void *clientData GCC_UNUSED, chtype key GCC_UNUSED)
{
   CDKCALENDAR *calendar = (CDKCALENDAR *)object;

   removeCDKCalendarMarker (calendar,
				calendar->day,
				calendar->month,
				calendar->year);

   drawCDKCalendar (calendar, ObjOf(calendar)->box);
}
