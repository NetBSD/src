options {}

# create a section
section (0) {
    load 0 > 0..256K;
    load "hello world!" > 0x1000;
}

