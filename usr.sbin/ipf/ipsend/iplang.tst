#
interface { ifname le0; mtu 1500; }

ipv4 {
	src 10.1.1.49; dst 10.1.1.50; id 123; opt { rr 7; };
	tcp {
		seq 12345; ack 0; sport 9999; dport 23; flags S;
		opt { mss 65535; }; data { value "abcdef"; } ;
	}
}
send { via 10.1.1.50; }
#
ipv4 {
	src 10.1.1.49; dst 10.1.1.50; id 1; opt { lsrr 1.1.1.1; };
	tcp {
		seq 12345; ack 0; sport 9999; dport 23; flags S;
		opt { wscale 2 ; eol; mss 1; }; data { value "abcdef"; } ;
	}
}
send { via 10.1.1.50; }
