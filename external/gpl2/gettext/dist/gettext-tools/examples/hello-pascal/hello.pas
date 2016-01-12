{ Example for use of GNU gettext.
  Copyright (C) 2003 Free Software Foundation, Inc.
  This file is in the public domain.

  Source code of the Pascal program.  }

program hello;
{$mode delphi}

uses gettext,  { translateresourcestrings }
     linux,    { getpid }
     sysutils; { format }

resourcestring
  hello_world = 'Hello, world!';
  running_as = 'This program is running as process number %d.';

begin
  translateresourcestrings({$i %LOCALEDIR%}+'/%s/LC_MESSAGES/hello-pascal.mo');
  writeln(hello_world);
  writeln(format(running_as,[getpid]));
end.
