# strftime.awk ; test the strftime code
#
# input is the output of `date', see Makefile.in
#
# The mucking about with $0 and $N is to avoid problems
# on cygwin, where the timezone field is empty and there
# are two consecutive blanks.

# Additional mucking about to lop off the seconds field;
# helps decrease chance of difference due to a second boundary

{
	$3 = sprintf("%02d", $3 + 0)
	$4 = substr($4, 1, 5)
	print > "strftime.ok"
	$0 = strftime("%a %b %d %H:%M %Z %Y")
	$NF = $NF
	print > OUTPUT
}
