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
 * @file
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   Client node tests.
 */

#include "server/client-node.hpp"

#include <gtest/gtest.h>

using namespace fl;
using namespace fl::core;

namespace
{

struct ClientNodeTest : public ::testing::Test
{
    ClientNode::Defaults defaults;

    ClientNodeTest()
    {
        constexpr auto SECOND = std::chrono::seconds(1);

        defaults[core::AccuracyLevel::NO_POWER] =
                ClientNode::AccuracyBasedSettings{100 * SECOND, 80 * SECOND};
        defaults[core::AccuracyLevel::BALANCED_POWER] =
                ClientNode::AccuracyBasedSettings{10 * SECOND, 8 * SECOND};
        defaults[core::AccuracyLevel::HIGH_ACCURACY] =
                ClientNode::AccuracyBasedSettings{2 * SECOND, 1 * SECOND};
    }

    void expectDefaultSettings(const ClientNode& node, AccuracyLevel level)
    {
        EXPECT_EQ(node.getAccuracyLevel(), level);
        EXPECT_EQ(node.getDesiredInterval(), defaults[level].desiredInterval);
        EXPECT_EQ(node.getMinimalInterval(), defaults[level].minimalInterval);
    }
};

}

TEST_F(ClientNodeTest, Init)
{
    ClientNode node{defaults};

    expectDefaultSettings(node, AccuracyLevel::NO_POWER);
}

TEST_F(ClientNodeTest, Defaults)
{
    ClientNode node{defaults};

    expectDefaultSettings(node, AccuracyLevel::NO_POWER);

    node.setAccuracyLevel(AccuracyLevel::HIGH_ACCURACY);
    expectDefaultSettings(node, AccuracyLevel::HIGH_ACCURACY);

    node.setAccuracyLevel(AccuracyLevel::BALANCED_POWER);
    expectDefaultSettings(node, AccuracyLevel::BALANCED_POWER);

    node.setAccuracyLevel(AccuracyLevel::NO_POWER);
    expectDefaultSettings(node, AccuracyLevel::NO_POWER);
}

TEST_F(ClientNodeTest, Manual)
{
    ClientNode node{defaults};

    expectDefaultSettings(node, AccuracyLevel::NO_POWER);

    node.setAccuracyLevel(AccuracyLevel::HIGH_ACCURACY);
    expectDefaultSettings(node, AccuracyLevel::HIGH_ACCURACY);

    constexpr auto newDesired = std::chrono::milliseconds(567);
    node.setDesiredInterval(newDesired);
    EXPECT_EQ(node.getAccuracyLevel(), AccuracyLevel::HIGH_ACCURACY);
    EXPECT_EQ(node.getDesiredInterval(), newDesired);
    EXPECT_EQ(node.getMinimalInterval(), defaults[AccuracyLevel::HIGH_ACCURACY].minimalInterval);

    constexpr auto newMinimal = std::chrono::milliseconds(678);
    node.setMinimalInterval(newMinimal);
    EXPECT_EQ(node.getAccuracyLevel(), AccuracyLevel::HIGH_ACCURACY);
    EXPECT_EQ(node.getDesiredInterval(), newDesired);
    EXPECT_EQ(node.getMinimalInterval(), newMinimal);

    node.setAccuracyLevel(AccuracyLevel::BALANCED_POWER);
    EXPECT_EQ(node.getDesiredInterval(), newDesired);
    EXPECT_EQ(node.getMinimalInterval(), newMinimal);
}
