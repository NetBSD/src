#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="serial";
#endif

/*
 * Create global definitions.
 */
#define DEFAULT_PORT			"/dev/ttya"
#define DEFAULT_POLL_INTERVAL		1	/* milliseconds */

/*
 * This is the working function which probes the serial port.
 */
boolean probeModem (void);

/*
 * Define some global variables.
 */
CDKLABEL *label		= 0;
int LLastState		= 0;
int LCurrentState	= 0;
extern char *optarg;
char port[256];
int LFD;

/*
 *
 */
int main (int argc, char **argv)
{
   CDKSCREEN *cdkScreen = 0;
   WINDOW *cursesWin	= 0;
   int lines		= 0;
   char *info[256], temp[256];
   struct termios termInfo;
   int ret;

   /* Set the deault values. */
   strcpy (port, DEFAULT_PORT);

   /* Parse up the command line. */
   while (1)
   {
      if ((ret = getopt (argc, argv, "p:h")) == -1)
      {
	 break;
      }

      switch (ret)
      {
	 case 'p' :
	       strcpy (port, optarg);
	       break;

	 case 'h' :
	       printf ("Usage: %s [-p Port] [-i Poll Interval] [-c Poll Count] [-v] [-h]\n", argv[0]);
	       exit (0);
	       break;
      }
   }

  /*
   * Create the CDK screen.
   */
   cursesWin = initscr();
   cdkScreen = initCDKScreen (cursesWin);

   /* Start CDK color. */
   initCDKColor();

  /*
   * Set the title of the main window.
   */
   sprintf (temp, "<C>Serial Port Monitor (%s)", port);
   info[lines++] = copyChar (temp);
   info[lines++] = copyChar ("<C><#HL(30)>");
   info[lines++] = copyChar ("");
   info[lines++] = copyChar ("Line Enabled       : -");
   info[lines++] = copyChar ("Data Terminal Ready: -");
   info[lines++] = copyChar ("Carrier Detect     : -");
   info[lines++] = copyChar ("Request To Send    : -");
   info[lines++] = copyChar ("Clear To Send      : -");
   info[lines++] = copyChar ("Secondary Transmit : -");
   info[lines++] = copyChar ("Secondary Receive  : -");
   info[lines++] = copyChar ("");

   /* Create the label widget. */
   label = newCDKLabel (cdkScreen, CENTER, CENTER, info, lines, TRUE, FALSE);
   drawCDKLabel (label, TRUE);

  /*
   * Open the serial port read only.
   */
   if ((LFD = open (port, O_RDONLY|O_NDELAY, 0)) == -1)
   {
      /* Create a pop-up dialog box... */
      printf ("Error: Open of <%s> failed.\n", port);
      exit (1);
   }

   termInfo.c_cflag = CRTSCTS | CLOCAL;
   if (tcgetattr (LFD, &termInfo) != 0)
   {
      /* Really should create a pop-up dialog box... */
      printf ("Error: Could not get port attributes. Closing the port.\n");
      close (LFD);
      exit (1);
   }

   for (;;)
   {
      /* Probe the modem. */
      probeModem();

     /*
      * Sleep for the given amount of time. We do this first so no
      * weird refresh things happen.
      */
      napms (DEFAULT_POLL_INTERVAL);
   }
}

/*
 * This probes the modem and determines if we need to update
 * the display.
 */
boolean probeModem (void)
{
   int lines		= 0;
   char *info[256], temp[256];

   /* Start building the label. */
   sprintf (temp, "<C>Serial Port Monitor (%s)", port);
   info[lines++] = copyChar (temp);
   info[lines++] = copyChar ("<C><#HL(30)>");
   info[lines++] = copyChar ("");

  /*
   * Get the serial port info.
   */
   ioctl (LFD, TIOCMGET, &LCurrentState);

  /*
   * If the states are different, change the display.
   */
   if (LLastState != LCurrentState)
   {
     /*
      * Check for line enabled.
      */
      if (LCurrentState & TIOCM_LE)
      {
	 info[lines++] = copyChar ("Line Enabled       : <#DI>");
      }
      else
      {
	 info[lines++] = copyChar ("Line Enabled       :  ");
      }

     /*
      * Check for data terminal ready.
      */
      if (LCurrentState & TIOCM_DTR)
      {
	 info[lines++] = copyChar ("Data Terminal Ready: <#DI>");
      }
      else
      {
	 info[lines++] = copyChar ("Data Terminal Ready:  ");
      }

     /*
      * Check for carrier detect.
      */
      if (LCurrentState & TIOCM_CAR)
      {
	 info[lines++] = copyChar ("Carrier Detect     : <#DI>");
      }
      else
      {
	 info[lines++] = copyChar ("Carrier Detect     :  ");
      }

     /*
      * Check for request to send.
      */
      if (LCurrentState & TIOCM_RTS)
      {
	 info[lines++] = copyChar ("Request To Send    : <#DI>");
      }
      else
      {
	 info[lines++] = copyChar ("Request To Send    :  ");
      }

     /*
      * Check for clear to send.
      */
      if (LCurrentState & TIOCM_CTS)
      {
	 info[lines++] = copyChar ("Clear To Send      : <#DI>");
      }
      else
      {
	 info[lines++] = copyChar ("Clear To Send      :  ");
      }

     /*
      * Check for secondary transmit.
      */
      if (LCurrentState & TIOCM_ST)
      {
	 info[lines++] = copyChar ("Secondary Transmit : <#DI>");
      }
      else
      {
	 info[lines++] = copyChar ("Secondary Transmit :  ");
      }

     /*
      * Check for secondary receive.
      */
      if (LCurrentState & TIOCM_SR)
      {
	 info[lines++] = copyChar ("Secondary Receive  : <#DI>");
      }
      else
      {
	 info[lines++] = copyChar ("Secondary Receive  :  ");
      }
   }
   info[lines++] = copyChar ("");

   /* Only do this if things have changed. */
   if (LLastState != LCurrentState)
   {
      eraseCDKLabel (label);
      setCDKLabel (label, info, lines, TRUE);
      drawCDKLabel (label, TRUE);
   }

  /*
   * Keep the current state.
   */
   LLastState = LCurrentState;

  /*
   * Return False to tell X that we want this funtion to be
   * run again.
   */
   return FALSE;
}
