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
 * @file    tests/ut/MotionDetectorTest.h
 * @brief   G-test of the MotionDetector class
 */

#pragma once
#ifndef __FL_MOTION_DETECTOR_TEST_H__
#define __FL_MOTION_DETECTOR_TEST_H__


#include <gtest/gtest.h>
#include "core/MotionDetector.h"

namespace fl {
namespace core {

class MotionDetectorTest
: public ::testing::Test
, public MotionDetector
, public MotionDetector::Receiver {

public:
	//! Constructor
	MotionDetectorTest()
	: MotionDetector(static_cast<MotionDetector::Receiver&>(*this))
	, __receivedMovementTriggers(0)
	, __receivedImmobilityTriggers(0) {
	}
	//! Implementation of MotionDetector::Receiver::onMotionState
	void onMotionState(State state) override;

	unsigned __receivedMovementTriggers;
	unsigned __receivedImmobilityTriggers;

}; // fl::core::MotionDetectorTest

} // fl::core

} // fl

#endif // __FL_MOTION_DETECTOR_TEST_H__
