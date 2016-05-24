/*
 *	Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

/**
 * @file    tests/ut/LPFilterTest.h
 * @brief   G-test of the LPFilter class
 */

#pragma once
#ifndef __FL_LP_FILTER_TEST_H__
#define __FL_LP_FILTER_TEST_H__

#include <gtest/gtest.h>
#include "core/Types.h"
#include "core/LPFilter.h"

namespace fl {
namespace core {

class LPFilterTest: public ::testing::Test {
protected:

	static constexpr   Hertz MIN_SAMPLING_FREQUENCY = static_cast<Hertz>(  8);   // [Hz]
	static constexpr   Hertz MAX_SAMPLING_FREQUENCY = static_cast<Hertz>(128);   // [Hz]
	static constexpr   Hertz MIN_PROBE_FREQUENCY    = static_cast<Hertz>(0.125); // [Hz]
	static constexpr Seconds TEST_INTERVAL          = static_cast<Seconds>( 32); // [s]

	LPFilter _lpFilter;

}; // fl::core::LPFilter

} // fl::core

} // fl

#endif // __FL_LP_FILTER_TEST_H__