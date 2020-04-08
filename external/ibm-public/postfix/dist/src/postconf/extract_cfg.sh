#!/bin/sh

# To view the formatted manual page of this file, type:
#	POSTFIXSOURCE/mantools/srctoman - extract_cfg.sh | nroff -man

#++
# NAME
#	extract_cfg 1
# SUMMARY
#	extract database parameter names from cfg_get_xxx() calls
# SYNOPSIS
#	\fBextract_cfg [-d|-s] [\fIfile...\fB]\fR
# DESCRIPTION
#	The \fBextract_cfg\fR command extracts the parameter names
#	from cfg_get_{str,int,bool}() calls in dict_xxx.c files. The
#	output is one parameter name per line, formatted as a C string
#	followed by comma.
#
#	Options:
# .IP \fB-d\fR
#	Add the "domain" parameter to the output. This is used by
#	the LDAP, memcache, and *SQL* tables.
# .IP \fB-s\fR
#	Add the legacy SQL query parameters: "select_field", "table",
#	"where_field", and "additional_conditions".
# LICENSE
# .ad
# .fi
#	The Secure Mailer license must be distributed with this software.
# HISTORY
# .ad
# .fi
#	This command was introduced with Postfix 3.3.
# AUTHOR(S)
#	Wietse Venema
#	Google, Inc.
#	111 8th Avenue
#	New York, NY 10011, USA
#--

# In case not installed.
m4 </dev/null || exit 1

# Flags to add db_common parameter names.
add_legacy_sql_query_params=
add_domain_param=

# Parse JCL.

while :
do
    case "$1" in
    -d) add_domain_param=1;;
    -s) add_legacy_sql_query_params=1;;
    -*) echo Bad option: $1 1>&2; exit 1;;
     *) break;;
    esac
    shift
done

# We use m4 macros to extract arguments from cfg_get_xxx() calls that
# may span multiple lines. We sandwich information of interest between
# control-A characters. Multiple cfg_get_xxx() calls on the same line
# should be OK, as long as the calls don't nest.

(
cat <<'EOF'
define(`cfg_get_str',`$2
')dnl
define(`cfg_get_int',`$2
')dnl
define(`cfg_get_bool',`$2
')dnl
EOF
# Convert selected C macro definitions into m4 macro definitions.
sed 's/^#define[ 	]*\([DICT_MC_NAME_A-Za-z0-9_]*\)[ 	]*\("[^"]*"\)/define(`\1'"'"',`\2'"'"')/' "$@"
) | m4 | awk -F '// { print $2 }' | (
test -n "$add_domain_param" && {
cat <<EOF
"domain"
EOF
}
test -n "$add_legacy_sql_query_params" && {
cat <<EOF
"table"
"select_field"
"where_field"
"additional_conditions"
EOF
}
cat -
) | sort -u | sed 's/$/,/'
