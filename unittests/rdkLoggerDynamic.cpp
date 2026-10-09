/*
  * If not stated otherwise in this file or this component's LICENSE file
  * the following copyright and licenses apply:
  *
  * Copyright 2016 RDK Management
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
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include "rdk_logger.h"
#include "gtest_app.h"
typedef enum {
    CAT_FATAL,
    CAT_ERROR,
    CAT_WARN,
    CAT_NOTICE,
    CAT_INFO,
    CAT_DEBUG,
    CAT_TRACE,
    CAT_NONE,
    CAT_BOUNDARY
} LogCategory;

typedef enum {
    LVL_FATAL,
    LVL_ERROR,
    LVL_WARN,
    LVL_NOTICE,
    LVL_INFO,
    LVL_DEBUG,
    LVL_TRACE,
    LVL_NONE,
    LVL_TEST	    
} LogLevel;

typedef struct {
    LogCategory category;
    LogLevel level;
} rdklogctrl_args_t;

void* run_rdklogctrl(void* arg) {
    rdklogctrl_args_t* args = (rdklogctrl_args_t*)arg;
    char category_str[128] = "";
    char level_str[16] = "";

    // Switch-case for category
    switch (args->category) {
        case CAT_FATAL:
            strcpy(category_str, "LOG.RDK.FATAL");
            break;
        case CAT_ERROR:
            strcpy(category_str, "LOG.RDK.ERROR");
            break;
        case CAT_WARN:
            strcpy(category_str, "LOG.RDK.WARN");
            break;
        case CAT_NOTICE:
            strcpy(category_str, "LOG.RDK.NOTICE");
	    break;
        case CAT_INFO:
            strcpy(category_str, "LOG.RDK.INFO");
	    break;
        case CAT_DEBUG:
            strcpy(category_str, "LOG.RDK.DEBUG");
	    break;
        case CAT_TRACE:
            strcpy(category_str, "LOG.RDK.TRACE");
	    break;
        case CAT_NONE:
            strcpy(category_str, "LOG.RDK.NONE");
	    break;
        case CAT_BOUNDARY:
            strcpy(category_str, "LOG.RDK.rdklogger_component_name_for_boundary_length_testing_123");
            break;
        default:
            break;
    }

    // Switch-case for level
    switch (args->level) {
        case LVL_FATAL:
            strcpy(level_str, "FATAL");
            break;
        case LVL_ERROR:
            strcpy(level_str, "ERROR");
            break;
        case LVL_WARN:
            strcpy(level_str, "WARN");
            break;
        case LVL_NOTICE:
            strcpy(level_str, "NOTICE");
            break;
        case LVL_INFO:
            strcpy(level_str, "INFO");
            break;
        case LVL_DEBUG:
            strcpy(level_str, "DEBUG");
            break;
        case LVL_TRACE:
            strcpy(level_str, "TRACE");
            break;
        case LVL_NONE:
            strcpy(level_str, "NONE");
            break;
        case LVL_TEST:
            strcpy(level_str, "~NONE");
            break;
        default:
            break;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./rdklogctrl rdk_logger_gtest %s %s", category_str, level_str);
    fprintf(stderr, "Executing command: %s\n", cmd);
    int ret = system(cmd);
    (void)ret;
    return NULL;
}

static void* run_log_messages(void*) {
    for (int i = 0; i < 500; ++i)
        rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.TEST", "Concurrent test log\n");
    return NULL;
}

TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_fatal) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;    
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_FATAL;
    args.level = LVL_FATAL;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}

TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_error) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_ERROR;
    args.level = LVL_ERROR;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}
TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_warn) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_WARN;
    args.level = LVL_WARN;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}
TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_notice) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_NOTICE;
    args.level = LVL_NOTICE;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}
TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_info) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_INFO;
    args.level = LVL_INFO;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}
TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_debug) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_DEBUG;
    args.level = LVL_DEBUG;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}
TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_trace) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_TRACE;
    args.level = LVL_TRACE;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}
TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_none) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_NONE;
    args.level = LVL_NONE;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);
}

TEST(RdkDynamicLoggerTest, MessageProcessingViaSystem_negnone) {
    rdk_Error ret = RDK_SUCCESS;
    char conf_file[] = GTEST_DEBUG_INI_FILE;
    EXPECT_EQ(rdk_logger_init(conf_file), 0);

    // Prepare arguments for thread
    rdklogctrl_args_t args;
    args.category = CAT_NONE;
    args.level = LVL_TEST;

    // Spawn a thread to run rdklogctrl (client)
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));

    // Wait briefly to allow message to be sent
    usleep(500000); // 0.5 seconds

    // Log a message using the high-level API, which will exercise dynamic logger code
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Test message for dynamic logger\n");
    // Clean up
    pthread_join(client_thread, nullptr);

}

TEST(RdkDynamicLoggerTest, Critical001_RejectsOversizedComponentName) {
    if (geteuid() != 0)
        GTEST_SKIP() << "Run this dynamic logger test as root";

    char conf_file[] = GTEST_DEBUG_INI_FILE;
    ASSERT_EQ(rdk_logger_init(conf_file), RDK_SUCCESS);

    const char* component = "LOG.RDK.rdklogger_component_name_for_boundary_length_testing_123";
    ASSERT_EQ(strlen(component), 64U);

    rdklogctrl_args_t args;
    args.category = CAT_BOUNDARY;
    args.level = LVL_INFO;

    testing::internal::CaptureStderr();
    pthread_t client_thread;
    ASSERT_EQ(0, pthread_create(&client_thread, nullptr, run_rdklogctrl, &args));
    usleep(500000);
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.TESTMOD", "Process dynamic log request\n");
    ASSERT_EQ(0, pthread_join(client_thread, nullptr));

    const std::string output = testing::internal::GetCapturedStderr();
    EXPECT_NE(output.find("Error: component name too long"), std::string::npos);
}

TEST(RdkDynamicLoggerTest, Medium002_ConcurrentLogCallsComplete) {
    if (geteuid() != 0)
        GTEST_SKIP() << "Run this dynamic logger test as root";

    ASSERT_EQ(rdk_logger_init(GTEST_DEBUG_INI_FILE), RDK_SUCCESS);
    rdklogctrl_args_t args;
    args.category = CAT_INFO;
    args.level = LVL_INFO;

    pthread_t control_thread;
    pthread_t log_threads[2];
    ASSERT_EQ(pthread_create(&control_thread, nullptr, run_rdklogctrl, &args), 0);
    ASSERT_EQ(pthread_create(&log_threads[0], nullptr, run_log_messages, nullptr), 0);
    ASSERT_EQ(pthread_create(&log_threads[1], nullptr, run_log_messages, nullptr), 0);
    ASSERT_EQ(pthread_join(control_thread, nullptr), 0);
    ASSERT_EQ(pthread_join(log_threads[0], nullptr), 0);
    ASSERT_EQ(pthread_join(log_threads[1], nullptr), 0);
}

TEST(RdkDynamicLoggerTest, Medium005_RdklogctrlRejectsOversizedAppName) {
    const std::string app_name(200, 'A');
    const char* output_path = "/tmp/rdklogctrl_medium005_test.txt";
    std::string command = "./rdklogctrl " + app_name +
                          " LOG.RDK.TEST DEBUG > " + output_path + " 2>&1";
    ASSERT_NE(system(command.c_str()), -1);

    std::ifstream output_file(output_path);
    const std::string output((std::istreambuf_iterator<char>(output_file)),
                             std::istreambuf_iterator<char>());
    EXPECT_NE(output.find("exceed packet size"), std::string::npos);
    remove(output_path);
}

