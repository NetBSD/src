# Extract initialization tables from actual source code.

/^(static| )*CONFIG_INT_TABLE .*\{/,/\};/ { 
    if ($1 ~ /VAR/) {
	print "int " substr($3,2,length($3)-2) ";" > "int_vars.h"
	print | "sed 's/[ 	][ 	]*/ /g' | sort -u >int_table.h" 
    }
}
/^(static| )*CONFIG_STR_TABLE .*\{/,/\};/ { 
    if ($1 ~ /VAR/) {
	print "char *" substr($3,2,length($3)-2) ";" > "str_vars.h"
	print | "sed 's/[ 	][ 	]*/ /g' | sort -u >str_table.h" 
    }
}
/^(static| )*CONFIG_BOOL_TABLE .*\{/,/\};/ { 
    if ($1 ~ /VAR/) {
	print "int " substr($3,2,length($3)-2) ";" > "bool_vars.h"
	print | "sed 's/[ 	][ 	]*/ /g' | sort -u >bool_table.h" 
    }
}

# Workaround for broken gawk versions.

END { exit(0); }
