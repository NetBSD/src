/*	$NetBSD: tests_main.h,v 1.1.1.1.6.1 2014/12/24 00:05:24 riz Exp $	*/

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
