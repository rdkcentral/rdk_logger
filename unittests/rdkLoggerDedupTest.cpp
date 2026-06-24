/*
 * Copyright 2024 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <cstdarg>
#include <cstring>

extern "C" {
#include "rdk_log_dedup.h"
}

class RDKLoggerDedupTest : public ::testing::Test {
protected:
    void SetUp() override {
        rdk_dedup_init();
    }

    void TearDown() override {
        rdk_dedup_flush_all();
    }

    /* Helper to call rdk_dedup_check with printf-style args */
    bool check_dedup(const void *cat, int prio, const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        bool result = rdk_dedup_check(cat, prio, fmt, args);
        va_end(args);
        return result;
    }
};

/* Test: When dedup is disabled, no messages are suppressed */
TEST_F(RDKLoggerDedupTest, DisabledModePassThrough) {
    /* Dedup is disabled by default after init */
    const void *cat = (const void *)0x1000;

    /* Same message 10 times should all pass through */
    for (int i = 0; i < 10; i++) {
        EXPECT_FALSE(check_dedup(cat, 300, "Error: connection failed\n"));
    }
}

/* Test: When enabled, consecutive duplicates are suppressed */
TEST_F(RDKLoggerDedupTest, ConsecutiveDuplicateSuppression) {
    rdk_dedup_config_t cfg = { true, 30, 1000 };
    rdk_dedup_set_config(&cfg);

    const void *cat = (const void *)0x2000;

    /* First message passes through */
    EXPECT_FALSE(check_dedup(cat, 300, "Error: connection failed\n"));

    /* Consecutive identical messages are suppressed */
    EXPECT_TRUE(check_dedup(cat, 300, "Error: connection failed\n"));
    EXPECT_TRUE(check_dedup(cat, 300, "Error: connection failed\n"));
    EXPECT_TRUE(check_dedup(cat, 300, "Error: connection failed\n"));
}

/* Test: Different messages are not suppressed */
TEST_F(RDKLoggerDedupTest, DifferentMessagesPassThrough) {
    rdk_dedup_config_t cfg = { true, 30, 1000 };
    rdk_dedup_set_config(&cfg);

    const void *cat = (const void *)0x3000;

    EXPECT_FALSE(check_dedup(cat, 300, "Message A\n"));
    EXPECT_FALSE(check_dedup(cat, 300, "Message B\n"));
    EXPECT_FALSE(check_dedup(cat, 300, "Message A\n"));
    EXPECT_FALSE(check_dedup(cat, 300, "Message B\n"));
}

/* Test: Max suppression count forces flush */
TEST_F(RDKLoggerDedupTest, MaxSuppressionCountFlush) {
    rdk_dedup_config_t cfg = { true, 30, 5 };
    rdk_dedup_set_config(&cfg);

    const void *cat = (const void *)0x4000;

    /* First passes through */
    EXPECT_FALSE(check_dedup(cat, 300, "Repeated error\n"));

    /* Next 5 are suppressed (reaching max) */
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(check_dedup(cat, 300, "Repeated error\n"));
    }

    /* After max reached, next occurrence passes through (flush) */
    EXPECT_FALSE(check_dedup(cat, 300, "Repeated error\n"));
}

/* Test: Independent category tracking */
TEST_F(RDKLoggerDedupTest, IndependentCategoryTracking) {
    rdk_dedup_config_t cfg = { true, 30, 1000 };
    rdk_dedup_set_config(&cfg);

    const void *cat1 = (const void *)0x5000;
    const void *cat2 = (const void *)0x6000;

    /* First message for each category passes */
    EXPECT_FALSE(check_dedup(cat1, 300, "Error in cat1\n"));
    EXPECT_FALSE(check_dedup(cat2, 300, "Error in cat2\n"));

    /* Duplicates in cat1 are suppressed */
    EXPECT_TRUE(check_dedup(cat1, 300, "Error in cat1\n"));

    /* cat2 with same message is still suppressed (its own duplicate) */
    EXPECT_TRUE(check_dedup(cat2, 300, "Error in cat2\n"));

    /* New message in cat1 passes through */
    EXPECT_FALSE(check_dedup(cat1, 300, "Different message\n"));

    /* cat2 duplicate still suppressed */
    EXPECT_TRUE(check_dedup(cat2, 300, "Error in cat2\n"));
}

/* Test: FNV-1a hash function */
TEST_F(RDKLoggerDedupTest, FNV1aHashConsistency) {
    uint32_t h1 = rdk_dedup_fnv1a_hash("test message");
    uint32_t h2 = rdk_dedup_fnv1a_hash("test message");
    uint32_t h3 = rdk_dedup_fnv1a_hash("different message");

    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, h3);
}

/* Test: NULL inputs don't crash */
TEST_F(RDKLoggerDedupTest, NullInputsSafe) {
    rdk_dedup_config_t cfg = { true, 30, 1000 };
    rdk_dedup_set_config(&cfg);

    EXPECT_FALSE(check_dedup(NULL, 300, "test\n"));
    EXPECT_FALSE(check_dedup((const void *)0x7000, 300, NULL));
}

/* Test: Config getter returns correct values */
TEST_F(RDKLoggerDedupTest, ConfigGetterReturnsCorrectValues) {
    rdk_dedup_config_t cfg = { true, 60, 500 };
    rdk_dedup_set_config(&cfg);

    const rdk_dedup_config_t *got = rdk_dedup_get_config();
    EXPECT_TRUE(got->enabled);
    EXPECT_EQ(got->window_sec, 60u);
    EXPECT_EQ(got->max_suppress, 500u);
}

/* Test: After new message arrives, summary is emitted and new message passes */
TEST_F(RDKLoggerDedupTest, SummaryEmittedOnNewMessage) {
    rdk_dedup_config_t cfg = { true, 30, 1000 };
    rdk_dedup_set_config(&cfg);

    const void *cat = (const void *)0x8000;

    /* First passes */
    EXPECT_FALSE(check_dedup(cat, 300, "Error A\n"));
    /* Duplicates suppressed */
    EXPECT_TRUE(check_dedup(cat, 300, "Error A\n"));
    EXPECT_TRUE(check_dedup(cat, 300, "Error A\n"));

    /* New message passes through (triggers summary internally) */
    EXPECT_FALSE(check_dedup(cat, 300, "Error B\n"));
}

/* Test: Format string with arguments */
TEST_F(RDKLoggerDedupTest, FormatStringWithArgs) {
    rdk_dedup_config_t cfg = { true, 30, 1000 };
    rdk_dedup_set_config(&cfg);

    const void *cat = (const void *)0x9000;

    /* Same format with same args = duplicate */
    EXPECT_FALSE(check_dedup(cat, 300, "Error code: %d\n", 42));
    EXPECT_TRUE(check_dedup(cat, 300, "Error code: %d\n", 42));

    /* Same format with different args = not duplicate */
    EXPECT_FALSE(check_dedup(cat, 300, "Error code: %d\n", 43));
}
