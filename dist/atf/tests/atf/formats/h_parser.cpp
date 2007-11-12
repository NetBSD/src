//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgement:
//        This product includes software developed by the NetBSD
//        Foundation, Inc. and its contributors.
// 4. Neither the name of The NetBSD Foundation nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

extern "C" {
#include <fcntl.h>
}

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "atf/formats.hpp"
#include "atf/io.hpp"
#include "atf/parser.hpp"
#include "atf/sanity.hpp"

class atffile_reader : protected atf::formats::atf_atffile_reader {
    void
    got_conf(const std::string& name, const std::string& val)
    {
        std::cout << "got_conf(" << name << ", " << val << ")" << std::endl;
    }

    void
    got_prop(const std::string& name, const std::string& val)
    {
        std::cout << "got_prop(" << name << ", " << val << ")" << std::endl;
    }

    void
    got_tp(const std::string& name, bool isglob)
    {
        std::cout << "got_tp(" << name << ", " << (isglob ? "true" : "false")
                  << ")" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    atffile_reader(std::istream& is) :
        atf::formats::atf_atffile_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_atffile_reader::read();
    }
};

class config_reader : protected atf::formats::atf_config_reader {
    void
    got_var(const std::string& name, const std::string& val)
    {
        std::cout << "got_var(" << name << ", " << val << ")" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    config_reader(std::istream& is) :
        atf::formats::atf_config_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_config_reader::read();
    }
};

class tcs_reader : protected atf::formats::atf_tcs_reader {
    void
    got_ntcs(size_t ntcs)
    {
        std::cout << "got_ntcs(" << ntcs << ")" << std::endl;
    }

    void
    got_tc_start(const std::string& tcname)
    {
        std::cout << "got_tc_start(" << tcname << ")" << std::endl;
    }

    void
    got_tc_end(const atf::tests::tcr& tcr)
    {
        std::string r;
        switch (tcr.get_status()) {
        case atf::tests::tcr::status_passed:
            r = "passed";
            break;

        case atf::tests::tcr::status_failed:
            r = "failed, " + tcr.get_reason();
            break;

        case atf::tests::tcr::status_skipped:
            r = "skipped, " + tcr.get_reason();
            break;

        default:
            UNREACHABLE;
        }

        std::cout << "got_tc_end(" << r << ")" << std::endl;
    }

    void
    got_stdout_line(const std::string& line)
    {
        std::cout << "got_stdout_line(" << line << ")" << std::endl;
    }

    void
    got_stderr_line(const std::string& line)
    {
        std::cout << "got_stderr_line(" << line << ")" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    tcs_reader(std::istream& is) :
        atf::formats::atf_tcs_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        int outfd = ::open(outname.c_str(), O_RDONLY);
        if (outfd == -1)
            throw std::runtime_error("Failed to open " + outname);
        atf::io::file_handle outfh(outfd);

        int errfd = ::open(errname.c_str(), O_RDONLY);
        if (errfd == -1)
            throw std::runtime_error("Failed to open " + errname);
        atf::io::file_handle errfh(errfd);

        atf::io::unbuffered_istream outin(outfh);
        atf::io::unbuffered_istream errin(errfh);
        atf_tcs_reader::read(outin, errin);
    }
};

class tps_reader : protected atf::formats::atf_tps_reader {
    void
    got_info(const std::string& what, const std::string& val)
    {
        std::cout << "got_info(" << what << ", " << val << ")" << std::endl;
    }

    void
    got_ntps(size_t ntps)
    {
        std::cout << "got_ntps(" << ntps << ")" << std::endl;
    }

    void
    got_tp_start(const std::string& tpname, size_t ntcs)
    {
        std::cout << "got_tp_start(" << tpname << ", " << ntcs << ")"
                  << std::endl;
    }

    void
    got_tp_end(const std::string& reason)
    {
        std::cout << "got_tp_end(" << reason << ")" << std::endl;
    }

    void
    got_tc_start(const std::string& tcname)
    {
        std::cout << "got_tc_start(" << tcname << ")" << std::endl;
    }

    void
    got_tc_end(const atf::tests::tcr& tcr)
    {
        std::string r;
        switch (tcr.get_status()) {
        case atf::tests::tcr::status_passed:
            r = "passed";
            break;

        case atf::tests::tcr::status_failed:
            r = "failed, " + tcr.get_reason();
            break;

        case atf::tests::tcr::status_skipped:
            r = "skipped, " + tcr.get_reason();
            break;

        default:
            UNREACHABLE;
        }

        std::cout << "got_tc_end(" << r << ")" << std::endl;
    }

    void
    got_tc_stdout_line(const std::string& line)
    {
        std::cout << "got_tc_stdout_line(" << line << ")" << std::endl;
    }

    void
    got_tc_stderr_line(const std::string& line)
    {
        std::cout << "got_tc_stderr_line(" << line << ")" << std::endl;
    }

    void
    got_eof(void)
    {
        std::cout << "got_eof()" << std::endl;
    }

public:
    tps_reader(std::istream& is) :
        atf::formats::atf_tps_reader(is)
    {
    }

    void
    read(const std::string& outname, const std::string& errname)
    {
        atf_tps_reader::read();
    }
};

template< class R >
void
process(const std::string& file, const std::string& outname = "",
        const std::string& errname = "")
{
    try {
        std::ifstream is(file.c_str());
        if (!is)
            throw std::runtime_error("Cannot open `" + file + "'");
        R reader(is);
        reader.read(outname, errname);
    } catch (const atf::parser::parse_errors& pes) {
        std::cerr << pes.what();
    } catch (const atf::parser::parse_error& pe) {
        std::cerr << "LONELY PARSE ERROR: " << pe.first << ": "
                  << pe.second << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    try {
        if (argc < 3) {
            std::cerr << "Missing arguments" << std::endl;
            return EXIT_FAILURE;
        }
        std::string format(argv[1]);
        std::string file(argv[2]);

        if (format == std::string("application/X-atf-atffile")) {
            process< atffile_reader >(file);
        } else if (format == std::string("application/X-atf-config")) {
            process< config_reader >(file);
        } else if (format == std::string("application/X-atf-tcs")) {
            if (argc < 5) {
                std::cerr << "Missing arguments" << std::endl;
                return EXIT_FAILURE;
            }
            process< tcs_reader >(file, argv[3], argv[4]);
        } else if (format == std::string("application/X-atf-tps")) {
            process< tps_reader >(file);
        } else {
            std::cerr << "Unknown format " << format << std::endl;
        }
    } catch (const atf::formats::format_error& e) {
        std::cerr << "Header format error: " << e.what() << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "UNEXPECTED ERROR: " << e.what() << std::endl;
    } catch (...) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
