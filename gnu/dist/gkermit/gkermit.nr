.\" @(#) gkermit.1 1.00 Columbia University 1999/12/25
.TH GKERMIT 1C "25 Dec 1999" "UNIX G-Kermit"
.SH NAME
gkermit \- G-Kermit (GNU Kermit) 1.00 file transfer software.
.ll 80
.SH SYNOPSIS
.nf
.sp
gkermit [ options ] -s file(s)    Send files
gkermit [ options ] -g file(s)    Get files
gkermit [ options ] -r            Receive files
.PP
.SH DESCRIPTION
G-Kermit is a UNIX program for transferring files using the
Kermit protocol.  G-Kermit is a product of Kermit Project at Columbia
University.  It is free software under the GNU Public License.  See the
COPYING file for details.
.PP
.SS INVOKING G-KERMIT
.PP
The G-Kermit binary is called "gkermit".  It should be stored someplace
in your UNIX PATH; normally it is available as /usr/local/bin/gkermit.
To run G-Kermit, just type "gkermit" followed by command-line options that
tell it what to do.  If no options are given, it prints a usage message
listing the available options.
.PP
If an option takes an argument, the argument is required; if an option does
not take an argument, no argument may be given (exception: -d).  The action
options are -r, -s, and -g.  Only one action option may be given.  If no
action options are given, G-Kermit does nothing (except possibly to print its
usage message or create a debug.log file).  Here are some examples ("$ " is
the shell prompt):
.nf
.sp
  $ gkermit -s hello.c   <-- Send the hello.c file
  $ gkermit -s hello.*   <-- Send all hello.* files
  $ gkermit -r           <-- Wait to receive files
  $ gkermit -g hello.c   <-- Get hello.c file
  $ gkermit -g hello.\\*  <-- Get all hello.* files
.sp
.fi
Options that do not take arguments can be "bundled" with other options.
An option that takes an argument must always be followed by a space and
then its argument(s).  Examples:
.nf
.sp
  $ gkermit -is hello.o  <-- Send hello.o in binary mode
  $ gkermit -dr          <-- Receive with debugging
.sp
.fi
.SS COMMAND-LINE OPTIONS
.sp
.nf
  -r      RECEIVE.  Wait for incoming files.
  -s fn   SEND.  Send file(s) specified by fn.
  -g fn   GET.  Get specified file(s) from server.
  -a fn   AS-NAME.  Alternative name for file.
  -i      IMAGE.  Binary-mode transfer (default).
  -T      TEXT. Text-mode transfer.
  -P      PATH (filename) conversion disabled.
  -w      WRITEOVER when filenames collide.
  -K      KEEP incompletely received files.
  -p x    PARITY. x = e,o,m,s,n; default = n(one).
  -e n    PACKET LENGTH.  n = 40-9000; default=4000.
  -b n    TIMEOUT.  Per-packet timeout, seconds.
  -x      XON/XOFF.  Set Xon/Xoff in the tty driver.
  --x     Unset Xon/Xoff in the tty driver.
  -S      STREAMING disabled.
  -X      EXTERNAL.  G-Kermit is an external protocol.
  -q      QUIET.  Suppress messages.
  -d      DEBUG.  Write debugging info to ./debug.log.
  -d fn   DEBUG.  Write debugging info to given file.
  -h      HELP.  Display brief usage message.
.fi
.PP
You may supply options to G-Kermit on the command line or through the
GKERMIT environment variable, which can contain any valid gkermit
command-line options.  These are processed before the actual command-line
options and so can be overridden by them.  Example for bash or ksh, which you
can put in your profile if you want to always keep incomplete files, suppress
streaming, suppress messages, and use Space parity:
.nf
.sp
  export GKERMIT="-K -S -q -p s"
.fi
.PP
.SS MECHANICS OF FILE TRANSFER
.PP
To transfer files with G-Kermit you must be connected through a terminal
emulator to the UNIX system where G-Kermit is running, meaning you are
online to UNIX and have access to the shell prompt (or to a menu that has
an option to invoke G-Kermit).  The connection can be serial (direct or
dialed) or network (Telnet, Rlogin, X.25, etc).
.PP
When you tell G-Kermit to SEND a file (or files), e.g. with:
.nf
.sp
  $ gkermit -Ts oofa.txt
.sp
.fi
it pauses for a second and then sends its first packet.  What happens next
depends on the capabilities of your terminal emulator:
.PP
.in +0.5i
.ll -0.5i
.ta +0.2i
.ti -0.2i
\(bu	If your emulator supports Kermit "autodownloads" then it receives the
file automatically and puts you back in the terminal screen when done.
.sp
.ti -0.2i
\(bu	Otherwise, you'll need to take whatever action is required by your
emulator to get its attention: a mouse action, a keystroke like Alt-x,
or a character sequence like Ctrl-\\ or Ctrl-] followed by the letter
"c" (this is called "escaping back") and then tell it to receive the
file.  When the transfer is complete, you must instruct your emulator
to go back to its terminal screen.
.ll +0.5i
.in -0.5i
.fi
.PP
During file transfer, most terminal emulators put up some kind of running
display of the file transfer progress.
.PP
When you tell G-Kermit to RECEIVE (with "gkermit -r"), this requires you to
escape back to your terminal emulator and instruct it to send the desired
file(s).
.PP
If your terminal emulator supports Kermit autodownloads AND Kermit server
mode, then you can use GET ("gkermit -g files...") rather than RECEIVE
("gkermit -r"), and the rest happens automatically, as when G-Kermit is
sending.
.SS INTERRUPTING FILE TRANSFER
.PP
G-Kermit supports file and group interruption.  The method for interrupting
a transfer depends on your terminal emulator.  For example, while the
file-transfer display is active, you might type the letter 'x' to cancel the
current file and go on to the next one (if any), and the letter 'z' to
cancel the group.  Or there might be buttons you can click with your mouse.
.PP
When G-Kermit is in packet mode and your terminal emulator is in its
terminal screen, you can also type three (3) Ctrl-C characters in a row to
make G-Kermit exit and restore the normal terminal modes.
.PP
.SS TEXT AND BINARY TRANSFER MODE
.PP
When sending files in binary mode, G-Kermit sends every byte exactly as it
is stored on the disk.  This mode is appropriate for program binaries,
graphics files, tar archives, compressed files, etc, and is G-Kermit's
default file transfer mode when sending.  When receiving files in binary
mode, G-Kermit simply copies each byte to disk.  (Obviously the bytes are
encoded for transmission, but the encoding and decoding procedures give a
replica of the original file after transfer.)
.PP
When sending files in text mode, G-Kermit converts the record format to the
common one that is defined for the Kermit protocol, namely lines terminated
by carriage return and linefeed (CRLF); the receiver converts the CRLFs to
whatever line-end or record-format convention is used on its platform.  When
receiving files in text mode, G-Kermit simply strips carriage returns,
leaving only a linefeed at the end of each line, which is the UNIX
convention.
.PP
When receiving files, the sender's transfer mode (text or binary)
predominates if the sender gives this information to G-Kermit in a Kermit
File Attribute packet, which of course depends on whether your terminal
emulator's Kermit protocol has this feature.  Otherwise, if you gave a -i or
-T option on the gkermit command line, the corresponding mode is used;
otherwise the default mode (binary) is used.
.PP
Furthermore, when either sending or receiving, G-Kermit and your terminal
emulator's Kermit can inform each other of their OS type (UNIX in G-Kermit's
case).  If your emulator supports this capability, which is called
"automatic peer recognition", and it tells G-Kermit that its platform is
also UNIX, G-Kermit and the emulator's Kermit automatically switch into
binary mode, since no record-format conversion is necessary in this case.
Automatic peer recognition is disabled automatically if you include the -i
(image) or -T (text) option.
.PP
When sending, G-Kermit sends all files in the same mode, text or binary.
There is no automatic per-file mode switching.  When receiving, however,
per-file switching occurs automatically based on the incoming Attribute
packets, if any (explained below), that accompany each file.
.PP
.SS PATHNAMES
.PP
When SENDING a file, G-Kermit obtains the filenames from the command line.
It depends on the shell to expand metacharacters (wildcards and tilde).
.PP
G-Kermit uses the full pathname given to find and open the file, but then
strips the pathname before sending the name to the receiver.  For example:
.nf
.sp
  $ gkermit -s /etc/hosts
.sp
.fi
results in the receiver getting a file called "HOSTS" or "hosts" (the
directory part, "/etc/", is stripped).
.PP
However, if a pathname is included in the -a option, the directory part
is not stripped:
.nf
.sp
  $ gkermit -s /etc/hosts -a /tmp/hosts
.sp
.fi
This example sends the /etc/hosts file but tells the receiver that its name
is "/tmp/hosts".  What the receiver does with the pathname is, of course, up
to the receiver, which might have various options for dealing with incoming
pathnames.
.PP
When RECEIVING a file, G-Kermit does NOT strip the pathname.  If the
incoming filename includes a path, G-Kermit tries to store the file in the
specified place.  If the path does not exist, the transfer fails.  The
incoming pathname can, of course, be overridden with the -a option.
.PP
.SS FILENAME CONVERSION
.PP
When sending a file, G-Kermit normally converts outbound filenames to
common form: uppercase, no more than one period, and no funny characters.
So, for example, gkermit.tar.gz would be sent as GKERMIT_TAR.GZ.
.PP
When receiving a file, if the name is all uppercase, G-Kermit converts it
to all lowercase.  If the name contains any lowercase letters, G-Kermit
leaves the name alone.
.PP
If the automatic peer recognition feature is available in the terminal
emulator, and G-Kermit recognizes the emulator's platform as UNIX, G-Kermit
automatically disables filename conversion and sends and accepts filenames
literally.
.PP
You can force literal filenames by including the -P option on the command
line.
.PP
.SS FILENAME COLLISIONS
.PP
When G-Kermit receives a file whose name is the same as that of an existing
file, G-Kermit "backs up" the existing file by adding a unique suffix to its
name.  The suffix is ".~n~", where n is a number.  This kind of backup suffix
is compatible with GNU EMACS and various other popular applications.
.PP
To defeat the backup feature and have incoming files overwrite existing
files of the same name, include the -w (writeover) option on the command
line.
.PP
.SH RETURN VALUES
G-Kermit resturns an exit status code of 0 if all actions succeeded
and 1 if any actions failed.
.PP
.SH IMPLEMENTATION NOTES
G-Kermit is designed to be small, portable, and stable, and is intended for
use only on the "far end" of a connection; it does not make connections
itself, although it can be used as an external protocol by other programs that
do make connections.  To keep it small and stable, it does not include sliding
windows, a command or scripting language or character-set translation.  To
keep it portable and stable, it avoids use of system services that are not
standardized across all UNIX varieties and therefore, in particular, does not
support file timestamps, internal wildcard expansion, and other features that
are not implemented consistently (or at all) across all UNIXes.
.PP
.SH ENVIRONMENT
A GKERMIT environment variable may be defined (for example in your shell
profile) to include G-Kermit command-line options; these are processed by
G-Kermit before any options that are specified on the command line, and
therefore are overriden by command-line options.
.PP
.SH DIAGNOSTICS
If an error occurs during file
transfer G-Kermit sends an error packet to your terminal emulator to cancel
the transfer; an appropriate error message should be displayed on your
screen.
.PP
.SH ERRORS
File transfers can fail for a number of reasons:
.sp
.in +0.5i
.ll -0.5i
.ta +0.2i
.ti -0.2i
\(bu	Lack of read access to a source file.
.ti -0.2i
\(bu	Lack of write access to a target directory.
.ti -0.2i
\(bu	Lack of adequate flow control.
.ti -0.2i
\(bu	Use of streaming on an unreliable connection.
.ti -0.2i
\(bu	Excessive unprefixing of control characters.
.ti -0.2i
\(bu	Sending bare 8-bit data on a 7-bit connection.
.ti -0.2i
\(bu	Packets too long for receiver's buffers.
.ti -0.2i
\(bu	Timeout interval too short for connection.
.ti -0.2i
.ll +0.5i
.in -0.5i
.fi
.sp
and many others; these are covered in the references.
.PP
.SH REFERENCES
.PP
The Kermit protocol is specified in "Kermit, A File Transfer Protocol" by
Frank da Cruz, Digital Press (1987).  A correctness proof of the Kermit
protocol appears in "Specification and Validation Methods", edited by Egon
Boerger, Oxford University Press (1995).  "Using C-Kermit" by Frank da Cruz
and Christine M. Gianone, Digital Press (1997, or later edition) explains many
of the terms and techniques referenced here in case you are not familiar with
them, and also includes tutorials on data communications, extensive
troubleshooting and performance tips, etc.  Various other books on Kermit are
available from Digital Press.  Online resources include:
.nf
.sp
  Web:   http://www.columbia.edu/kermit/
  FTP:   ftp://kermit.columbia.edu/kermit/g/
  News:  comp.protocols.kermit.misc
  Email: kermit-support@columbia.edu
.fi
.sp
Also see the README file distributed with G-Kermit for further detail.  It
can also be found at ftp://kermit.columbia.edu/kermit/g/README.
.PP
.SH BUGS
The speed of a file transfer depends not only on the speed of the two
computers involved and the characteristics of the connection, but also on the
capabilities and configuration of the two Kermit programs.  Kermit is a fast
and reliable protocol, but not all implementations of it are necessarily fast
or reliable.
.PP
Nonstreaming transfers on a TCP/IP connection might be inordinately slow
if one or both of the TCP/IP stacks uses the Nagle or Delayed ACK tricks.
Streaming is used automatically if the other Kermit supports it.
.PP
When receiving files in text mode, G-Kermit strips all carriage returns,
even if they aren't followed by linefeed.
.PP
A backups files are not guaranteed to have the highest number in their
backup suffix.
.PP
.SH AUTHOR
Frank da Cruz, the Kermit Project, Columbia University, New York City,
December 1999.
.br
