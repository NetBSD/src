#!/usr/bin/python

#
# Id: gen-data-queryperf.py,v 1.1 2003/04/10 02:33:40 marka Exp
#
# Contributed by Stephane Bortzmeyer <bortzmeyer@nic.fr>
#
# "A small tool which may be useful with contrib/queryperf. This script
#  can generate files of queries, both with random names (to test the
#  behaviour with NXdomain) and with domains from a real zone file."
#

import sys
import getopt
import random
import re

ldh = []
# Letters
for i in range(97, 122):
    ldh.append(chr(i))
# Digits
for i in range(48, 57):
    ldh.append(chr(i))
# Hyphen
ldh.append('-')

maxsize=10
tld='org'
num=4
percent_random = 0.3
gen = None
zone_file = None
domains = {}
domain_ns = "^([a-z0-9-]+)(\.([a-z0-9-\.]+|)|)( +IN|) +NS"
domain_ns_re = re.compile(domain_ns, re.IGNORECASE)

def gen_random_label():
    label = ""
    for i in range(gen.randint(1, maxsize)):
        label = label + gen.choice(ldh)
    return label

def make_domain(label):
    return "www." + label + "." + tld + "     A"

def usage():
    sys.stdout.write("Usage: " + sys.argv[0] + " [-n number] " + \
                     "[-p percent-random] [-t TLD]\n")
    sys.stdout.write("       [-m MAXSIZE] [-f zone-file]\n")
    
try:
    optlist, args = getopt.getopt(sys.argv[1:], "hp:f:n:t:m:",
                                  ["help", "percentrandom=", "zonefile=",
                                   "num=", "tld=",
                                   "maxsize="])
    for option, value in optlist:
        if option == "--help" or option == "-h":
            usage()
            sys.exit(0)
        elif option == "--number" or option == "-n":
            num = int(value)
        elif option == "--maxsize" or option == "-m":
            maxsize = int(value)
        elif option == "--percentrandom" or option == "-p":
            percent_random = float(value)
        elif option == "--tld" or option == "-t":
            tld = str(value)
        elif option == "--zonefile" or option == "-f":
            zone_file = str(value)
        else:
            error("Unknown option " + option)
except getopt.error, reason:
    sys.stderr.write(sys.argv[0] + ": " + str(reason) + "\n")
    usage()
    sys.exit(1)
    
if len(args) <> 0:
    usage()
    sys.exit(1)

gen = random.Random()
if zone_file:
    file = open(zone_file)
    line = file.readline()
    while line:
        domain_line = domain_ns_re.match(line)
        if domain_line:
            domain = domain_line.group(1)
            domains[domain] = 1
        line = file.readline()
    file.close()
for i in range(num):
    if zone_file:
        if gen.random() < percent_random:
            print make_domain(gen_random_label())
        else:
            print make_domain(gen.choice(domains.keys()))
    else:
        print make_domain(gen_random_label())
