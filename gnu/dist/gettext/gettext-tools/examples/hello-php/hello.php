#!@PHP@ -q
<?
  // Example for use of GNU gettext.
  // Copyright (C) 2003 Free Software Foundation, Inc.
  // This file is in the public domain.
  //
  // Source code of the PHP program.

  setlocale (LC_ALL, "");
  textdomain ("hello-php");
  bindtextdomain ("hello-php", "@localedir@");

  echo _("Hello, world!");
  echo "\n";
  echo printf (_("This program is running as process number %d."),
               posix_getpid());
  echo "\n";
?>
