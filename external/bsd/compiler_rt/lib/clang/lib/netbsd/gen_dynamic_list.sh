#!/bin/sh
#===- lib/sanitizer_common/scripts/gen_dynamic_list.sh ---------------------===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
#
# Generates the list of functions that should be exported from sanitizer
# runtimes. The output format is recognized by --dynamic-list linker option.
# Usage:
#   gen_dynamic_list.sh libclang_rt.*san*.a [ files ... ]
#
#===------------------------------------------------------------------------===#

usage() {
	echo "usage: gen_dynamic_list.py [-h] [--version-list] [--extra EXTRA]"
	echo "                           libraries [libraries ...]"
}

get_global_functions() {
	# On PowerPC, nm prints function descriptors from .data section.
	$my_file $1 | grep -q "PowerPC"
	if [ $? -eq 0 ]; then
		export func_symbols="T|W|D"
	else
		export func_symbols="T|W"
	fi

	$my_nm $1 > $tmpfile1
	if [ $? -ne 0 ]; then
		printf 'Error calling: %s %s... bailing out\n' "$my_nm" "$1"
		exit 1
	fi

	$my_awk '{if(NF == 3 && $2 ~ ENVIRON["func_symbols"]){print $3}}' \
		$tmpfile1 >> $tmpfile2
}

version_list=
extra=

tmpfile1=`mktemp /tmp/${0##*/}.XXXXXX` || exit 1
tmpfile2=`mktemp /tmp/${0##*/}.XXXXXX` || exit 1

trap "rm -f $tmpfile1 $tmpfile2" EXIT

my_nm=${NM:-nm}
my_awk=${AWK:-awk}
my_file=${FILE:-file}

while :; do
	case $1 in
		-h|--help)
			usage
			exit
			;;
		--extra)
			if [ "x$2" = "x" ]; then
				usage
				printf 'Error missing argument for the option\n'
			else
				extra=$2
				shift
			fi
			;;
		--extra=?*)
			extra=${1#*=}
			;;
		--extra=)
			usage
			printf 'Error missing argument: %s\n' "$1"
			;;
		--version-list)
			version_list=1
			;;
		--)
			shift
			break
			;;
		-?*)
			usage
			printf 'Error parsing argument: %s\n' "$1"
			;;
		*)
			if [ "x$1" = "x" ]; then
				break
			else
				get_global_functions "$1"
			fi
			;;
	esac
	shift
done

export version_list

cat $tmpfile2 > /tmp/A

$my_awk "
BEGIN {
	# operator new[](unsigned long)
	new_delete[\"_Znam\"] = 1
	new_delete[\"_ZnamRKSt9nothrow_t\"] = 1
	# operator new(unsigned long)
	new_delete[\"_Znwm\"] = 1
	new_delete[\"_ZnwmRKSt9nothrow_t\"] = 1
	# operator new[](unsigned int)
	new_delete[\"_Znaj\"] = 1
	new_delete[\"_ZnajRKSt9nothrow_t\"] = 1
	# operator new(unsigned int)
	new_delete[\"_Znwj\"] = 1
	new_delete[\"_ZnwjRKSt9nothrow_t\"] = 1
	# operator new(unsigned long, std::align_val_t)
	new_delete[\"_ZnwmSt11align_val_t\"] = 1
	new_delete[\"_ZnwmSt11align_val_tRKSt9nothrow_t\"] = 1
	# operator new(unsigned int, std::align_val_t)
	new_delete[\"_ZnwjSt11align_val_t\"] = 1
	new_delete[\"_ZnwjSt11align_val_tRKSt9nothrow_t\"] = 1
	# operator new[](unsigned long, std::align_val_t)
	new_delete[\"_ZnamSt11align_val_t\"] = 1
	new_delete[\"_ZnamSt11align_val_tRKSt9nothrow_t\"] = 1
	# operator new[](unsigned int, std::align_val_t)
	new_delete[\"_ZnajSt11align_val_t\"] = 1
	new_delete[\"_ZnajSt11align_val_tRKSt9nothrow_t\"] = 1
	# operator delete[](void *)
	new_delete[\"_ZdaPv\"] = 1
	new_delete[\"_ZdaPvRKSt9nothrow_t\"] = 1
	# operator delete(void *)
	new_delete[\"_ZdlPv\"] = 1
	new_delete[\"_ZdlPvRKSt9nothrow_t\"] = 1
	# operator delete[](void*, unsigned long)
	new_delete[\"_ZdaPvm\"] = 1
	# operator delete(void*, unsigned long)
	new_delete[\"_ZdlPvm\"] = 1
	# operator delete[](void*, unsigned int)
	new_delete[\"_ZdaPvj\"] = 1
	# operator delete(void*, unsigned int)
	new_delete[\"_ZdlPvj\"] = 1
	# operator delete(void*, std::align_val_t)
	new_delete[\"_ZdlPvSt11align_val_t\"] = 1
	new_delete[\"_ZdlPvSt11align_val_tRKSt9nothrow_t\"] = 1
	# operator delete[](void*, std::align_val_t)
	new_delete[\"_ZdaPvSt11align_val_t\"] = 1
	new_delete[\"_ZdaPvSt11align_val_tRKSt9nothrow_t\"] = 1
	# operator delete(void*, unsigned long,  std::align_val_t)
	new_delete[\"_ZdlPvmSt11align_val_t\"] = 1
	# operator delete[](void*, unsigned long, std::align_val_t)
	new_delete[\"_ZdaPvmSt11align_val_t\"] = 1
	# operator delete(void*, unsigned int,  std::align_val_t)
	new_delete[\"_ZdlPvjSt11align_val_t\"] = 1
	# operator delete[](void*, unsigned int, std::align_val_t)
	new_delete[\"_ZdaPvjSt11align_val_t\"] = 1

	versioned_functions[\"memcpy\"] = 1
	versioned_functions[\"pthread_attr_getaffinity_np\"] = 1
	versioned_functions[\"pthread_cond_broadcast\"] = 1
	versioned_functions[\"pthread_cond_destroy\"] = 1
	versioned_functions[\"pthread_cond_init\"] = 1
	versioned_functions[\"pthread_cond_signal\"] = 1
	versioned_functions[\"pthread_cond_timedwait\"] = 1
	versioned_functions[\"pthread_cond_wait\"] = 1
	versioned_functions[\"realpath\"] = 1
	versioned_functions[\"sched_getaffinity\"] = 1

	len = 0
}
NR==FNR {
	function_set[\$0] = 1
	next
}
{
	if (\$0 in new_delete) {
		result[len++] = \$0
		next
	}
	if (substr(\$0, 1, 14) == \"__interceptor_\") {
		result[len++] = \$0
		# We have to avoid exporting the interceptors for versioned library
		# functions due to gold internal error.
		orig_name = substr(\$0, 15)
		if ((orig_name in function_set) &&
		    (ENVIRON["version_list"] == "1" ||
		    !(orig_name in versioned_functions))) {
			result[len++] = orig_name
		}
		next
	}
	if (substr(\$0, 1, 12) == \"__sanitizer_\") {
		result[len++] = \$0
		next
	}
}
END {
	for (i in result) {
		print result[i]
	}
}
" $tmpfile2 $tmpfile2 > $tmpfile1

if [ "x$extra" != "x" ]; then
	cat $extra >> $tmpfile1
fi

sort $tmpfile1 -o $tmpfile2

echo "{"
if [ "x$version_list" != "x" ]; then
	echo "global:"
fi
$my_awk '{print "  " $0 ";"}' $tmpfile2
if [ "x$version_list" != "x" ]; then
	echo "local:"
	echo "  *;"
fi
echo "};"
