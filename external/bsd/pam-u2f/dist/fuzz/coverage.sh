#!/bin/sh -eux

make -C .. CFLAGS="-fprofile-instr-generate -fcoverage-mapping" V=1
if [ ! -e "corpus" ]; then
    curl --retry 4 -s -o corpus.tgz https://storage.googleapis.com/kroppkaka/corpus/pam-u2f.corpus.tgz
    tar xzf corpus.tgz
fi
./fuzz_format_parsers -runs=1 -dump_coverage=1 corpus
llvm-profdata merge -sparse *.profraw -o default.profdata
llvm-cov report -show-functions -instr-profile=default.profdata fuzz_format_parsers ../*.c

# other report alternatives for convenience:
#llvm-cov report -use-color=false -instr-profile=default.profdata fuzz_format_parsers
#llvm-cov show -format=html -tab-size=8 -instr-profile=default.profdata -output-dir=report fuzz_format_parsers
#llvm-cov show fuzz_format_parsers -instr-profile=default.profdata -name=format -format=html > report.html
