// Splice string `s` into the format string `fmtstring` replacing the
// %-parameter at position `pos`
@initialize:python@
@@

# regex from https://stackoverflow.com/questions/30011379/how-can-i-parse-a-c-format-string-in-python
import re
fmtstring = '''\
(                                  # start of capture group 1
%                                  # literal "%"
(?:                                # first option
(?:[-+0 #]{0,5})                   # optional flags
(?:\d+|\*)?                        # width
(?:\.(?:\d+|\*))?                  # precision
(?:h|l|ll|w|I|I32|I64)?            # size
[cCdiouxXeEfgGaAnpsSZ]             # type
) |                                # OR
%%)                                # literal "%%"
'''

regex = re.compile(fmtstring, re.X)

def parse_format(f):
    return tuple((m.span(), m.group()) for m in
        regex.finditer(f))

def insert_at_pos(fmt, s, pos):
    formats = parse_format(fmt)
    span, format = formats[pos]
    acc = fmt[:span[0]]
    if s.startswith('"'):
        acc += s[1:]
    else:
        acc += '" '
        acc += s
    if acc.endswith('"'):
        acc = acc[:-1] + fmt[span[1]:]
    else:
        acc += ' "'
        acc += fmt[span[1]:]
    return acc

// rest of the file implements the same as 09-merge.cocci
// The main difference is that we only match on snprintf and Debug that are
// directly adjacent, not based on control flow information which trips
// coccinelle's model-checker
@shortcut@
identifier buf;
expression E, L;
expression list args_before, args, args_after;
expression format1, format2;
position p1, p2;
@@

snprintf@p1( buf, E, format1, args );
Debug@p2( L, format2, args_before, buf, args_after );

// use insert_at_pos above to construct the new format-string
@script:python shortcut_process@
format1 << shortcut.format1;
format2 << shortcut.format2;
args_before << shortcut.args_before;
merged;
@@

pos = len(args_before.elements)
coccinelle.merged = insert_at_pos(format2, format1, pos)

@shortcut_replace@
position shortcut.p1, shortcut.p2;
identifier shortcut_process.merged;

identifier buf;
expression E, L;
expression list args_before, args, args_after;
expression format1, format2;
@@

-snprintf@p1( buf, E, format1, args );
-Debug@p2( L, format2, args_before, buf, args_after );
+Debug( L, merged, args_before, args, args_after );

@shortcut_locked@
identifier buf;
expression E, L, lock;
expression list args_before, args, args_after;
expression format1, format2;
position p1, p2;
@@

ldap_pvt_thread_mutex_lock(lock);
snprintf@p1( buf, E, format1, args );
ldap_pvt_thread_mutex_unlock(lock);
Debug@p2( L, format2, args_before, buf, args_after );

// use insert_at_pos above to construct the new format-string
@script:python shortcut_locked_process@
format1 << shortcut_locked.format1;
format2 << shortcut_locked.format2;
args_before << shortcut_locked.args_before;
merged;
@@

pos = len(args_before.elements)
coccinelle.merged = insert_at_pos(format2, format1, pos)

@shortcut_locked_replace@
position shortcut_locked.p1, shortcut_locked.p2;
identifier shortcut_locked_process.merged;

identifier buf;
expression E, L, lock;
expression list args_before, args, args_after;
expression format1, format2;
@@

ldap_pvt_thread_mutex_lock(lock);
-snprintf@p1( buf, E, format1, args );
+Debug( L, merged, args_before, args, args_after );
ldap_pvt_thread_mutex_unlock(lock);
-Debug@p2( L, format2, args_before, buf, args_after );

// so long as we don't reference 'buf' afterwards, no need to keep it defined.
// A lot of pattern-matching is spelled out explicitly to work around the fact
// that the state space doesn't get compressed otherwise.
@@
type T;
identifier buf, id;
expression E, lock;
initializer I;
@@
{
-\( T buf = I; \| T buf; \)
(
 ldap_pvt_thread_mutex_lock(lock);
|
)
(
 Debug( ... );
&
 ... when != buf
)
(
 ldap_pvt_thread_mutex_unlock(lock);
|
)
(
|
 continue;
|
 break;
|
 goto id;
|
 \(
  return E;
 \&
  ... when != buf
 \)
)
}

// the rest identifies and removes a (newly-)redundant LogTest check
@if_guard@
position p;
statement s;
@@

(
 if ( ... ) {@p 
  Debug( ... );
 } else s
|
 if ( ... ) {@p
  Debug( ... );
 }
)

@else_guard@
position p;
statement s;
@@

if ( ... ) s
else {@p
 Debug( ... );
}

@loop_guard@
position p;
@@

(
 while ( ... ) {@p
  Debug( ... );
 }
|
 for ( ...;...;... ) {@p
  Debug( ... );
 }
)

@@
position p != { if_guard.p , else_guard.p, loop_guard.p };
@@
-{@p
 Debug( ... );
-}

@useless_if@
expression L;
@@

-if ( LogTest( L ) ) {
 Debug( L, ... );
-}
