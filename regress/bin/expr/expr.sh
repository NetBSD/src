#!/bin/ksh

function test_expr {
	echo "Testing: `eval echo $1`"
	res=`eval expr $1 2>&1`
	if [ "$res" != "$2" ]; then
		echo "Expected $2, got $res from expression: `eval echo $1`"
		exit 1
	fi
}
	
# These will get eval'd so escape any meta characters

# Test from gtk-- configure that cause problems on old expr
test_expr '3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 4 \& 5 \>= 5' '1'
test_expr '3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 4 \& 5 \>= 6' '0'
test_expr '3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 3 \& 5 \>= 5' '0'
test_expr '3 \> 3 \| 3 = 3 \& 4 \> 4 \| 3 = 2 \& 4 = 4 \& 5 \>= 5' '0'
test_expr '3 \> 2 \| 3 = 3 \& 4 \> 4 \| 3 = 3 \& 4 = 4 \& 5 \>= 6' '1'
test_expr '3 \> 3 \| 3 = 3 \& 4 \> 3 \| 3 = 3 \& 4 = 4 \& 5 \>= 5' '1'

# Basic precendence test with the : operator vs. math
test_expr '2 : 4 / 2' '0'
test_expr '4 : 4 % 3' '1'

# Dangling arithemtic operator
test_expr '.java_wrapper : /' '0'
test_expr '4 : \*' '0'
test_expr '4 : +' '0'
test_expr '4 : -' '0'
test_expr '4 : /' '0'
test_expr '4 : %' '0'

# Basic math test
test_expr '2 + 4 \* 5' '22'

# Basic functional tests
test_expr '2' '2'
test_expr '-4' '-4'
test_expr 'hello' 'hello'

# Compare operator precendence test
test_expr '2 \> 1 \* 17' '0'

# Compare operator tests
test_expr '2 \!= 5' '1'
test_expr '2 \!= 2' '0'
test_expr '2 \<= 3' '1'
test_expr '2 \<= 2' '1'
test_expr '2 \<= 1' '0'
test_expr '2 \< 3' '1'
test_expr '2 \< 2' '0'
test_expr '2 = 2' '1'
test_expr '2 = 4' '0'
test_expr '2 \>= 1' '1'
test_expr '2 \>= 2' '1'
test_expr '2 \>= 3' '0'
test_expr '2 \> 1' '1'
test_expr '2 \> 2' '0'

# Known failure once
test_expr '1 \* -1' '-1'

# Test a known case that should fail
test_expr '- 1 + 5' 'expr: syntax error'

# Double check negative numbers
test_expr '1 - -5' '6'

# More complex math test for precedence
test_expr '-3 + -1 \* 4 + 3 / -6' '-7'

# The next two are messy but the shell escapes cause that. 
# Test precendence
test_expr 'X1/2/3 : X\\\(.\*[^/]\\\)//\*[^/][^/]\*/\*$ \| . : \\\(.\\\)' '1/2'

# Test proper () returning \1 from a regex
test_expr '1/2 : .\*/\\\(.\*\\\)' '2'

exit 0
