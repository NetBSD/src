#!obj/sh
# Variable quoting test.

check() {
	if [ "$1" != "$2" ]
	then
		echo "expected [$2], found [$1]" 1>&2
		exit 1
	fi
}

foo='${a:-foo}'
check "$foo" '${a:-foo}'

foo="${a:-foo}"
check "$foo" "foo"

foo=${a:-"'{}'"}
check "$foo" "'{}'"

foo=${a:-${b:-"'{}'"}}
check "$foo" "'{}'"

foo="${a:-"'{}'"}"
check "$foo" "'{}'"

foo="${a:-${b:-"${c:-${d:-"x}"}}y}"}}z}"
#   "                                z*"
#    ${a:-                          } 
#         ${b:-                    }
#              "                y*"
#               ${c:-          }
#                    ${d:-    } 
#                         "x*"  
check "$foo" "x}y}z}"
