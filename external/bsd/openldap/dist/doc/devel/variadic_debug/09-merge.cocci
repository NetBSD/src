// Note that this file has not actually been used in the end, since
// 07-shortcut.cocci covers everything we needed in the project, but being
// simpler, it makes the intent of 07-shortcut.cocci clearer


// Splice string `s` into the format string `fmtstring` replacing the
// %-parameter at position `pos`
@initialize:python@
@@

#regex from https://stackoverflow.com/questions/30011379/how-can-i-parse-a-c-format-string-in-python
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

// Identify the redundant snprintfs (within a locked region)
@a exists@
expression lock, E, L;
expression list args_before, args, args_after;
identifier buf;
expression format1, format2;
type T;
position p1, p2;
@@

{
...
T buf;
...
ldap_pvt_thread_mutex_lock(lock);
...
snprintf@p1( buf, E, format1, args );
...
ldap_pvt_thread_mutex_unlock(lock);
...
Debug@p2( L, format2, args_before, buf, args_after );
...
}

// Merge the format strings with insert_at_pos above
@script:python a_process@
format1 << a.format1;
format2 << a.format2;
args_before << a.args_before;
merged;
@@

pos = len(args_before.elements)
coccinelle.merged = insert_at_pos(format2, format1, pos)

// And merge the two together, replacing the extra buffer that's not used anymore
@a_replace@
position a.p1, a.p2;
identifier a_process.merged;

expression lock, E, L;
expression list args_before, args, args_after;
identifier buf;
expression format1, format2;
type T;
@@

{
...
-T buf;
...
ldap_pvt_thread_mutex_lock(lock);
...
-snprintf@p1( buf, E, format1, args );
+Debug( L, merged, args_before, args, args_after );
...
ldap_pvt_thread_mutex_unlock(lock);
...
-Debug@p2( L, format2, args_before, buf, args_after );
...
}

// Once again (same as the 'a' series above, but those that remain to be sorted
// now don't need to stay within a locked region
@b exists@
expression E, L;
expression list args_before, args, args_after;
identifier buf;
expression format1, format2;
position p1, p2;
@@

snprintf@p1( buf, E, format1, args );
...
Debug@p2( L, format2, args_before, buf, args_after );

@script:python b_process@
format1 << b.format1;
format2 << b.format2;
args_before << b.args_before;
merged;
@@

pos = len(args_before.elements)
coccinelle.merged = insert_at_pos(format2, format1, pos)

@b_replace@
position b.p1, b.p2;
identifier b_process.merged;

expression E, L;
expression list args_before, args, args_after;
identifier buf;
expression format1, format2;
@@

-snprintf@p1( buf, E, format1, args );
+Debug( L, merged, args_before, args, args_after );
...
-Debug@p2( L, format2, args_before, buf, args_after );
