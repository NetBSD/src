#!/usr/local/bin/perl
# -*-perl-*-

# Test driver for the Make test suite

# Usage:  run_make_tests  [testname]
#                         [-debug]
#                         [-help]
#                         [-verbose]
#                         [-keep]
#                         [-make <make prog>]
#                        (and others)

$valgrind = 0;              # invoke make with valgrind

require "test_driver.pl";

#$SIG{INT} = sub { print STDERR "Caught a signal!\n"; die @_; };

sub valid_option
{
   local($option) = @_;

   if ($option =~ /^-make([-_]?path)?$/)
   {
      $make_path = shift @argv;
      if (!-f $make_path)
      {
	 print "$option $make_path: Not found.\n";
	 exit 0;
      }
      return 1;
   }

   if ($option =~ /^-valgrind$/i) {
     $valgrind = 1;
     return 1;
   }

# This doesn't work--it _should_!  Someone needs to fix this badly.
#
#   elsif ($option =~ /^-work([-_]?dir)?$/)
#   {
#      $workdir = shift @argv;
#      return 1;
#   }

   return 0;
}

sub run_make_with_options
{
   local ($filename,$options,$logname,$expected_code) = @_;
   local($code);
   local($command) = $make_path;

   $expected_code = 0 unless defined($expected_code);

   if ($filename)
   {
      $command .= " -f $filename";
   }

   if ($options)
   {
      $command .= " $options";
   }

   if ($valgrind) {
     print VALGRIND "\n\nExecuting: $command\n";
   }

   $code = &run_command_with_output($logname,$command);

   # Check to see if we have Purify errors.  If so, keep the logfile.
   # For this to work you need to build with the Purify flag -exit-status=yes

   if ($pure_log && -f $pure_log) {
     if ($code & 0x7000) {
       $code &= ~0x7000;

       # If we have a purify log, save it
       $tn = $pure_testname . ($num_of_logfiles ? ".$num_of_logfiles" : "");
       print("Renaming purify log file to $tn\n") if $debug;
       rename($pure_log, "$tn")
	 || die "Can't rename $log to $tn: $!\n";
       ++$purify_errors;
     }
     else {
       unlink($pure_log);
     }
   }

   if ($code != $expected_code)
   {
      print "Error running $make_path ($code): $command\n";
      $test_passed = 0;
      # If it's a SIGINT, stop here
      if ($code & 127) {
        print STDERR "\nCaught signal ".($code & 127)."!\n";
        exit($code);
      }
      return 0;
   }

   if ($profile & $vos)
   {
      system "add_profile $make_path";
   }
1;
}

sub print_usage
{
   &print_standard_usage ("run_make_tests", "[-make_path make_pathname]");
}

sub print_help
{
   &print_standard_help ("-make_path",
          "\tYou may specify the pathname of the copy of make to run.");
}

sub get_this_pwd {
  if ($vos) {
    $delete_command = "delete_file";
    $__pwd = `++(current_dir)`;
  }
  else {
    $delete_command = "rm";
    chop ($__pwd = `pwd`);
  }

  return $__pwd;
}

sub set_defaults
{
   # $profile = 1;
   $testee = "GNU make";
   $make_path = "make";
   $tmpfilesuffix = "mk";
   $pwd = &get_this_pwd;
}

sub set_more_defaults
{
   local($string);
   local($index);

   # Make sure we're in the C locale for those systems that support it,
   # so sorting, etc. is predictable.
   #
   $ENV{LANG} = 'C';

   # find the type of the port.  We do this up front to have a single
   # point of change if it needs to be tweaked.
   #
   # This is probably not specific enough.
   #
   if ($osname =~ /Windows/i) {
     $port_type = 'W32';
   }
   # Bleah, the osname is so variable on DOS.  This kind of bites.
   # Well, as far as I can tell if we check for some text at the
   # beginning of the line with either no spaces or a single space, then
   # a D, then either "OS", "os", or "ev" and a space.  That should
   # match and be pretty specific.
   elsif ($osname =~ /^([^ ]*|[^ ]* [^ ]*)D(OS|os|ev) /) {
     $port_type = 'DOS';
   }
   # Everything else, right now, is UNIX.  Note that we should integrate
   # the VOS support into this as well and get rid of $vos; we'll do
   # that next time.
   else {
     $port_type = 'UNIX';
   }

   # On DOS/Windows system the filesystem apparently can't track
   # timestamps with second granularity (!!).  Change the sleep time
   # needed to force a file to be considered "old".
   #
   $wtime = $port_type eq 'UNIX' ? 1 : 4;

   # Find the full pathname of Make.  For DOS systems this is more
   # complicated, so we ask make itself.

   $make_path = `sh -c 'echo "all:;\@echo \\\$(MAKE)" | $make_path -f-'`;
   chop $make_path;
   print "Make\t= `$make_path'\n" if $debug;

   $string = `$make_path -v -f /dev/null 2> /dev/null`;

   $string =~ /^(GNU Make [^,\n]*)/;
   $testee_version = "$1\n";

   $string = `sh -c "$make_path -f /dev/null 2>&1"`;
   if ($string =~ /(.*): \*\*\* No targets\.  Stop\./) {
     $make_name = $1;
   }
   else {
     if ($make_path =~ /$pathsep([^\n$pathsep]*)$/) {
       $make_name = $1;
     }
     else {
       $make_name = $make_path;
     }
   }

   # prepend pwd if this is a relative path (ie, does not
   # start with a slash, but contains one).  Thanks for the
   # clue, Roland.

   if (index ($make_path, ":") != 1 && index ($make_path, "/") > 0)
   {
      $mkpath = "$pwd$pathsep$make_path";
   }
   else
   {
      $mkpath = $make_path;
   }

   # Get Purify log info--if any.

   $ENV{PURIFYOPTIONS} =~ /.*-logfile=([^ ]+)/;
   $pure_log = $1 || '';
   $pure_log =~ s/%v/$make_name/;
   $purify_errors = 0;

   $string = `sh -c "$make_path -j 2 -f /dev/null 2>&1"`;
   if ($string =~ /not supported/) {
     $parallel_jobs = 0;
   }
   else {
     $parallel_jobs = 1;
   }

   # Set up for valgrind, if requested.

   if ($valgrind) {
#     use POSIX qw(:fcntl_h);
#     require Fcntl;
     open(VALGRIND, "> valgrind.out")
       || die "Cannot open valgrind.out: $!\n";
     #  -q --leak-check=yes
     $make_path = "valgrind --num-callers=15 --logfile-fd=".fileno(VALGRIND)." $make_path";
     # F_SETFD is 2
     fcntl(VALGRIND, 2, 0) or die "fcntl(setfd) failed: $!\n";
     system("echo Starting on `date` 1>&".fileno(VALGRIND));
     print "Enabled valgrind support.\n";
   }
}

sub setup_for_test
{
  $makefile = &get_tmpfile;
  if (-f $makefile) {
    unlink $makefile;
  }

  # Get rid of any Purify logs.
  if ($pure_log) {
    ($pure_testname = $testname) =~ tr,/,_,;
    $pure_testname = "$pure_log.$pure_testname";
    system("rm -f $pure_testname*");
    print("Purify testfiles are: $pure_testname*\n") if $debug;
  }
}

exit !&toplevel;
