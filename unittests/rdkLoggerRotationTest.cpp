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
/******************************************************
Test Case : Testing RDK Logger Log Rotation Functionality
*******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "rdk_logger.h"
#include "gtest_app.h"
#include "test_utils.h"
class RDKLoggerRotationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        system("mkdir -p /tmp/rdk_logger_rotation_test");
        
        // Create test configuration file
        createTestConfigFile("/tmp/rdk_logger_rotation_test/test.ini", 
            "LOG.RDK.DEFAULT=WARN\n"
            "LOG.RDK.ROTATION=DEBUG\n");
    }
    
    void TearDown() override {
        // Cleanup
        system("rm -rf /tmp/rdk_logger_rotation_test");
        rdk_logger_deinit();
    }
    
    void createTestConfigFile(const char* filename, const char* content) {
        FILE* file = fopen(filename, "w");
        if (file) {
            fprintf(file, "%s", content);
            fclose(file);
        }
    }
    
    long getFileSize(const char* filename) {
        struct stat st;
        if (stat(filename, &st) == 0) {
            return st.st_size;
        }
        return -1;
    }
    
    int countFilesInDirectory(const char* dirname) {
        DIR* dir = opendir(dirname);
        if (!dir) {
            return -1;
        }
        
        int count = 0;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.') {
	        printf("Found file: %s\n", entry->d_name);	    
                count++;
            }
        }
        closedir(dir);
        return count;
    }
    
    void createLargeLogMessage(char* buffer, size_t size) {
        // Create a message that will fill the buffer
        const char* base_message = "This is a test log message for rotation testing. ";
        size_t base_len = strlen(base_message);
        
        size_t pos = 0;
        while (pos < size - 1) {
            size_t remaining = size - pos - 1;
            size_t copy_len = (remaining > base_len) ? base_len : remaining;
            memcpy(buffer + pos, base_message, copy_len);
            pos += copy_len;
        }
        buffer[size - 1] = '\0';
    }
};

// Test extended initialization with log rotation
TEST_F(RDKLoggerRotationTest, ExtendedInitialization) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "test_rotation.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024; // 1KB
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_TRACE;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Extended initialization should succeed";

            // Test that logging works
            rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Test message for rotation");
    });
}

TEST_F(RDKLoggerRotationTest, CountBasedRotation) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "count_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 256; // 256 Bytes
            testPolicy.fileCountMax = 2;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_TRACE;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Extended initialization should succeed";

            // Generate many log messages to trigger multiple rotations
            char large_message[200];
            createLargeLogMessage(large_message, sizeof(large_message));

            for (int i = 0; i < 20; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d: %s", i, large_message);
                usleep(10000); // 10ms delay to avoid excessively long test runtime
            }

            // Check that only maxCount files exist
            int file_count = countFilesInDirectory("/tmp/rdk_logger_rotation_test");
            printf("file_count : %d\n",file_count);
            system("ls -lt /tmp/rdk_logger_rotation_test");
            EXPECT_LE(file_count, testPolicy.fileCountMax + 1) << "Should not exceed maxCount files";
    });
}
#if 0
// Test log rotation with invalid configuration
TEST_F(RDKLoggerRotationTest, InvalidConfiguration)
{
    RUN_IN_FORK({
            rdk_Error err = rdk_logger_ext_init(NULL);
            EXPECT_EQ(-1, (int)err)<<"EXT_INIT failed";

            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024; // 1KB
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_ERROR;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;

            err = rdk_logger_ext_init(&config);
            EXPECT_EQ(-1, (int)err)<<"EXT_INIT failed";
    });
}
#endif
// Test log rotation with invalid directory
TEST_F(RDKLoggerRotationTest, InvalidDirectory) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/nonexistent/directory", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024; // 1KB
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_ERROR;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            // Should handle gracefully (may fail or create directory)
    });
}

// Test log rotation with very small size limits
TEST_F(RDKLoggerRotationTest, VerySmallSizeLimits) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "small_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 10; // 10 bytes
            testPolicy.fileCountMax = 2;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_INFO;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Should handle very small size limits";

            // Generate log messages
            for (int i = 0; i < 5; i++) {
            rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d", i);
            sleep(1);
            }
    });
    // Should handle gracefully
}

// Test log rotation with very large size limits
TEST_F(RDKLoggerRotationTest, VeryLargeSizeLimits) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "large_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024*1024*100; // 100MB
            testPolicy.fileCountMax = 10;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_INFO;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_DETAIL_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Should handle very large size limits";
            printf("ext_init succes\n");
            // Generate some log messages
            for (int i = 0; i < 10; i++) {
            rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d", i);
            sleep(1);
            printf("Iteration:%d\n",i);
            }
    });
    // Should handle gracefully
}

// Test log rotation with zero count limits
TEST_F(RDKLoggerRotationTest, ZeroCountLimits) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "zero_count_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024; // 1KB
            testPolicy.fileCountMax = 0;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_INFO;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_PLAINTEXT;
            config.pFilePolicy = &testPolicy;

            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Should handle zero count limits";

            // Generate log messages
            for (int i = 0; i < 5; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d", i);
                sleep(1);
            }
    });
    // Should handle gracefully
}

// Test log rotation with negative values
TEST_F(RDKLoggerRotationTest, NegativeValues) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "negative_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = -1;
            testPolicy.fileCountMax = -1;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_INFO;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "negative values";

            for (int i = 0; i < 5; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d", i);
                sleep(1);
            }
    });
    // Should handle gracefully
}
// Test log rotation with long file names
TEST_F(RDKLoggerRotationTest, LongFileNames) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            // Create a very long file name
            char long_filename[RDKLOG_MAX_FILENAME_SIZE];
            memset(long_filename, 'A', sizeof(long_filename) - 5);
            strcpy(long_filename + sizeof(long_filename) - 5, ".log");
            long_filename[sizeof(long_filename) - 1] = '\0';

            strncpy(testPolicy.fileName, long_filename, sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024; // 1KB
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_TRACE;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Should handle long file names";

            // Generate log messages
            for (int i = 0; i < 5; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d", i);
                sleep(1);
            }
    });
    // Should handle gracefully
}

// Test log rotation with long directory paths
TEST_F(RDKLoggerRotationTest, LongDirectoryPaths) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            char long_dir[RDKLOG_MAX_PATH_SIZE];
            memset(long_dir, 'A', sizeof(long_dir) - 1);
            long_dir[sizeof(long_dir) - 1] = '\0';

            strncpy(testPolicy.fileLocation, long_dir, sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024; // 1 KB
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_TRACE;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&config);
            // Should handle gracefully (may fail due to path length)

            // Generate log messages
            for (int i = 0; i < 5; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d", i);
                sleep(1);
            }
    });
    // Should handle gracefully
}
// Test log rotation with special characters in file names
TEST_F(RDKLoggerRotationTest, SpecialCharactersInFileNames) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "test_file_with_special_chars.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024; // 1 KB
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_TRACE;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_PLAINTEXT;
            config.pFilePolicy = &testPolicy;

            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Should handle special characters in file names";

            // Generate log messages
            for (int i = 0; i < 5; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Message %d", i);
                sleep(1);
            }
    });
    // Should handle gracefully
}

// Test log rotation with concurrent access
TEST_F(RDKLoggerRotationTest, ConcurrentAccess) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "concurrent_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 512; // 512 Bytes
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_DEBUG;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS;
            config.pFilePolicy = &testPolicy;

            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Extended initialization should succeed";

            // Generate log messages rapidly to test concurrent access
            for (int i = 0; i < 20; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Concurrent message %d", i);
                rdk_logger_msg_printf(RDK_LOG_DEBUG, "LOG.RDK.ROTATION", "Debug message %d", i);
                rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.ROTATION", "Error message %d", i);
                usleep(10000); // 10 ms delay instead of 1 second to keep test fast
            }
    });
    // Should handle concurrent access gracefully
}

// Test log rotation with RDKLOG_FORMAT_WITH_TID format
TEST_F(RDKLoggerRotationTest, FormatWithTID) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "format_tid_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 512; // 512 bytes to trigger rotation
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_DEBUG;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TID;
            config.pFilePolicy = &testPolicy;

            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Extended initialization with RDKLOG_FORMAT_WITH_TID should succeed";

            // Generate log messages to test the format with thread ID
            char large_message[150];
            createLargeLogMessage(large_message, sizeof(large_message));
            for (int i = 0; i < 15; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "TID format test %d: %s", i, large_message);
                usleep(10000); // 10ms delay
            }

            // Verify log file was created
            char logFilePath[512];
            snprintf(logFilePath, sizeof(logFilePath), "%s/%s", 
                     testPolicy.fileLocation, testPolicy.fileName);
            struct stat st;
            EXPECT_EQ(stat(logFilePath, &st), 0) << "Log file should exist";
            
            // Read and verify log content contains thread ID
            FILE* logFile = fopen(logFilePath, "r");
            ASSERT_NE(logFile, nullptr) << "Failed to open log file for validation";
            char line[1024];
            bool foundThreadId = false;
            while (fgets(line, sizeof(line), logFile)) {
                // Thread ID format typically appears as [TID:xxxxx] or similar
                if (strstr(line, "TID") != NULL || strchr(line, '[') != NULL) {
                    foundThreadId = true;
                    break;
                }
            }
            fclose(logFile);
            EXPECT_TRUE(foundThreadId) << "Log should contain thread ID information";
    });
}

// Test log rotation with RDKLOG_FORMAT_WITH_TS_TID format
TEST_F(RDKLoggerRotationTest, FormatWithTSTID) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "format_ts_tid_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 512; // 512 bytes to trigger rotation
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_DEBUG;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_WITH_TS_TID;
            config.pFilePolicy = &testPolicy;

            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Extended initialization with RDKLOG_FORMAT_WITH_TS_TID should succeed";

            // Generate log messages to test the format with timestamp and thread ID
            char large_message[150];
            createLargeLogMessage(large_message, sizeof(large_message));
            for (int i = 0; i < 15; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "TS_TID format test %d: %s", i, large_message);
                usleep(10000); // 10ms delay
            }

            // Verify log file was created
            char logFilePath[512];
            snprintf(logFilePath, sizeof(logFilePath), "%s/%s", 
                     testPolicy.fileLocation, testPolicy.fileName);
            struct stat st;
            EXPECT_EQ(stat(logFilePath, &st), 0) << "Log file should exist";
            
            // Read and verify log content contains both timestamp and thread ID
            FILE* logFile = fopen(logFilePath, "r");
            ASSERT_NE(logFile, nullptr) << "Failed to open log file for validation";
            char line[1024];
            bool foundTimestamp = false;
            bool foundThreadId = false;
            while (fgets(line, sizeof(line), logFile)) {
                // Look for timestamp patterns (date/time format)
                if (strchr(line, ':') != NULL && strchr(line, '-') != NULL) {
                    foundTimestamp = true;
                }
                // Look for thread ID
                if (strstr(line, "TID") != NULL || strchr(line, '[') != NULL) {
                    foundThreadId = true;
                }
                if (foundTimestamp && foundThreadId) break;
            }
            fclose(logFile);
            EXPECT_TRUE(foundTimestamp) << "Log should contain timestamp information";
            EXPECT_TRUE(foundThreadId) << "Log should contain thread ID information";
    });
}

// Test log rotation with RDKLOG_FORMAT_DETAIL_WITHOUT_TS format
TEST_F(RDKLoggerRotationTest, FormatDetailWithoutTS) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "format_detail_no_ts_test.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 512; // 512 bytes to trigger rotation
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t config;
            memset(&config, 0, sizeof(config));
            config.pModuleName = "LOG.RDK.ROTATION";
            config.loglevel = RDK_LOG_DEBUG;
            config.output = RDKLOG_OUTPUT_FILE;
            config.format = RDKLOG_FORMAT_DETAIL_WITHOUT_TS;
            config.pFilePolicy = &testPolicy;

            rdk_Error ret = rdk_logger_ext_init(&config);
            ASSERT_EQ(ret, RDK_SUCCESS) << "Extended initialization with RDKLOG_FORMAT_DETAIL_WITHOUT_TS should succeed";

            // Generate log messages to test the detailed format without timestamp
            char large_message[150];
            createLargeLogMessage(large_message, sizeof(large_message));
            for (int i = 0; i < 15; i++) {
                rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Detail no TS format test %d: %s", i, large_message);
                usleep(10000); // 10ms delay
            }

            // Verify log file was created
            char logFilePath[512];
            snprintf(logFilePath, sizeof(logFilePath), "%s/%s", 
                     testPolicy.fileLocation, testPolicy.fileName);
            struct stat st;
            EXPECT_EQ(stat(logFilePath, &st), 0) << "Log file should exist";

            // Read and verify log content contains both timestamp and thread ID
            FILE* logFile = fopen(logFilePath, "r");
            ASSERT_NE(logFile, nullptr) << "Failed to open log file for validation";

            char line[1024];
            bool foundModuleName = false;
            if (fgets(line, sizeof(line), logFile)) {
                // Check for full module name (LOG.RDK.ROTATION) which should be in detailed format
                if (strstr(line, "LOG.RDK.ROTATION") != NULL) {
                    foundModuleName = true;
                }
            }
            fclose(logFile);
            EXPECT_TRUE(foundModuleName) << "Log should contain module name in detailed format";
    });
}

#if 0
// Test log rotation with different log levels
TEST_F(RDKLoggerRotationTest, DifferentLogLevels) {
    rdk_logger_ext_config_t config;
    strncpy(config.fileName, "levels_test.log", sizeof(config.fileName) - 1);
    config.fileName[sizeof(config.fileName) - 1] = '\0';

    strncpy(config.fileLocation, "/tmp/rdk_logger_rotation_test", sizeof(config.fileLocation) - 1);
    config.fileLocation[sizeof(config.fileLocation) - 1] = '\0';

    config.fileSizeMax = 1024;
    config.fileCountMax = 3;

    rdk_Error ret = rdk_logger_ext_init(&config);
    ASSERT_EQ(ret, RDK_SUCCESS) << "Extended initialization should succeed";

    // Test all log levels
    rdk_logger_msg_printf(RDK_LOG_FATAL, "LOG.RDK.ROTATION", "Fatal message");
    rdk_logger_msg_printf(RDK_LOG_ERROR, "LOG.RDK.ROTATION", "Error message");
    rdk_logger_msg_printf(RDK_LOG_WARN, "LOG.RDK.ROTATION", "Warning message");
    rdk_logger_msg_printf(RDK_LOG_NOTICE, "LOG.RDK.ROTATION", "Notice message");
    rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.ROTATION", "Info message");
    rdk_logger_msg_printf(RDK_LOG_DEBUG, "LOG.RDK.ROTATION", "Debug message");
    rdk_logger_msg_printf(RDK_LOG_TRACE, "LOG.RDK.ROTATION", "Trace message");

    // Should handle all log levels correctly
}
#endif
