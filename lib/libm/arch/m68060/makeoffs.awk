BEGIN{FS=",";s = -16;}
/\.long/{s += 16;}
s<0 || s>1023{print $0}
s>=0 && s<1024{\
	printf "ASENTRY_NOPROFILE(___060FPLSP%04x) ", s;\
	print $1 "," $2;\
	printf "ASENTRY_NOPROFILE(___060FPLSP%04x) ", s+8;\
	print "	.long	" $3 "," $4;\
}
