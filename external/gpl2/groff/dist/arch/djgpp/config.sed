# Additional editing of Makefiles and atconfig
/ac_given_INSTALL=/,/^CEOF/ {
  /^s%@g@%/a\
  /path=/s,:,;,g
}

# DOS-style absolute file names should be supported as well
/\*) srcdir=/s,/\*,[\\\\/]* | [A-z]:[\\\\/]*,
/\$]\*) INSTALL=/s,\[/\$\]\*,[\\\\/$]* | [A-z]:[\\\\/]*,

# Who said each line has only \012 at its end?
/DEFS=`sed -f/s,'\\012','\\012\\015',

# Switch the order of the two Sed commands, since DOS path names
# could include a colon
/ac_file_inputs=/s,\( -e "s%\^%\$ac_given_srcdir/%"\)\( -e "s%:% $ac_given_srcdir/%g"\),\2\1,
