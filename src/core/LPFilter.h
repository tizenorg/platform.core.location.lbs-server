/*
 *  Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @file    core/LPFilter.h
 * @brief   Low-pass (1Hz) filter for omnidirectional acceleration.
 */

#pragma once
#ifndef __FL_LPFILTER_H__
#define __FL_LPFILTER_H__


#include "common/Result.h"
#include "common/MathExt.h"

namespace fl {
namespace core {

class LPFilter {
public:
	static constexpr real CUTOFF_FREQUENCY = static_cast<real>(4.0); // [Hz]

	//! Constructor
	LPFilter(void);
	//! Set-up sampling frequency and reset the past state memory
	Result reset(real samplingFrequencyHz);
	//! Reset only the past state memory
	void resetMemory(real uPast = 0);
	//! Process single input sample
	real process(real u);

private:
	enum TimeShift
	{
		T1, // t - 1
		T2, // t - 2
		// always the last
		COUNT
	};

	// filter coefficients
	double _G;  // omega^2/B_0
	double _B1; //     B_1/B_0
	double _B2; //     B_2/B_0
	// memory of past states
	double _u[TimeShift::COUNT];
	double _v[TimeShift::COUNT];

}; // fl::core::LPFilter

} // fl::core

} // fl

#endif // __FL_LPFILTER_H__
