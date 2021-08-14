@initialize:ocaml@
@@
// count the number of % characters in the format string
let fmtn(fmt,n) =
    List.length (Str.split_delim (Str.regexp_string "%") fmt) = n + 1

@@
identifier Logs =~ "Log[0-9]";
@@
-Logs
+Log

@@
@@
-StatslogTest
+LogTest

// Process two-argument Debug() macros with an extra zero
@@
char[] fmt : script:ocaml() { fmtn(fmt,2) };
expression list[2] args;
expression E;
@@

Debug( E, fmt, args
-, 0
 );

@@
char[] fmt : script:ocaml() { fmtn(fmt,2) };
expression list[2] args;
expression E;
@@

Debug( E, fmt, args
-, NULL
 );

// Single argument Debug() macros with two extra zeroes
@@
char[] fmt : script:ocaml() { fmtn(fmt,1) };
expression list[1] args;
expression E;
@@

Debug( E, fmt, args
-, 0, 0
 );

@@
char[] fmt : script:ocaml() { fmtn(fmt,1) };
expression list[1] args;
expression E;
@@

Debug( E, fmt, args
-, NULL, NULL
 );

// Debug() macros with no arguments just padded with zeroes
@@
expression E, S;
@@

Debug( E, S
-, 0, 0, 0
 );

@@
expression E, S;
@@

Debug( E, S
-, NULL, NULL, NULL
 );

// Similar to above, just for Statslog
@@
char[] fmt : script:ocaml() { fmtn(fmt,5) };
expression list[5] args;
expression E;
@@

-Statslog
+Debug
 ( E, fmt, args );

@@
char[] fmt : script:ocaml() { fmtn(fmt,4) };
expression list[4] args;
expression E;
@@

-Statslog
+Debug
 ( E, fmt, args
-, 0
 );

@@
char[] fmt : script:ocaml() { fmtn(fmt,3) };
expression list[3] args;
expression E;
@@

-Statslog
+Debug
 ( E, fmt, args
-, 0, 0
 );

@@
char[] fmt : script:ocaml() { fmtn(fmt,2) };
expression list[2] args;
expression E;
@@

-Statslog
+Debug
 ( E, fmt, args
-, 0, 0, 0
 );

@@
char[] fmt : script:ocaml() { fmtn(fmt,1) };
expression list[1] args;
expression E;
@@

-Statslog
+Debug
 ( E, fmt, args
-, 0, 0, 0, 0
 );

@@
expression E, S;
@@

-Statslog
+Debug
 ( E, S
-, 0, 0, 0, 0, 0
 );

// And StatslogEtime
@@
char[] fmt : script:ocaml() { fmtn(fmt,4) };
expression list[4] args;
expression E;
@@

StatslogEtime( E, fmt, args
-, 0
 );

@@
identifier Stats =~ "^Statslog";
@@
(
 StatslogEtime
|
-Stats
+Debug
)
