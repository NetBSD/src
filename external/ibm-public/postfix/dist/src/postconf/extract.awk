# Extract initialization tables from actual source code.

# XXX: Associated variable aliasing:
#
# Some parameters bind to different variables in different contexts,
# And other parameters map to associated variables in a many-to-1
# fashion. This is mostly the result of the SMTP+LMTP integration
# and the overloading of parameters that have identical semantics,
# for the corresponding context.
#
# The "++table[...]" below ignores the associated variable name
# when doing duplicate elimination. Differences in the default value
# or lower/upper bounds still result in "postconf -d" duplicates,
# which are a sign of an error somewhere...
#
# XXX Work around ancient AWK implementations with a 10 file limit
# and no working close() operator (e.g. Solaris). Some systems
# have a more modern implementation that is XPG4-compatible, but it
# is too much bother to find out where each system keeps these.

/^(static| )*(const +)?CONFIG_INT_TABLE .*\{/,/\};/ { 
    if ($1 ~ /VAR/) {
	int_vars["int " substr($3,2,length($3)-2) ";"] = 1
	if (++itab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    int_table[$0] = 1
	}
    }
}
/^(static| )*(const +)?CONFIG_STR_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	str_vars["char *" substr($3,2,length($3)-2) ";"] = 1
	if (++stab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    str_table[$0] = 1
	}
    }
}
/^(static| )*(const +)?CONFIG_RAW_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	raw_vars["char *" substr($3,2,length($3)-2) ";"] = 1
	if (++rtab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    raw_table[$0] = 1
	}
    }
}
/^(static| )*(const +)?CONFIG_BOOL_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	bool_vars["int " substr($3,2,length($3)-2) ";"] = 1
	if (++btab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    bool_table[$0] = 1
	}
    }
}
/^(static| )*(const +)?CONFIG_TIME_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	time_vars["int " substr($3,2,length($3)-2) ";"] = 1
	if (++ttab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    time_table[$0] = 1
	}
    }
}
/^(static| )*(const +)?CONFIG_NINT_TABLE .*\{/,/\};/ { 
    if ($1 ~ /VAR/) {
	nint_vars["int " substr($3,2,length($3)-2) ";"] = 1
	if (++itab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    nint_table[$0] = 1
	}
    }
}

END { 
    # Print parameter declarations without busting old AWK's file limit.
    print "cat >int_vars.h <<'EOF'"
    for (key in int_vars)
	print key
    print "EOF"

    print "cat >str_vars.h <<'EOF'"
    for (key in str_vars)
	print key
    print "EOF"

    print "cat >raw_vars.h <<'EOF'"
    for (key in raw_vars)
	print key
    print "EOF"

    print "cat >bool_vars.h <<'EOF'"
    for (key in bool_vars)
	print key
    print "EOF"

    print "cat >time_vars.h <<'EOF'"
    for (key in time_vars)
	print key
    print "EOF"

    print "cat >nint_vars.h <<'EOF'"
    for (key in nint_vars)
	print key
    print "EOF"

    # Print parameter initializations without busting old AWK's file limit.
    print "sed 's/[ 	][ 	]*/ /g' >int_table.h <<'EOF'"
    for (key in int_table)
	print key
    print "EOF"

    print "sed 's/[ 	][ 	]*/ /g' >str_table.h <<'EOF'"
    for (key in str_table)
	print key
    print "EOF"

    print "sed 's/[ 	][ 	]*/ /g' >raw_table.h <<'EOF'"
    for (key in raw_table)
	print key
    print "EOF"

    print "sed 's/[ 	][ 	]*/ /g' >bool_table.h <<'EOF'"
    for (key in bool_table)
	print key
    print "EOF"

    print "sed 's/[ 	][ 	]*/ /g' >time_table.h <<'EOF'"
    for (key in time_table)
	print key
    print "EOF"

    print "sed 's/[ 	][ 	]*/ /g' >nint_table.h <<'EOF'"
    for (key in nint_table)
	print key
    print "EOF"

    # Flush output nicely.
    exit(0);
}
