#include <cdk.h>

#ifdef HAVE_XCURSES
char *XCursesProgramName="vinstall";
#endif

/*
 * Written by:	Mike Glover
 * Purpose:
 *		This is a fairly basic install interface.
 */

/* Declare global types and prototypes. */
char *FPUsage = "-f filename [-s source directory] [-d destination directory] [-t title] [-o Output file] [-q]";
typedef enum {vCanNotOpenSource,
		vCanNotOpenDest,
		vOK
		} ECopyFile;
ECopyFile copyFile (CDKSCREEN *cdkScreen, char *src, char *dest);
int verifyDirectory (CDKSCREEN *screen, char *directory);

int main (int argc, char **argv)
{
   /* Declare variables. */
   WINDOW	*cursesWin	= 0;
   CDKSCREEN	*cdkScreen	= 0;
   CDKSWINDOW	*installOutput	= 0;
   CDKENTRY	*sourceEntry	= 0;
   CDKENTRY	*destEntry	= 0;
   CDKLABEL	*titleWin	= 0;
   CDKHISTOGRAM *progressBar	= 0;
   char		*sourcePath	= 0;
   char		*destPath	= 0;
   char		*sourceDir	= 0;
   char		*destDir	= 0;
   char		*filename	= 0;
   char		*title		= 0;
   char		*output		= 0;
   int		quiet		= FALSE;
   int		errors		= 0;
   int		sWindowHeight	= 0;
   char		*titleMessage[10], *fileList[2000], *mesg[20];
   char		oldPath[512], newPath[512], temp[2000];
   char		**files;
   int count, chunks, ret, x;

   /* Parse up the command line. */
   while (1)
   {
      ret = getopt (argc, argv, "d:s:f:t:o:q");
      if (ret == -1)
      {
	 break;
      }
      switch (ret)
      {
	 case 's' :
	      sourcePath = strdup (optarg);
	      break;

	 case 'd' :
	      destPath = strdup (optarg);
	      break;

	 case 'f' :
	      filename = strdup (optarg);
	      break;

	 case 't' :
	      title = strdup (optarg);
	      break;

	 case 'o' :
	      output = strdup (optarg);
	      break;

	 case 'q' :
	      quiet = TRUE;
	      break;
      }
   }

   /* Make sure have everything we need. */
   if (filename == 0)
   {
      fprintf (stderr, "Usage: %s %s\n", argv[0], FPUsage);
      exit (-1);
   }

   /* Open the file list file and read it in. */
   count = readFile (filename, fileList, 2000);
   if (count == 0)
   {
      fprintf (stderr, "%s: Input filename <%s> is empty.\n", argv[0], filename);
      exit (-1);
   }

  /*
   * Cycle through what was given to us and save it.
   */
   for (x=0; x < count; x++)
   {
      /* Strip white space from the line. */
      stripWhiteSpace (vBOTH, fileList[x]);
   }

   /* Set up CDK. */ 
   cursesWin = initscr();
   cdkScreen = initCDKScreen (cursesWin);

   /* Start color. */
   initCDKColor();
   
   /* Create the title label. */
   titleMessage[0] = "<C></32/B><#HL(30)>";
   if (title == 0)
   {
      sprintf (temp, "<C></32/B>CDK Installer");
   }
   else
   {
      sprintf (temp, "<C></32/B>%s", title);
   }
   titleMessage[1] = copyChar (temp);
   titleMessage[2] = "<C></32/B><#HL(30)>";
   titleWin = newCDKLabel (cdkScreen, CENTER, TOP, 
				titleMessage, 3, FALSE, FALSE);
   freeChar (titleMessage[1]);

   /* Allow them to change the install directory. */
   if (sourcePath == 0)
   {
      sourceEntry = newCDKEntry (cdkScreen, CENTER, 8,
					0, "Source Directory:      ",
					A_NORMAL, '.', vMIXED, 
					40, 0, 256, TRUE, FALSE);
   }
   if (destPath == 0)
   {
      destEntry = newCDKEntry (cdkScreen, CENTER, 11,
				0, "Destination Directory: ", A_NORMAL, 
				'.', vMIXED, 40, 0, 256, TRUE, FALSE);
   }

   /* Get the source install path. */
   if (sourceEntry != 0)
   {
      drawCDKScreen (cdkScreen);
      sourceDir = copyChar (activateCDKEntry (sourceEntry, 0));
   }
   else
   {
      sourceDir = copyChar (sourcePath);
   }

   /* Get the destination install path. */
   if (destEntry != 0)
   {
      drawCDKScreen (cdkScreen);
      destDir = copyChar (activateCDKEntry (destEntry, 0));
   }
   else
   {
      destDir = copyChar (destPath);
   }

   /* Destroy the path entry fields. */
   if (sourceEntry != 0)
   {
      destroyCDKEntry (sourceEntry);
   }
   if (destEntry != 0)
   {
      destroyCDKEntry (destEntry);
   }

  /*
   * Verify that the source directory is valid.
   */
   if (verifyDirectory (cdkScreen, sourceDir) != 0)
   {
      /* Clean up and leave. */
      freeChar (destDir);
      freeChar (sourceDir);
      destroyCDKLabel (titleWin);
      destroyCDKScreen (cdkScreen);
      endCDK();

      /* Clean up the file list information. */
      for (x=0; x < count; x++)
      {
	 freeChar (fileList[x]);
      }
      exit (-1);
   }

  /*
   * Verify that the source directory is valid.
   */
   if (verifyDirectory (cdkScreen, destDir) != 0)
   {
      /* Clean up and leave. */
      freeChar (destDir);
      freeChar (sourceDir);
      destroyCDKLabel (titleWin);
      destroyCDKScreen (cdkScreen);
      endCDK();

      /* Clean up the file list information */
      for (x=0; x < count; x++)
      {
	 freeChar (fileList[x]);
      }
      exit (-2);
   }

   /* Create the histogram. */
   progressBar = newCDKHistogram (cdkScreen, CENTER, 5,
					1, 0, HORIZONTAL,
					"<C></56/B>Install Progress",
					TRUE, FALSE);

   /* Set the top left/right characters of the histogram.*/
   setCDKHistogramLLChar (progressBar, ACS_LTEE);
   setCDKHistogramLRChar (progressBar, ACS_RTEE);

   /* Set the initial value of the histogram. */
   setCDKHistogram (progressBar, vPERCENT, TOP, A_BOLD,
			1, count, 1,
			COLOR_PAIR (24) | A_REVERSE | ' ',
			TRUE);

   /* Determine the height of the scrolling window. */
   if (LINES >= 16)
   {
      sWindowHeight = LINES - 8;
   }
   else
   {
      sWindowHeight = 3;
   }
   
   /* Create the scrolling window. */
   installOutput = newCDKSwindow (cdkScreen, CENTER, BOTTOM,
					sWindowHeight, 0,
					"<C></56/B>Install Results",
					2000, TRUE, FALSE);

   /* Set the top left/right characters of the scrolling window.*/
   setCDKSwindowULChar (installOutput, ACS_LTEE);
   setCDKSwindowURChar (installOutput, ACS_RTEE);

   /* Draw the screen. */
   drawCDKScreen (cdkScreen);
   wrefresh (progressBar->win);
   wrefresh (installOutput->win);

   /* Start copying the files. */
   for (x=0; x < count; x++)
   {
     /*
      * If the 'file' list file has 2 columns, the first is
      * the source filename, the second being the destination
      * filename.
      */
      files = CDKsplitString (fileList[x], ' ');
      chunks = CDKcountStrings (files);
      if (chunks == 2)
      {
	 /* Create the correct paths. */
	 sprintf (oldPath, "%s/%s", sourceDir, files[0]);
	 sprintf (newPath, "%s/%s", destDir, files[1]);
      }
      else
      {
	 /* Create the correct paths. */
	 sprintf (oldPath, "%s/%s", sourceDir, fileList[x]);
	 sprintf (newPath, "%s/%s", destDir, fileList[x]);
      }
      CDKfreeStrings(files);

      /* Copy the file from the source to the destination. */
      ret = copyFile (cdkScreen, oldPath, newPath);
      if (ret == vCanNotOpenSource)
      {
	 sprintf (temp, "</16>Error: Can not open source file %s<!16>", oldPath);
	 errors++;
      }
      else if (ret == vCanNotOpenDest)
      {
	 sprintf (temp, "</16>Error: Can not open destination file %s<!16>", newPath);
	 errors++;
      }
      else
      {
	 sprintf (temp, "</24>%s -> %s", oldPath, newPath);
      }

      /* Add the message to the scrolling window. */
      addCDKSwindow (installOutput, temp, BOTTOM);
      drawCDKSwindow (installOutput, ObjOf(installOutput)->box);

      /* Update the histogram. */
      setCDKHistogram (progressBar, vPERCENT, TOP, A_BOLD,
			1, count, x+1, 
			COLOR_PAIR (24) | A_REVERSE | ' ',
			TRUE);
      
      /* Update the screen. */
      drawCDKHistogram (progressBar, TRUE);

      wrefresh (progressBar->win);
      wrefresh (installOutput->win);
   }

  /*
   * If there were errors, inform the user and allow them to look at the
   * errors in the scrolling window.
   */
   if (errors != 0)
   {
      /* Create the information for the dialog box. */
      char *buttons[] = {"Look At Errors Now", "Save Output To A File", "Ignore Errors"};
      mesg[0] = "<C>There were errors in the installation.";
      mesg[1] = "<C>If you want, you may scroll through the";
      mesg[2] = "<C>messages of the scrolling window to see";
      mesg[3] = "<C>what the errors were. If you want to save";
      mesg[4] = "<C>the output of the window you may press </R>s<!R>";
      mesg[5] = "<C>while in the window, or you may save the output";
      mesg[6] = "<C>of the install now and look at the install";
      mesg[7] = "<C>history at a later date.";

      /* Popup the dialog box. */
      ret = popupDialog (cdkScreen, mesg, 8, buttons, 3);
      if (ret == 0)
      {
	 activateCDKSwindow (installOutput, 0);
      }
      else if (ret == 1)
      {
	 injectCDKSwindow (installOutput, 's');
      }
   }
   else
   {
     /*
      * If they specified the name of an output file, then save the
      * results of the installation to that file.
      */
      if (output != 0)
      {
	  dumpCDKSwindow (installOutput, output);
      }
      else
      {
	 /* Ask them if they want to save the output of the scrolling window. */
	 if (quiet == FALSE)
	 {
	    char *buttons[] = {"No", "Yes"};
	    mesg[0] = "<C>Do you want to save the output of the";
	    mesg[1] = "<C>scrolling window to a file?";

	    if (popupDialog (cdkScreen, mesg, 2, buttons, 2) == 1)
	    {
	       injectCDKSwindow (installOutput, 's');
	    }
	 }
      }
   }
   
   /* Clean up. */
   destroyCDKLabel (titleWin);
   destroyCDKHistogram (progressBar);
   destroyCDKSwindow (installOutput);
   destroyCDKScreen (cdkScreen);
   endCDK();

   /* Clean up the file list. */
   for (x=0; x < count; x++)
   {
      freeChar (fileList[x]);
   }
   exit (0);
}

/*
 * This copies a file from one place to another. (tried rename
 * library call, but it is equivalent to mv)
 */
ECopyFile copyFile (CDKSCREEN *cdkScreen GCC_UNUSED, char *src, char *dest)
{
   char command[2000];
   FILE *fd;

   /* Make sure we can open the source file. */
   if ((fd = fopen (src, "r")) == 0)
   {
      return vCanNotOpenSource;
   }
   fclose (fd);

   /*
    * Remove the destination file first, just in case it already exists.
    * This allows us to check if we can write to the desintation file.
    */
   remove (dest);

   /* Try to open the destination. */
   if ((fd = fopen (dest, "w")) == 0)
   {
      return vCanNotOpenDest;
   }
   fclose (fd);

  /*
   * Copy the file. There has to be a better way to do this. I
   * tried rename and link but they both have the same limitation
   * as the 'mv' command that you can not move across partitions.
   * Quite limiting in an install binary.
   */
   sprintf (command, "rm -f %s; cp %s %s; chmod 444 %s", dest, src, dest, dest);
   system (command);
   return vOK;
}

/*
 * This makes sure that the directory given exists. If it
 * doesn't then it will make it.
 * THINK
 */
int verifyDirectory (CDKSCREEN *cdkScreen, char *directory)
{
   char *buttons[]	= {"Yes", "No"};
   int status		= 0;
   mode_t dirMode	= S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH;
   struct stat fileStat;
   char *mesg[10];
   char *error[10];
   char temp[200];

   /* Stat the directory. */
   if (lstat (directory, &fileStat) != 0)
   {
      /* The directory does not exist. */
      if (errno == ENOENT)
      {
	 /* Create the question. */
	 mesg[0] = "<C>The directory ";
	 sprintf (temp, "<C>%s", directory);
	 mesg[1] = copyChar (temp);
	 mesg[2] = "<C>Does not exist. Do you want to";
	 mesg[3] = "<C>create it?";

	 /* Ask them if they want to create the directory. */
	 if (popupDialog (cdkScreen, mesg, 4, buttons, 2) == 0)
	 {
	    /* Create the directory. */
	    if (mkdir (directory, dirMode) != 0)
	    {
	       /* Create the error message. */
	       error[0] = "<C>Could not create the directory";
	       sprintf (temp, "<C>%s", directory);
	       error[1] = copyChar (temp);

#ifdef HAVE_STRERROR
	       sprintf (temp, "<C>%s", strerror (errno));
#else
	       sprintf (temp, "<C>Check the permissions and try again.");
#endif
	       error[2] = copyChar (temp);

	       /* Pop up the error message. */
	       popupLabel (cdkScreen, error, 3);

	       /* Clean up and set the error status. */
	       freeChar (error[1]);
	       freeChar (error[2]);
	       status = -1;
	    }
	 }
	 else
	 {
	    /* Create the message. */
	    error[0] = "<C>Installation aborted.";
	    
	    /* Pop up the error message. */
	    popupLabel (cdkScreen, error, 1);

	    /* Set the exit status. */
	    status = -1;
	 }
 
	 /* Clean up. */
	 freeChar (mesg[1]);
      }
   }
   return status;
}
