G-Kermit  --  GNU Kermit                                          -*- text -*-

Version 1.00 : 25 December 1999

Author:
  Frank da Cruz
  The Kermit Project
  Columbia University
  612 West 115th Street
  New York NY 10025-7799  USA
  fdc@columbia.edu


CONTENTS...

   1. OVERVIEW
   2. INVOKING G-KERMIT
   3. COMMAND-LINE OPTIONS
   4. THE MECHANICS OF FILE TRANSFER
   5. INTERRUPTING FILE TRANSFER
   6. TEXT AND BINARY TRANSFER MODE
   7. PATHNAMES
   8. FILENAME CONVERSION
   9. FILENAME COLLISIONS
  10. KERMIT PROTOCOL DETAILS
  11. PROBLEMS, BUGS, ERRORS
  12. BUILDING G-KERMIT
  13. INSTALLING G-KERMIT
  14. DESIGN AND IMPLEMENTATION NOTES


1. OVERVIEW

G-Kermit is a Unix program for uploading and downloading files with the
Kermit protocol.  G-Kermit is a product of Kermit Project at Columbia
University.  It is free software under the GNU Public License.  See the
COPYING file for details.

G-Kermit is:
 . Fast
 . Small
 . Portable
 . Easy to use
 . Interoperable
 . Low-maintenance
 . Stable and reliable

Features include:
 . Text and binary file transfer on both 7-bit and 8-bit connections
 . Files can be transferred singly or in groups
 . Automatic startup configuration via GKERMIT environment variable
 . Configurability as an external protocol

Kermit protocol features include:
 . Automatic peer recognition
 . Streaming on reliable connections
 . Selectable packet length, 40 to 9000 bytes (4000 default)
 . Single shifts for 8-bit data on 7-bit connections
 . Control-character prefixing for control-character transparency
 . Control-character unprefixing for increased speed (incoming only)
 . Compression of repeated bytes
 . Per-file and batch cancellation

Features selectable on command line:
 . Text or binary mode transfer
 . Filename conversion on/off
 . Filename collision backup versus overwrite
 . Keep or discard incompletely received files
 . Packet length
 . Packet timeout
 . Flow control
 . Parity
 . Streaming
 . Messages
 . Debugging

Features not included (see Section 14):
 . Making connections
 . Character-set translation
 . Interactive commands and scripting
 . File date-time stamps


2. INVOKING G-KERMIT

G-Kermit is always on the "far end" of a connection, on a Unix system that
you have made a connection to from a terminal emulator by dialup, network,
or direct serial.  If you have a direct or dialup serial connection into
Unix, use the "stty -a" or "stty all" command to see if your Unix terminal
driver is conditioned for the appropriate kind of flow control; if it isn't,
very few applications (including gkermit) will work well, or at all.  The
command for setting terminal flow control varies from platform to platform,
but it is usually something like this:

 $ stty crtscts

(where "$ " is the shell prompt) for RTS/CTS hardware flow control, or:

 $ stty ixon ixoff

for Xon/Xoff "software" flow control.  When you have a network connection,
flow control is usually nothing to worry about, since the network protocol
(TCP or X.25) takes care of it automatically, but on certain platforms (such
as HP-UX) the TCP/IP Telnet or Rlogin server uses this for flow control
between itself and the underlying pseudoterminal in which your session runs,
so Xon/Xoff might be required for these sessions too.

The G-Kermit binary is called "gkermit".  It should be stored someplace in
your Unix PATH, such as /usr/local/bin/gkermit or somewhere in the /opt tree
on System V R4.  To run G-Kermit, just type "gkermit" followed by command-
line options that tell it what to do.  If no options are given, G-Kermit
prints a usage message listing the available options:

 G-Kermit CU-1.00, Columbia University, 1999-12-25: POSIX.
 Usage:  gkermit [ options ]
 Options:
  -r      Receive files
  -s fn   Send files
  -g fn   Get files from server
  -a fn   As-name for single file
  -i      Image (binary) mode transfer
  -T      Text mode transfer
  -P      Path/filename conversion disabled
  -w      Write over existing files with same name
  -K      Keep incompletely received files
  -p x    Parity: x = o[dd],e[ven],m[ark],s[pace],n[one]
  -e n    Receive packet-length (40-9000)
  -b n    Timeout (sec, 0 = none)
  -x      Force Xon/Xoff (--x = Don't force Xon/Xoff)
  -S      Disable streaming
  -X      External protocol
  -q      Quiet (suppress messages)
  -d [fn] Debug to ./debug.log [or specified file]
  -h      Help (this message)
 More info: http://www.columbia.edu/kermit/ <kermit@columbia.edu>

If an option takes an argument, the argument is required; if an option does
not take an argument, no argument may be given (exceptions: -d may or may
not take an argument; -s can take 1 or more arguments).

The action options are -r, -s, and -g.  Only one action option may be given.
If no action options are given, G-Kermit does nothing (except possibly
printing its usage message or creating a debug.log file).  Here are some
examples (in which "$ " is the shell prompt):

  $ gkermit -s hello.c       <-- Sends the hello.c file.
  $ gkermit -s hello.*       <-- Sends all hello.* files.
  $ gkermit -r               <-- Waits for you to send a file to it.
  $ gkermit -g hello.c       <-- Gets the hello.c file from your computer.
  $ gkermit -g \*.c          <-- Gets all *.c files from your computer.

Options that do not take arguments can be "bundled" with other options.
An option that takes an argument must always be followed by a space and
then its argument(s).  Examples:

  $ gkermit -is hello.o      <-- Sends hello.o in binary mode.
  $ gkermit -dSr             <-- Receives with debugging and no streaming.

G-Kermit's exit status is 0 if all operations succeeded and 1 if there were
any failures.  If a group of files was transferred, the exit status is 1
if one or more files was not successfully transferred and 0 if all of them
were transferred successfully.


3. COMMAND-LINE OPTIONS

  -r     RECEIVE: This option tells G-Kermit to receive a file or files;
         that is, to passively wait for you to send files from your
         terminal emulator.

  -s fn  SEND: This tells G-Kermit to send the file or files specified by
         fn, which can be a filename, a regular expression, or a list of
         filenames and/or regular expressions (wildcards).  Regular
         expressions are interpreted and expanded by your shell into the
         list of names of files that is given to G-Kermit.  For example
         "*.c" expands to a list of all files in the current directory
         whose names end with ".c".

  -g fn  GET: This option tells G-Kermit to get a file (or files) from a
         Kermit server.  It is useful only when your terminal emulator
         supports the Kermit autodownload feature AND it includes a Kermit
         server mode.  It is equivalent to "gkermit -r", escaping back,
         telling your terminal emulator to send the given files, and then
         reconnecting to Unix.

  -a fn  AS-NAME: When used with -s, this option tells G-Kermit to send
         the file whose name is given as the first -s argument under the
         name fn.  For example, "gkermit -s game -a work" sends the file
         called "game" under the name "work", so the receiver will think
         its name is "work".  When given with the -r or -g command, the
         incoming file (or the first incoming file if there is more than
         one) is stored under the name fn.  In all cases, the given name
         is used as-is; it is not converted.

  -i     IMAGE (binary) mode transfer.  When used with -s, tells G-Kermit
         to send in binary mode.  When used with -r, tells G-Kermit to
         receive in binary mode if the file sender does not specify the
         transfer mode (text or binary).  When used with -g, tells
         G-Kermit to ask your terminal emulator's Kermit to send the given
         file in binary mode.  See Section 6 for details.

  -T     TEXT mode transfer (note uppercase T).  When used with -s, tells
         G-Kermit to send in text mode.  When used with -r, tells G-Kermit
         to receive in text mode if the file sender does not specify the
         transfer mode (text or binary).  When used with -g, tells G-Kermit
         to ask your emulator's Kermit to send the given file in text mode.
         See Section 6 for details.

  -P     PATH (filename) conversion disabled (note uppercase P).
         Normally when sending files, G-Kermit converts filenames to a
         form that should be acceptable to non-Unix platforms, primarily
         changing lowercase letters to uppercase, ensuring there is no
         more than one period, and replacing any "funny" characters by X
         or underscore (explained in Section 8).

  -w     WRITEOVER.  When receiving, and an incoming file has the same
         name as an existing file, write over the existing file.  By
         default G-Kermit backs up the existing file by adding a suffix
         to its name (see Section 9).

  -K     KEEP incompletely received files.  Normally when receiving files,
         and a file transfer is interrupted, G-Kermit discards the
         partially received file so you won't think you have the whole
         file.  Include -K on the command line to tell G-Kermit to keep
         partially received files, e.g. "gkermit -Kr".

  -p x   PARITY: Use the given kind of parity, where x can be 'n' for
         None (which is the default, for use on 8-bit-clean connections);
         's' for Space, 'e' for Even, 'o' for Odd, and 'm' for Mark.  's'
         might be needed on certain Telnet connections; 'e', 'o', and 'm'
         are only for serial connections; don't try them on TCP/IP
         connections.

  -e n   PACKET LENGTH: Receive packet-length, where n can be any number
         between 40 and 9000.  The default length on most platforms is
         4000.  Use this option to specify a different length; usually
         this would be necessary only if transfers fail using the default
         length due to some kind of buffering problem in the host or along
         the communication path.  Example: "gkermit -e 240 -r".

  -b n   TIMEOUT (sec, 0 = none).  Specify the number of seconds to wait
         for a packet before timing out and retransmitting.  By default,
         G-Kermit uses whatever timeout interval your terminal emulator's
         Kermit asks it to use.  No need to change this unless the timeout
         action causes problems.

  -x     XON/XOFF.  Force Xon/Xoff flow control in the Unix terminal
         driver.  Try this if uploads fail without it.  But don't use it
         if you don't need to; on some platforms or connections it hurts
         rather than helps.

  --x    Don't force Xon/Xoff; for use when G-Kermit was built with the
         SETXONXOFF compile-time option (Section 12), to override the
         automatic setting of Xon/Xoff in case it interferes with file
         transfers.

  -S     STREAMING disabled.  Streaming is a high-performance option to
         be used on reliable connections, such as in Telnet or Rlogin
         sessions.  It is used if your terminal emulator's Kermit requests
         it.  Use the -S option (note: uppercase S) to suppress this
         feature in case it causes trouble.  Details in Section 10.

  -X     EXTERNAL PROTOCOL.  Include this option when invoking G-Kermit
         from another program that redirects G-Kermit's standard i/o,
         e.g. over a connection to another computer.  If you omit this
         switch when using G-Kermit as an external protocol to another
         communications program, G-Kermit is likely to perform illegal
         operations and exit prematurely.  If you include this switch
         when G-Kermit is NOT an external protocol to another program,
         file transfers will fail.  G-Kermit has no way of determining
         automatically whether it is being used as an external protocol.

  -q     QUIET.  Suppresses messages.

  -d     DEBUG.  Use this for troubleshooting.  It creates a file called
         debug.log in your current directory, to be used in conjunction
         with the source code, or sent to the Kermit support address for
         analysis.  More about this in Section 11.

  -d fn  DEBUG to specified file (rather than default ./debug.log).

  -h     HELP: Displays the usage message shown above.

You may supply options to G-Kermit on the command line or through the
GKERMIT environment variable, which can contain any valid gkermit
command-line options.  These are processed before the actual command-line
options and so can be overridden by them.  Example for bash or ksh, which
you can put in your profile if you want to always keep incomplete files,
suppress streaming, suppress messages, and use Space parity:

  export GKERMIT="-K -S -q -p s"

G-Kermit's options are compatible with C-Kermit's, with the following
exceptions:

  -P (available only in C-Kermit 7.0 and later)
  -K (currently not used in C-Kermit)
  -b (used in C-Kermit for serial device speed)
  -S (used in C-Kermit to force an interactive command prompt)
  -x (used in C-Kermit to start server mode)
 --x (currently not used in C-Kermit)
  -X (currently not used in C-Kermit)


4. THE MECHANICS OF FILE TRANSFER

To transfer files with G-Kermit you must be connected through a terminal
emulator to the Unix system where G-Kermit is installed, meaning you are
online to Unix and have access to the shell prompt (or to some menu that has
an option to invoke G-Kermit), and your terminal emulator must support the
Kermit file transfer protocol.  The connection can be serial (direct or
dialed) or network (Telnet, Rlogin, X.25, etc).

4.1. Sending Files

When you tell G-Kermit to SEND a file (or files), e.g. with:

  $ gkermit -Ts oofa.txt

it pauses for a second and then sends its first packet.  What happens next
depends on the capabilities of your terminal emulator:

 . If your emulator supports Kermit "autodownloads" then it receives the
   file automatically and puts you back in the terminal screen when done.

 . Otherwise, you'll need to take whatever action is required by your
   emulator to get its attention: a mouse action, a keystroke like Alt-x,
   or a character sequence like Ctrl-\ or Ctrl-] followed by the letter
   "c" (this is called "escaping back") and then tell it to receive the
   file.  When the transfer is complete, you might have to instruct your
   emulator to go back to its terminal screen.

During file transfer, most terminal emulators put up some kind of running
display of the file transfer progress.

4.2. Receiving Files

When you tell G-Kermit to RECEIVE, this requires you to escape back to your
terminal emulator and instruct it to send the desired file(s).  Autodownload
is not effective in this case.  When the transfer is complete, you'll need
to instruct your emulator to return to its terminal screen.

4.3. Getting Files

If your terminal emulator supports Kermit autodownloads AND server mode, you
can use GET ("gkermit -g files...") rather than RECEIVE ("gkermit -r"), and
the rest happens automatically, as when G-Kermit is sending.


5. INTERRUPTING FILE TRANSFER

G-Kermit supports file and group interruption.  The method for interrupting
a transfer depends on your terminal emulator.  For example, while the
file-transfer display is active, you might type the letter 'x' to cancel the
current file and go on to the next one (if any), and the letter 'z' to
cancel the group.  Or there might be buttons you can click with your mouse.

When G-Kermit is in packet mode and your terminal emulator is in its
terminal screen, you can also type three (3) Ctrl-C characters in a row to
make G-Kermit exit and restore the normal terminal modes.


6. TEXT AND BINARY TRANSFER MODE

When sending files in binary mode, G-Kermit sends every byte exactly as it
appears in the file.  This mode is appropriate for program binaries,
graphics files, tar archives, compressed files, etc, and is G-Kermit's
default file-transfer mode when sending.  When receiving files in binary
mode, G-Kermit simply copies each byte to disk.  (Obviously the bytes are
encoded for transmission, but the encoding and decoding procedures give a
replica of the original file after transfer.)

When sending files in text mode, G-Kermit converts the record format to the
common one that is defined for the Kermit protocol, namely lines terminated
by carriage return and linefeed (CRLF); the receiver converts the CRLFs to
whatever line-end or record-format convention is used on its platform.  When
receiving files in text mode, G-Kermit simply strips carriage returns,
leaving only a linefeed at the end of each line, which is the Unix
convention.

When receiving files, the sender's transfer mode (text or binary)
predominates if the sender gives this information to G-Kermit in a Kermit
File Attribute packet, which of course depends on whether your terminal
emulator's Kermit protocol has this feature.  Otherwise, if you gave a -i or
-T option on the gkermit command line, the corresponding mode is used;
otherwise the default mode (binary) is used.

Furthermore, when either sending or receiving, G-Kermit and your terminal
emulator's Kermit can inform each other of their OS type (Unix in G-Kermit's
case).  If your emulator supports this capability, which is called
"automatic peer recognition", and it tells G-Kermit that its platform is
also Unix, G-Kermit and the emulator's Kermit automatically switch into
binary mode, since no record-format conversion is necessary in this case.
Automatic peer recognition is disabled automatically if you include the -i
(image) or -T (text) option.

When sending, G-Kermit sends all files in the same mode, text or binary.
There is no automatic per-file mode switching.  When receiving, however,
per-file switching occurs automatically based on the incoming Attribute
packets, if any (explained below), that accompany each file, so if the
file sender switches types between files, G-Kermit follows along.


7. PATHNAMES

When SENDING a file, G-Kermit obtains the filenames from the command line.
It depends on the shell to expand metacharacters (wildcards and tilde).

G-Kermit uses the full pathname given to find and open the file, but then
strips the pathname before sending the name to the receiver.  For example:

  $ gkermit -s /etc/hosts

results in an arriving file called "HOSTS" or "hosts" (the directory part,
"/etc/", is stripped; see next section about capitalization).

However, if a pathname is included in the -a option, the directory part
is not stripped:

  $ gkermit -s /etc/hosts -a /tmp/hosts

This example sends the /etc/hosts file but tells the receiver that its name
is "/tmp/hosts".  What the receiver does with the pathname is, of course, up
to the receiver, which might have various options for dealing with incoming
pathnames.

When RECEIVING a file, G-Kermit does NOT strip the pathname, since incoming
files normally do not include a pathname unless you told your terminal to
include them or gave an "as-name" including a path when sending to G-Kermit.
If the incoming filename includes a path, G-Kermit tries to store the file
in the specified place.  If the path does not exist, the transfer fails.
The incoming filename can, of course, be superseded with the -a option.


8. FILENAME CONVERSION

When sending a file, G-Kermit normally converts outbound filenames to
common form: uppercase, no more than one period, and no funny characters.
So, for example, gkermit.tar.gz would be sent as GKERMIT_TAR.GZ.

When receiving a file, if the name is all uppercase, G-Kermit converts it
to all lowercase.  If the name contains any lowercase letters, G-Kermit
leaves the name alone.  Otherwise G-Kermit accepts filename characters as
they are, since Unix allows filenames to contain practically any characters.

If the automatic peer recognition feature is available in the terminal
emulator, and G-Kermit recognizes the emulator's platform as Unix, G-Kermit
automatically disables filename conversion and sends and accepts filenames
literally.

You can force literal filenames by including the -P option on the command
line.


9. FILENAME COLLISIONS

When G-Kermit receives a file whose name is the same as that of an existing
file, G-Kermit backs up the existing file by adding a unique suffix to its
name.  The suffix is ".~n~", where n is a number between 1 and 999.  This
the same kind of backup suffix used by GNU EMACS and C-Kermit (both of which
can be used to prune excess backup files).  But since G-Kermit does not read
directories (see Implementation Notes below), it can not guarantee that the
number chosen will be higher than any other backup prefix number for the
same file.  In fact, the first free number, starting from 1, is chosen.  If
an incoming file already has a backup suffix, G-Kermit strips it before
adding a new one, rather than creating a file that has two backup suffixes.

To defeat the backup feature and have incoming files overwrite existing
files of the same name, include the -w (writeover) option on the command
line.

If G-Kermit has not been been given the -w option and it fails to create a
backup file, the transfer fails.


10. KERMIT PROTOCOL DETAILS

Block check
  G-Kermit uses the 3-byte, 16-bit CRC by default.  If the other Kermit
  does not agree, both Kermits automatically drop down to the single-byte
  6-bit checksum that is required of all Kermit implementations.

Attributes
  When sending files, G-Kermit conveys the file transfer mode and file
  size in bytes to the receiver in an Attribute (A) packet if the use of
  A-packets was negotiated.  This allows the receiver to switch to the
  appropriate mode automatically, and to display the percent done, estimated
  time left, and/or a thermometer bar if it has that capability.  When
  receiving, G-Kermit looks in the incoming A-packet, if any, for the
  transfer mode (text or binary) and switches itself accordingly on a
  per-file basis.

Handling of the Eighth Bit
  G-Kermit normally treats the 8th bit of each byte as a normal data bit.
  But if you have a 7-bit connection, transfers of 8-bit files fail unless
  you tell one or both Kermits to use the appropriate kind of parity, in
  which case Kermit uses single-shift escaping for 8-bit bytes.  Generally,
  telling either Kermit is sufficient; it tells the other.  Use the -p
  option to tell G-Kermit which parity to use.  Locking shifts are not
  included in G-Kermit.

Control-Character Encoding
  G-Kermit escapes all control characters when sending (for example,
  Ctrl-A becomes #A).  When receiving, it accepts both escaped and bare
  control characters, including NUL (0).  However, unescaped control
  characters always present a danger, so if uploads to G-Kermit fail, tell
  your terminal emulator's Kermit to escape most or all control characters
  (in C-Kermit and Kermit 95 the command is SET PREFIXING CAUTIOUS or SET
  PREFIXING ALL).

Packet Length
  All legal packet lengths, 40-9020, are supported although a lower
  maximum might be imposed on platforms where it is known that bigger ones
  don't work.  When receiving, G-Kermit sends its receive packet length to
  the sender, and the sender must not send packets any longer than this
  length.  The default length for most platforms is 4000 and it may be
  overridden with the -e command-line option.

Sliding Windows
  G-Kermit does not support sliding windows.  Streaming is used instead.
  If the other Kermit bids to use sliding windows, G-Kermit declines.

Streaming
  If the terminal emulator's Kermit informs G-Kermit that it has a
  reliable connection (such as TCP/IP or X.25), and the emulator's Kermit
  supports streaming, then a special form of the Kermit protocol is used
  in which data packets are not acknowledged; this allows the sender to
  transmit a steady stream of (framed and checksummed) data to the
  receiver without waiting for acknowledgements, allowing the fastest
  possible transfers.  Streaming overcomes such obstacles as long round
  trip delays, unnecessary retransmissions on slow network connections,
  and most especially the TCP/IP Nagle and Delayed ACK heuristics which
  are deadly to a higher-level ACK/NAK protocol.  When streaming is in use
  on a particular connection, Kermit speeds are comparable to FTP.  The
  drawback of streaming is that transmission errors are fatal; that's why
  streaming is only used on reliable connections, which, by definition,
  guarantee there will be no transmission errors.  However, watch out for
  the relatively rare circumstance in which emulator thinks it has a
  reliable connection when it doesn't -- for example a Telnet connection
  to a terminal server, and a dialout from the terminal server to the
  host.  Use the -S option on the command line to defeat streaming in such
  situations.

Using all defaults on a TCP/IP connection on 10BaseT (10Mbps) Ethernet from
a modern Kermit program like C-Kermit 7.0 or Kermit 95, typical transfer
rates are 150-500Kcps.


11. PROBLEMS, BUGS, ERRORS

If file transfers fail:

 . Make sure your terminal emulator is not unprefixing control characters;
   various control characters might cause trouble along the communication
   path.  When in doubt, instruct the file sender to prefix all control
   characters.

 . Make sure your Unix terminal is conditioned for the appropriate kind
   of flow control.

 . Use command-line options to back off on performance and transparency;
   use -S to disable streaming, -e to select a shorter packet length, -p
   to select space or other parity, -b to increase or disable the timeout,
   and/or establish the corresponding settings on your emulator.

When receiving files in text mode, G-Kermit strips all carriage returns,
even if they aren't part of a CRLF pair.

If you have a TCP/IP connection (e.g. Telnet or Rlogin) to Unix from a
terminal emulator whose Kermit protocol does not support streaming,
downloads from G-Kermit are likely to be as much as 10 or even 100 times
slower than uploads if the TCP/IP stack engages in Nagle or Delayed ACK
heuristics; typically, when your terminal emulator's Kermit protocol sends
an acknowledgment, the TCP stack holds on to it for (say) 1/5 second before
sending it, because it is "too small" to send right away.

As noted in Section 9, the backup prefix is not guaranteed to be the highest
number.  For example, if you have files oofa.txt, oofa.txt.~1~, and
oofa.txt.~3~ in your directory, and a new oofa.txt file arrives, the old
oofa.txt is backed up to oofa.txt.~2~, rather than oofa.txt.~4~ as you might
expect.  This is because gkermit lacks directory reading capabilities, for
reasons noted in Section 14, and without this, finding the highest existing
backup number for a file is impractical.

If you send a file to G-Kermit with streaming active when the connection is
not truly reliable, all bets are off.  A fatal error should occur promptly,
but if huge amounts of data are lost, G-Kermit might never recognize a single
data packet and therefore not diagnose a single error; yet your terminal
emulator keeps sending packets since no acknowledgments are expected; the
transfer eventually hangs at the end of file.  Use -S on G-Kermit's command
line to disable streaming in situations where the terminal emulator requests
it in error.

You can use G-Kermit's debug log for troubleshooting; this is useful mainly
in conjunction with the source code.  But even if you aren't a C programmer,
it should reveal any problem in enough detail to help pinpoint the cause of
the failure.  "gkermit -d" (with no action options) writes a short debug.log
file that shows the build options and settings.

The debug log is also a packet log; to extract the packets from it, use:

  grep ^PKT debug.log

Packets in the log are truncated to avoid wrap-around on your screen, and
they have the Ctrl-A packet-start converted to ^ and A to avoid triggering
a spurious autodownload when displaying the log on your screen.

In certain circumstances it is not desirable or possible to use -d to create
a log file called debug.log in the current directory; for example, if you
don't have write access to the current directory, or you already have a
debug.log file that you want to keep (or transfer).  In this case, you can
include a filename argument after -d:

  gkermit -d /tmp/testing.log -s *.c

(This is an exception to the rule that option arguments are not optional.)

If all else fails, you can contact the Kermit Project for technical support;
see:

  http://www.columbia.edu/kermit/support

for instructions.


12. BUILDING G-KERMIT

G-Kermit is written to require the absolute bare minimum in system services
and C-language features and libraries, and therefore should be portable to
practically any Unix platform at all with any C compiler.

The source files are:

  makefile   The build procedure
  gwart.c    Source code for a mini-lex substitute
  gproto.w   G-Kermit protocol state machine to be preprocessed by gwart
  gkermit.h  G-Kermit header file
  gkermit.c  G-Kermit main module and routines
  gcmdline.c G-Kermit command-line parser
  gunixio.c  Unix-specific i/o routines

A simple makefile is provided, which can be used with make or gmake.  There
are three main targets in the makefile:

  posix
    Build for any POSIX.1 compliant platform (termios).  This is the
    default target, used if you type "make" (or "gmake") alone.  This
    target works for most modern Unixes, including GNU/Linux, FreeBSD,
    OpenBSD, NetBSD, BSDI, HP-UX, Solaris, SunOS, Unixware, AIX, etc.

  sysv
    Build for almost any AT&T System V platform (termio).  Examples
    include AT&T Unix releases, e.g. for the AT&T 7300, HP-UX versions
    prior to 7.00.

  bsd
    Build for any BSD (pre-4.4) or Unix V7 platform (sgtty).  Examples
    include NeXTSTEP 3.x, OSF/1, and 4.3BSD or earlier.

Note that the target names are all lowercase; "posix" is the default target
(the one used if you just type "make").  If the build fails with a message
like:

  gunixio.c: 65: Can't find include file termios.h
  *** Error code 1

then try "make sysv" or "make bsd".  See the build list below for examples.

Some special build targets are also provided:

  sysvx
    Like sysv but uses getchar()/putchar() for packet i/o rather than
    buffered nonblocking read()/write(); this is necessary for certain
    very old System V platforms (see description of USE_GETCHAR below).

  stty
    When none of the other targets compiles successfully, try this one,
    which runs the external stty program rather than trying to use
    API calls to get/set terminal modes (system("stty raw -echo") and
    system("stty -raw echo")).

Several maintenance/management targets are also included:

  clean
    Remove object and intermediate files.

  install
    Install gkermit (read the makefile before using this).

  uninstall
    Uninstall gkermit from wherever "make install" installed it.

The default compiler is cc.  To override (e.g. to force the use of gcc on
computers that have both cc and gcc installed, or that don't have cc), use:

  [g]make CC=gcc [<target>]

No other tools beyond make, the C compiler and linker, a short list of
invariant header files, and the standard C library are needed or used.  The
resulting binary should be 100K or less on all hardware platforms (and 64K
or less on most; see list below).

You may also specify certain build options by including a KFLAGS clause on
the make command line, e.g.:

  make "KFLAGS=-DSETXONXOFF -DEXTRADEBUG" sysv

By default, nonblocking buffered read() is used for packets; this technique
works on most platforms but other options -- USE_GETCHAR and DUMBIO -- are
provided when it doesn't work or when nonblocking i/o is not available.

The build options include:

__STDC__
  Include this when the compiler requires ANSI prototyping but does
  does not define __STDC__ itself.  Conversely, you might need to
  include -U__STDC__ if the compiler defines __STDC__ but does not
  support minimum ANSI features.

ULONG=long
  Include this if compilation fails with "unknown type: unsigned long".

CHAR=char
  Include this if compilation fails with "unknown type: unsigned char".

SMALL
  Define this when building on or for a "small" platform, for example
  a 16-bit architecture.

USE_GETCHAR
  Specifies that packet i/o should be done with (buffered) getchar()
  and putchar() rather than the default method of nonblocking,
  internally buffered read() and write().  Use this only when G-Kermit
  does not build or run otherwise, since if the default i/o code is
  not used, G-Kermit won't be able to do streaming.

DUMBIO
  Specifies that packet i/o should be done with blocking single-byte
  read() and write().  Use this only when G-Kermit doesn't build or
  run, even with USE_GETCHAR.

MAXRP=nnn
  Change the maximum receive-packet length to something other than the
  default, which is 9020.  You should change this only to make it smaller;
  making it bigger is not supported by the Kermit protocol.

DEFRP=nnn
  Change the default receive packet length to something other than the
  default, which is 4000.  Making it any bigger than this is not advised.

TINBUFSIZ=nnn
  On builds that use nonblocking buffered read(), override the default
  input buffer size of 4080.

SETXONXOFF
  On some platforms, mainly those based on System V R4 and earlier, it was
  found that receiving files was impossible on TCP/IP connections unless
  the terminal driver was told to use Xon/Xoff flow control.  If downloads
  work but uploads consistently fail (or fail consistently whenever
  streaming is used or the packet length is greater than a certain number
  like 100, or 775), try adding this option.  When gkermit is built with
  this option, it is equivalent to the user always giving the -x option on
  the command line.  (Most versions of HP-UX need this; it is defined
  automatically at compile time if __hpux is defined.)

SIG_V
  The data type of signal handlers is void.  This is set automatically
  for System V and POSIX builds.

SIG_I
  The data type of signal handlers is int.  This is set automatically
  for BSD builds.

NOGETENV
  Add this to disable the feature in which G-Kermit gets options from
  the GKERMIT environment variable.

NOSTREAMING
  Add this to disable streaming.

EXTRADEBUG
  This adds a lot (a LOT) of extra information to the debug log
  regarding packet and character-level i/o.

FULLPACKETS
  Show full packets in the debug log rather than truncating them.

Any compiler warnings should be harmless.  Examples include:

"Passing arg 2 of `signal' from incompatible pointer"
(or "Argument incompatible with prototype"):
  Because no two Unix platforms agree about signal handlers.  Harmless
  because the signal handler does not return a value that is used.  We
  don't want to open the door to platform-specific #ifdefs just to
  silence this warning.  However, you can include -DSIG_I or -DSIG_V
  on the CC command line to override the default definitions.

"<blah> declared but never used":
  Some function parameters are not used because they are just placeholders
  or compatibility items, or even required by prototypes in system headers.
  Others might be declared in system header files (like mknod, lstat, etc,
  which are not used by G-Kermit).

"Do you mean equality?":
  No, in "while (c = *s++)" the assignment really is intentional.

"Condition is always true":
  Yes, "while (1)" is always true.

"Flow between cases":
  Intentional.

"No flow into statement":
  In gproto.c, because it is a case statement generated by machine,
  not written by a human.

The coding conventions are aimed at maximum portability.  For example:
 . Only relatively short identifiers.
 . No long character-string constants.
 . Only #ifdef, #else, #endif, #define, and #undef preprocessor directives.
 . Any code that uses ANSI features is enclosed in #ifdef __STDC__..#endif.
 . No gmake-specific constructs in the makefile.

Here are some sample builds (December 1999):

 Platform                         Size  Target    Notes
  Apple Mac OS X 1.0 gcc:          48K   posix    (AKA Rhapsody 5.5)
  AT&T 3B2/300 SVR2 cc:            49K   sysv     (4)
  AT&T 6300 PLUS cc:               58K   sysv     (6)
  AT&T 7300 UNIX PC cc:            40K   sysv
  AT&T 7300 UNIX PC gcc:           55K   sysv     (23K with shared lib)
  BSDI 4.0.1 gcc:                  34K   posix
  DEC 5000 MIPS Ultrix 4.3 cc:     99K   posix
  DEC Alpha Digital UNIX 3.2 cc:   98K   bsd      (AKA OSF/1) (1)
  DEC Alpha Tru64 UNIX 4.0e cc:    82K   bsd      (1)
  DEC PDP-11 2.11BSD cc:           40K   bsd211   (7)
  DG/UX 5.4R3.10 cc:               52K   posix
  DG/UX 5.4R4.11 gcc:              51K   posix
  DYNIX/ptx 4.4.2 cc:              43K   posix
  FreeBSD 2.2.7 gcc:               41K   posix
  FreeBSD 3.3 gcc:                 34K   posix
  GNU/Linux RH 5.2 gcc:            35K   posix    (RH = Red Hat)
  GNU/Linux RH 6.1 gcc:            44K   posix
  GNU/Linux SW 3.5 gcc:            34K   posix    (SW = Slackware)
  GNU/Linux SW 4.0 gcc:            36K   posix
  GNU/Linux SW 7.0 gcc:            44K   posix
  HP-UX 5.21 cc:                   55K   sysv     (2)
  HP-UX 6.5 cc:                    40K   sysv     (5)
  HP-UX 7.05 cc:                   50K   posix
  HP-UX 8.00 gcc:                  33K   posix
  HP-UX 9.05 cc:                   57K   posix
  HP-UX 10.01 cc:                  57K   posix
  HP-UX 10.20 cc:                  61K   posix
  IBM AIX 3.2 IBM cc:              62K   posix
  IBM AIX 4.1.1 IBM cc:            67K   posix
  IBM AIX 4.3.2 IBM cc:            69K   posix
  Motorola 88K SV/88 R4.3          42K   posix
  Motorola 68K SV/68 R3.6          56K   sysv     (4)
  NetBSD 1.4.1 gcc:                41K   posix
  NeXTSTEP m68k 3.1 gcc:           77K   bsd      (3)
  NeXTSTEP m68k 3.3 gcc:           78K   bsd      (3)
  OpenBSD 2.5 gcc:                 47K   posix
  QNX 4.25 cc:                     33K   posix
  SCO XENIX 2.3.4 cc:              41K   sysv     (4)
  SCO UNIX 3.2v4.2 cc:             73K   posix
  SCO UNIX 3.2v4.2 gcc:            61K   posix
  SCO ODT 3.0 cc:                  97K   posix
  SCO OSR5.0.5 gcc:                42K   posix
  SCO Unixware 2.1.3 cc:           38K   posix
  SCO Unixware 7.0.1 cc:           37K   posix
  SGI IRIX 5.3 cc:                 86K   posix
  SGI IRIX 6.5.4 cc:               91K   posix
  SINIX 5.42 MIPS cc:              57K   posix
  Solaris 2.4 cc:                  50K   posix
  Solaris 2.5.1 cc:                51K   posix
  Solaris 2.6 cc:                  52K   posix
  Solaris 7 cc:                    52K   posix
  SunOS 4.1.3 cc:                  57K   posix
  SunOS 4.1.3 gcc:                 64K   posix

Notes:

(1) "make posix" builds without complaint on OSF/1 (Digital UNIX (Tru64))
    but it doesn't work -- i/o hangs or program dumps core.  "make bsd"
    works fine.

(2) POSIX APIs not available in this antique OS (circa 1983).  Also due
    to limited terminal input buffering capacity, streaming must be
    disabled and relatively short packets must be used when receiving:
    "gkermit -Se 250 -r".  However, it can use streaming when sending.

(3) POSIX APIs not available.

(4) On System V R3 and earlier, EWOULDBLOCK is not defined, so we use EGAIN
    instead.  No special build procedures needed.

(5) Built with 'make -i "KFLAGS=-DDEFRP=512 -DUSE_GETCHAR" sysv'.  It can
    be built without -DUSE_GETCHAR but doesn't work.

(6) Use 'make "CC=cc -Ml" "KFLAGS=-DUSE_GETCHAR sysv'.  It builds but
    doesn't work, reason unknown, but probably because it was never designed
    to be accessed remotely in the first place.

(7) This is a 16-bit architecture.  A special makefile target is needed
    because its make program does not expand the $(CC) value when invoking
    second-level makes.  Packet and buffer sizes are reduced to keep
    static data within limits.  Overlays are not needed.


13. INSTALLING G-KERMIT

The makefile creates a binary called "gkermit".  Simply move this binary to
the desired directory, such as /usr/local/bin.  It needs no special
permissions other than read, write, and execute for the desired users and
groups: no setuid, no setgid, or any other form of privilege.  It should be
called "gkermit" and not "kermit", since "kermit" is the binary name for
C-Kermit, and the two are likely to be installed side by side on the same
computer; even when they are not, consistent naming is better for support
and sanity purposes.  There is also a short man page:

  gkermit.nr

You can view it with:

  nroff -man gkermit.nr | more

Rename and store it appropriately.  In addition, this file itself (README)
should be made available in a public documentation directory as:

  gkermit.txt

The makefile includes a SAMPLE 'install' target that does all this.  Please
read it before use to be sure the appropriate directories and permissions
are indicated.  There is also an 'uninstall' target to undo an installation.
Obviously you need write access to the relevant directories before you can
install or uninstall G-Kermit.


14. DESIGN AND IMPLEMENTATION NOTES

A primary objective in developing G-Kermit is that it can be released and
used forever without constant updates to account for platform idiosyncracies
and changes.  For this reason, certain features have been deliberately
omitted:

 . File timestamps.  The methods for dealing with internal time formats
   are notoriously unportable and also a moving target, especially now
   with the 32-bit internal time format rollover looming in 2038 and the
   time_t data type changing out from under us.  Furthermore, by excluding
   any date-handling code, G-Kermit is automatically Y2K, 2038, and Y10K
   compliant.

 . Internal wildcard expansion, recursive directory traversal, etc.  Even
   after more than 30 years, there is still no standard and portable
   service in Unix for this.

 . Hardware flow control, millisecond sleeps, nondestructive input buffer
   peeking, threads, select(), file permissions, etc etc.

Other features are omitted to keep the program small and simple, and to
avoid creeping featurism:

 . Sliding windows.  This technique is more complicated than streaming but
   not as fast, and furthermore would increase the program size by a
   factor of 5 or 10 due to buffering requirements.

 . An interactive command parser and scripting language (because users
   always want more and more commands and features).

 . Character set conversion (because users always want more and more
   character sets).  Adding character set support would increase the
   program size by a factor of 2 to 4, depending on the selection of sets.

 . Making connections (because this requires huge amounts of tricky and
   unstable high-maintenance platform- and device-specific code for serial
   ports, modems, modem signals, network stacks, etc).

All of the above can be found in C-Kermit, which is therefore bigger and
more complicated, with more platform-specific code and #ifdef spaghetti.
C-Kermit requires constant updates and patches to keep pace with changes in
the underlying platforms, networking methods, and demands from its users for
more features.

The goal for G-Kermit, on the other hand, is simplicity and stability, so we
don't need thousands of #ifdefs like we have in C-Kermit, and we don't need
to tweak the code every time a new release of each Unix variety comes out.
G-Kermit is meant to be PORTABLE and LONG-LASTING so the stress is on a
MINIMUM of platform dependencies.

If you make changes, please try to avoid adding anything platform-dependent
or in any other way destabilizing.  Bear in mind that the result of your
changes should still build and run successfully on at least all the
platforms where it was built originally.  In any case, you are encouraged to
send any changes back to the Kermit Project to be considered for addition to
the master G-Kermit distribution.


15. FURTHER INFORMATION

The Kermit protocol is specified in "Kermit, A File Transfer Protocol" by
Frank da Cruz, Digital Press (1987).  A correctness proof of the Kermit
protocol appears in "Specification and Validation Methods", edited by Egon
Boerger, Oxford University Press (1995).  "Using C-Kermit" by Frank da Cruz
and Christine M. Gianone, Digital Press (1997, or later edition) explains
many of the terms and techniques referenced in this document in case you are
not familiar with them, and also includes tutorials on data communications,
extensive troubleshooting and performance tips, etc.  Various other books on
Kermit are available from Digital Press.  Online resources include:

  http://www.columbia.edu/kermit/    The Kermit Project website
  comp.protocols.kermit.misc         The unmoderated Kermit newsgroup
  kermit-support@columbia.edu        Technical support

(End of G-Kermit README)
