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
 * @file    MathExt.h
 * @brief   Math extensions
 */

#pragma once
#ifndef __FL_MATH_EXT_H__
#define __FL_MATH_EXT_H__

#define _USE_MATH_DEFINES
#include <math.h>

typedef float   real;

/*******************************************************************//**
 * Second power
 */
template<class T> constexpr inline T square(const T& x)
{
	return x * x;
}

/*******************************************************************//**
 * Third power
 */
template<class T> constexpr inline T cube(const T& x)
{
	return x * x * x;
}

/*******************************************************************//**
 * Approximate expression for modified Bessel function I_0(x)
 */
template<class T> T besselI0(const T& x)
{
	return static_cast<T>(exp(sqrt(pow(x * (3.0 / 4), 1.0 / 0.46) + 1) - 1));
}

//! Modified Bessel function I_0(1)
constexpr real BESSEL_I0_1 = static_cast<real>(1.2660658777520083355);
//! Modified Bessel function I_0(2)
constexpr real BESSEL_I0_2 = static_cast<real>(2.2795853023360672674);

#endif // __FL_MATH_EXT_H__
