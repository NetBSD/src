die "too few arguments" if (scalar(@ARGV) != 2);
$me = $ARGV[0];
$you = $ARGV[1];
$hostname = `hostname`;
$hostname =~ s/\n$//;
$userfqdn = `whoami`;
$userfqdn =~ s/\n$//;
$userfqdn .= '@' . $hostname;
$rcsid = '$KAME: sandiego.pl,v 1.11 2000/03/26 10:52:59 itojun Exp $';

print <<EOF;
# automatically generated from $rcsid
# do not edit.

# sample policy setting for setkey(8):
# spdadd $me $you any -P out ipsec esp/transport//use ah/transport//use;

# search this file for pre_shared_key with various ID key.
path pre_shared_key "./psk.txt" ;

# racoon will search this directory if the certificate or certificate request
# is received.
path certificate "./cert.txt" ;

# personal infomation.
identifier vendor_id "KAME/racoon";
identifier user_fqdn "$userfqdn";
identifier fqdn "$hostname";
identifier keyid "./keyid.txt";

# "log" specifies logging level.  It is followed by either "info", "notify",
# "debug" or "debug2".
log debug2;

# "padding" defines some parameter of padding.  You should not touch these.
padding {
	maximum_length 20;	# maximum padding length.
	randomize off;		# enbale randomize length.
	restrict_check off;	# enable restrict check.
	exclusive_tail off;	# extract last one octet.
}

# if no listen directive is specified, racoon will listen to all
# available interface addresses.
listen {
#	isakmp 127.0.0.1 [7000];
#	isakmp 0.0.0.0 [500];
	admin [7002];	# administrative's port by kmpstat.
}

# Specification of default various timer.
timer {
	# These value can be changed per remote node.
	counter 1;		# maximun trying count to send.
	interval 30 sec;	# maximun interval to resend.
	persend 1;		# the number of packets per a send.

	# timer for waiting to complete each phase.
	phase1 20 sec;
	phase2 15 sec;
}

# main mode example, with "anonymous" (any peer) configuration
remote anonymous
{
	# In below case, main mode and aggressive mode are accepted.  When
	# initiating, main mode is first to be sent.
	exchange_mode main, aggressive;

	identifier address;
	nonce_size 16;

	# for aggressive mode definition.
	dh_group modp1024;

	proposal {
		encryption_algorithm 3des;
		hash_algorithm md5;
		authentication_method pre_shared_key ;
		dh_group modp1024;
		lifetime time 600 sec;
	}
	proposal {
		encryption_algorithm des;
		hash_algorithm sha1;
		authentication_method pre_shared_key ;
		dh_group modp1024;
		lifetime time 1000 sec;
	}
#	proposal {
#		encryption_algorithm 3des;
#		hash_algorithm sha1;
#		authentication_method rsasig ;
#		dh_group modp1024;
#		lifetime time 600 sec;
#	}
}

remote 194.100.55.1 [500]
{
	exchange_mode main, aggressive;

	# default doi is "ipsec_doi".
	doi ipsec_doi;

	# default situation is "identity_only".
	situation identity_only;

	# specify the identifier type
	# "address", "fqdn", "user_fqdn", "keyid"
	identifier user_fqdn;

	# specify the bytes length of nonce.
	nonce_size 16;

	# means to do keep-a-live.  This should not be used in dial-up.
	keepalive;

	dh_group modp1024;

	proposal {
		# they can be defined explicitly.
		encryption_algorithm des;
		hash_algorithm md5;
		dh_group modp768;
		authentication_method pre_shared_key ;
	}
	proposal {
		encryption_algorithm 3des;
		authentication_method pre_shared_key ;

		# they can be defined individually.
		lifetime time 5 min;
		lifetime byte 2 MB;
	}
}

policy $me $you any inout ipsec
{
	pfs_group modp1024;

	# This proposal means IP2|AH|ESP|ULP.
	proposal {
		lifetime time 300 second;
		lifetime byte 10000 KB;

		protocol esp {
			level require ;
			mode transport ;
			encryption_algorithm des ;
			authentication_algorithm hmac_sha1 ;
		}

		# "ah" means AH.
		protocol ah {
			level require ;
			mode transport ;
			authentication_algorithm hmac_sha1 ;
		}
	}

	# This proposal means IP2|ESP|ULP.
	proposal {
		lifetime time 600 second;
		lifetime byte 10000 KB;
		protocol esp {
			level require ;
			mode transport ;
			encryption_algorithm des ;
			authentication_algorithm hmac_sha1 ;
		}
	}

}
EOF
