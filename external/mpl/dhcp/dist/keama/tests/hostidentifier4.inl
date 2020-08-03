# host declaration with flexible identifiers config

# subnet4 declaration
subnet 10.5.5.0 netmask 255.255.255.224 {
    range 10.5.5.5 10.5.5.10;
}

# option definition

option my-id code 250 = text;

# host declarations
host test1 {
    host-identifier option my-id test1;
    option domain-search "example.com", "example.org";
    default-lease-time 1800;
    fixed-address 10.5.5.1, 10.10.10.10;
}

host test2 {
    hardware ethernet 00:07:0E:36:48:19;
    fixed-address 10.5.5.2;
}

host test3 {
    hardware fddi 00:07:0E:36:48:19;
    fixed-address 10.10.10.1;
}

subnet 10.10.10.0 netmask 255.255.255.224 { }
