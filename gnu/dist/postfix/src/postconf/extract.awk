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

/^(static| )*CONFIG_INT_TABLE .*\{/,/\};/ { 
    if ($1 ~ /VAR/) {
	print "int " substr($3,2,length($3)-2) ";" > "int_vars.h"
	if (++itab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    print |"sed 's/[ 	][ 	]*/ /g' > int_table.h"
	}
    }
}
/^(static| )*CONFIG_STR_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	print "char *" substr($3,2,length($3)-2) ";" > "str_vars.h"
	if (++stab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    print |"sed 's/[ 	][ 	]*/ /g' > str_table.h"
	}
    }
}
/^(static| )*CONFIG_RAW_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	print "char *" substr($3,2,length($3)-2) ";" > "raw_vars.h"
	if (++rtab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    print |"sed 's/[ 	][ 	]*/ /g' > raw_table.h"
	}
    }
}
/^(static| )*CONFIG_BOOL_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	print "int " substr($3,2,length($3)-2) ";" > "bool_vars.h"
	if (++btab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    print |"sed 's/[ 	][ 	]*/ /g' > bool_table.h"
	}
    }
}
/^(static| )*CONFIG_TIME_TABLE .*\{/,/\};/ { 
    if ($1 ~ /^VAR/) {
	print "int " substr($3,2,length($3)-2) ";" > "time_vars.h"
	if (++ttab[$1 $2 $4 $5 $6 $7 $8 $9] == 1) {
	    print |"sed 's/[ 	][ 	]*/ /g' > time_table.h" 
	}
    }
}

# Workaround for broken gawk versions.

END { exit(0); }
