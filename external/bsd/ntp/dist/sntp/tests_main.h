/*	$NetBSD: tests_main.h,v 1.2.10.2 2015/01/07 10:10:21 msaitoh Exp $	*/

#ifndef TESTS_MAIN_H
#define TESTS_MAIN_H

#include "config.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

extern "C" {
#include "ntp_stdlib.h"
}

class ntptest : public ::testing::Test {
public:
	static void SetExtraParams(int start, int count, char** argv);
protected:
	static std::vector<std::string> m_params;
};

#endif // TESTS_MAIN_H
