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
 * @file    core/LPFilter.cpp
 * @brief   Low-pass (1Hz) filter for omnidirectional acceleration.
 */

#if !defined(LOG_THRESHOLD)
	// custom logging threshold - keep undefined on the repo
	// #define LOG_THRESHOLD   LOG_LEVEL_TRACE
#endif

#include "common/MathExt.h"
#include "LPFilter.h"
#include "common/Logger.h"

constexpr fl::core::Hertz fl::core::LPFilter::CUTOFF_FREQUENCY;

/***************************************************************************//**
 *  Constructor
 *
 *  @param[in] samplingFrequencyHz
 *      The sampling frequency in [Hz] of the accelerometer (or processing
 *      callback if downsampled). This value should be obtained from the driver.
 */
fl::core::LPFilter::LPFilter(void)
{
	LOG_TRACE(FUNCTION_THIS_PREFIX("()"));
	_G  = 0;
	_B1 =-2;
	_B2 = 1;
	resetMemory();
} // fl::LPFilter::LPFilter

/***************************************************************************//**
 *  Set-up sampling frequency and reset the past state memory
 */
fl::Result fl::core::LPFilter::reset(Hertz samplingFrequencyHz)
{
	if (samplingFrequencyHz > 0) {
		double omegaC  = tan(M_PI * CUTOFF_FREQUENCY / samplingFrequencyHz);
		double omegaC2 = square(omegaC);
		double _B0;

		_B0 = 1.0 / (1 + 2 * omegaC * cos(M_PI_4) + omegaC2);
		_B1 = _B0 * 2*(omegaC2 - 1);
		_B2 = _B0 * (1 - 2 * omegaC * cos(M_PI_4) + omegaC2);
		_G  = _B0 * omegaC2;

		resetMemory();
		LOG_TRACE(FUNCTION_THIS_PREFIX("(%g Hz): OK"), samplingFrequencyHz);
		return Result::OK;
	} else {
		LOG_ERROR(FUNCTION_THIS_PREFIX("(%g Hz): E_INVALID_ARGUMENT"), samplingFrequencyHz);
		return Result::E_INVALID_ARGUMENT;
	}
} // fl::LPFilter::reset

/***************************************************************************//**
 *  Reset only the past state memory
 */
void fl::core::LPFilter::resetMemory(double uPast)
{
	LOG_DETAIL(FUNCTION_THIS_PREFIX("()"));
	_u[TimeShift::T1] = uPast;
	_u[TimeShift::T2] = uPast;
	_v[TimeShift::T1] = uPast;
	_v[TimeShift::T2] = uPast;
} // fl::LPFilter::reset

/***************************************************************************//**
 *  Process single input sample
 *
 *  @param[in] u
 *      Input signal value
 *
 *  @return
 *      Output signal value
 */
real fl::core::LPFilter::process(real u)
{
	double v;

	v  = _G * (u + 2 * _u[TimeShift::T1] + _u[TimeShift::T2]) - _B1 * _v[TimeShift::T1] - _B2 * _v[TimeShift::T2];
	_u[TimeShift::T2] = _u[TimeShift::T1];
	_u[TimeShift::T1] =  u;
	_v[TimeShift::T2] = _v[TimeShift::T1];
	_v[TimeShift::T1] =  v;

	LOG_DETAIL(FUNCTION_THIS_PREFIX("(%g): %g"), u, v);
	return static_cast<real>(v);
} // fl::LPFilter::process
