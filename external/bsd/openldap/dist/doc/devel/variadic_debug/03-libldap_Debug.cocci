using "equivalence.iso"

@initialize:ocaml@
@@
// count the number of % characters in the format string
let fmtn(fmt,n) =
    List.length (Str.split_delim (Str.regexp_string "%") fmt) = n + 1

# replace osip_debug/oslocal_debug with Debug() macros first
@@
expression E;
expression list args;
@@
(
-osip_debug
|
-oslocal_debug
)
+Debug
 (
-E,
+LDAP_DEBUG_TRACE,
 args );

// replace Debug( ..., arg1, arg2, 0 ) with Debug2( ..., arg1, arg2 )
@@
char[] fmt : script:ocaml() { fmtn(fmt,2) };
expression list[2] args;
expression E;
@@

-Debug
+Debug2
 ( E, _(fmt), args
-, 0
 );

// replace Debug( ..., arg1, 0, 0 ) with Debug1()
@@
char[] fmt : script:ocaml() { fmtn(fmt,1) };
expression list[1] args;
expression E;
@@

-Debug
+Debug1
 ( E, _(fmt), args
-, 0, 0
 );

// Zero-argument Debug() -> Debug0()
@@
expression E, S;
@@

-Debug
+Debug0
 ( E, S
-, 0, 0, 0
 );

// everything else is a regular 3-argument debug macro, replace with Debug3()
@@
expression E, S;
expression list[3] args;
@@

-Debug
+Debug3
 ( E, S, args );
