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

#define SQUARE(x)    ((x) * (x))

typedef float   real;

template<class T> inline T square(const T& x)
{
	return x * x;
}

template<class T> inline T cube(const T& x)
{
	return x * x * x;
}

#endif // __FL_MATH_EXT_H__
