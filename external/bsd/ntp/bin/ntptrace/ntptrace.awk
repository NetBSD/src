#!/bin/sh

/usr/bin/awk '
#
# Based on a perl script by
# John Hay -- John.Hay@icomtek.csir.co.za / jhay@FreeBSD.org
#
# awk version by Frederick Bruckman <bruckman@ntp.org>


function usage() {
    print "usage: ntptrace [-nv] [-r retries] [host]"
    exit 1
}

function getargs( command, i, argc, argv, done) {
    host = "localhost."
    retries = 1

    command = "getopt nr:v"
    for (i = 2; i <= ARGC; i++)
	command = command " " ARGV[i-1]
    command | getline
    argc = split($0, argv)
    for (i = 1; i <= argc; i++) {
	if (argv[i] == "-n") use_numeric = 1
	if (argv[i] == "-v") do_verbose = 1
	if (argv[i] == "-r") retries = argv[++i]
	if (done == 1) host = argv[i]
	if (argv[i] == "--") done = 1
    }

    if (retries != retries + 0)
	usage()
}

function do_one_server( command, i, nvars, vars, stratum, peer, offset,
	rootdelay, rootdispersion, refid, srcadr) {
    rootdelay = 0
    rootdispersion = 0
    srcadr = ""
    stratum = 255

    command = "ntpq -n -c rv " host
    while (command | getline) {
	gsub(/,/,"")
	nvars = split($0, vars)
	for (i = 1; i <= nvars; i++) {
	    if (vars[i] ~ /stratum=/) {
		stratum = vars[i]
		sub(/stratum=/, "", stratum)
	    }
	    if (vars[i] ~ /peer=/) {
		peer = vars[i]
		sub(/peer=/, "", peer)
	    }
	    if (vars[i] ~ /offset=/) {
		offset = vars[i]
		sub(/offset=/, "", offset)
	    }
	    if (vars[i] ~ /phase=/) {
		offset = vars[i]
		sub(/phase=/, "", offset)
	    }
	    if (vars[i] ~ /rootdelay=/) {
		rootdelay = vars[i]
		sub(/rootdelay=/, "", rootdelay)
	    }
	    if (vars[i] ~ /rootdispersion=/) {
		rootdispersion = vars[i]
		sub(/rootdispersion=/, "", rootdispersion)
	    }
	    if (vars[i] ~ /refid=/) {
		refid = vars[i]
		sub(/refid=/, "", refid)
	    }
	}
    }

    if (stratum == 255)
	exit 1

    offset = offset / 1000
    syncdistance = (rootdispersion + (rootdelay / 2)) / 1000

    printf("%s: stratum %d, offset %f, synch distance %f",
	dhost(host), stratum, offset, syncdistance)
    if (stratum == 1) printf(", refid '\''%s'\''", refid)
    printf("\n")

    if (stratum == 0 || stratum == 1 || stratum == 16)
	exit 0
    if (refid ~ /127\.127\.[0-9]{1,3}\.[0-9]{1,3}/)
	exit 0

    command = "ntpq -n -c '\''pstat " peer "'\'' " host
    while (command | getline) {
	gsub(/,/,"")
	nvars = split($0, vars)
	for (i = 1; i <= nvars; i++) {
	    if (vars[i] ~ /srcadr=/) {
		srcadr = vars[i]
		sub(/srcadr=/, "", srcadr)
	    }
	}
    }

    if (srcadr ~ /127\.127\.[0-9]{1,3}\.[0-9]{1,3}/ || srcadr == "")
	exit 0

    host = srcadr
    do_one_server()
}

function dhost(myhost,  command) {
	command = "host " myhost
	while (command | getline) {
	    if (use_numeric) {
		if (/address/) return $NF
	    } else {
		if (/pointer/) return $NF
	    }
	}
	return myhost
}

BEGIN {
    getargs()
    do_one_server()
}
' $@
