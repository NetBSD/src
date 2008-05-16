#	$Id: wc.tcl,v 1.1.1.1 2008/05/16 18:04:19 aymeric Exp $ (Berkeley) $Date: 2008/05/16 18:04:19 $
#
proc wc {} {
	global viScreenId
	global viStartLine
	global viStopLine

	set lines [viLastLine $viScreenId]
	set output ""
	set words 0
	for {set i $viStartLine} {$i <= $viStopLine} {incr i} {
		set outLine [split [string trim [viGetLine $viScreenId $i]]]
		set words [expr $words + [llength $outLine]]
	}
	viMsg $viScreenId "$words words"
}
