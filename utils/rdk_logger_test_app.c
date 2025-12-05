#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "rdk_logger.h"

// Helper function to create file policy with string copying (not allocation)
rdk_LogOutput_File* create_file_policy(const char* logdir, const char* fileName, 
                                     long maxBytes, int maxRotation) {
    rdk_LogOutput_File* policy = (rdk_LogOutput_File*)malloc(sizeof(rdk_LogOutput_File));
    if (!policy) return NULL;
    
    // Copy strings to the array fields (not allocate new memory)
    strncpy(policy->fileLocation, logdir, sizeof(policy->fileLocation) - 1);
    policy->fileLocation[sizeof(policy->fileLocation) - 1] = '\0';  // Ensure null termination
    
    strncpy(policy->fileName, fileName, sizeof(policy->fileName) - 1);
    policy->fileName[sizeof(policy->fileName) - 1] = '\0';  // Ensure null termination
    
    policy->fileSizeMax = maxBytes;
    policy->fileCountMax = maxRotation;
    
    return policy;
}

// Helper function to free file policy (simpler now - no string freeing needed)
void free_file_policy(rdk_LogOutput_File* policy) {
    if (policy) {
        free(policy);  // Only free the policy structure itself
    }
}
#if 0
// Helper function to create file policy with allocated strings
rdk_LogFilePolicy* create_file_policy(const char* logdir, const char* fileName, 
                                     long maxBytes, int maxRotation) {
    rdk_LogFilePolicy* policy = (rdk_LogFilePolicy*)malloc(sizeof(rdk_LogFilePolicy));
    if (!policy) return NULL;
    strncpy(policy->fileName, "Testlog.txt", sizeof(policy->fileName)-1);
    strncpy(policy->logdir, "/tmp/", sizeof(policy->logdir)-1);    
    policy->maxBytesPerFile = maxBytes;
    policy->maxRotationCount = maxRotation;
    
    return policy;
}
// Helper function to free file policy
void free_file_policy(rdk_LogFilePolicy* policy) {
    if (policy) {
        if (policy->logdir) free(policy->logdir);
        if (policy->fileName) free(policy->fileName);
        free(policy);
    }
}
#endif
// Test different log levels
void test_log_levels(const char* module) {
    printf("\n=== Testing Different Log Levels for %s ===\n", module);
    
    RDK_LOG(RDK_LOG_FATAL, module, "This is a FATAL message - system is unusable\n");
    RDK_LOG(RDK_LOG_ERROR, module, "This is an ERROR message - error condition\n");
    RDK_LOG(RDK_LOG_WARN, module, "This is a WARNING message - warning condition\n");
    RDK_LOG(RDK_LOG_NOTICE, module, "This is a NOTICE message - normal but significant\n");
    RDK_LOG(RDK_LOG_INFO, module, "This is an INFO message - informational\n");
    RDK_LOG(RDK_LOG_DEBUG, module, "This is a DEBUG message - debug information\n");
    RDK_LOG(RDK_LOG_TRACE, module, "This is a TRACE message - trace information\n");
}

// Test stdout appender with different layouts
void test_stdout_appender() {
    printf("\n\n########## TESTING STDOUT APPENDER ##########\n");
    
    // Test with basic layout
    printf("\n--- Testing STDOUT with PLAINTEXT layout ---\n");
    rdk_logger_ext_config_t config = {0};
    config.pCategoryName = "LOG.RDK.STDOUT.BASIC";
    config.appender = RDKLOG_OUTPUT_CONSOLE;
    config.layout = RDKLOG_FORMAT_ONLY_TEXT;
    config.loglevel = RDK_LOG_TRACE;
    config.pFilePolicy = NULL;  // No file policy for stdout
    
    rdk_Error ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        test_log_levels("LOG.RDK.STDOUT.BASIC");
    } else {
        printf("Failed to initialize basic stdout logger\n");
    }
    
    // Test with timestamped layout
    printf("\n--- Testing STDOUT with TIMESTAMPED layout ---\n");
    config.pCategoryName = "LOG.RDK.STDOUT.TIMESTAMP";
    config.layout = RDKLOG_FORMAT_WITH_DATETIME;
    
    ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        test_log_levels("LOG.RDK.STDOUT.TIMESTAMP");
    } else {
        printf("Failed to initialize timestamped stdout logger\n");
    }
    
    // Test with comcast layout
    printf("\n--- Testing STDOUT with COMCAST layout ---\n");
    config.pCategoryName = "LOG.RDK.STDOUT.COMCAST";
    config.layout = RDKLOG_FORMAT_WITH_THREADID;
    
    ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        test_log_levels("LOG.RDK.STDOUT.COMCAST");
    } else {
        printf("Failed to initialize comcast stdout logger\n");
    }
}

// Test file appender with different layouts
void test_file_appender() {
    printf("\n\n########## TESTING FILE APPENDER ##########\n");
    
    // Create test directory
    system("mkdir -p /tmp/rdk_logger_test");
    
    // Test with basic layout
    printf("--- Testing FILE with PLAINTEXT layout (check /tmp/rdk_logger_test/test_log.txt) ---\n");
    rdk_LogOutput_File* policy = create_file_policy("/tmp/rdk_logger_test", "test_log", 
                                                   1024 * 1024, 5);
    if (!policy) {
        printf("Failed to create file policy\n");
        return;
    }
    
    rdk_logger_ext_config_t config = {0};
    config.pCategoryName = "LOG.RDK.FILE.BASIC";
    config.appender = RDKLOG_OUTPUT_FILE;
    config.layout = RDKLOG_FORMAT_ONLY_TEXT;
    config.loglevel = RDK_LOG_TRACE;
    config.pFilePolicy = policy;
    
    rdk_Error ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        test_log_levels("LOG.RDK.FILE.BASIC");
    } else {
        printf("Failed to initialize basic file logger\n");
    }
    free_file_policy(policy);
    
    // Test with timestamped layout
    printf("--- Testing FILE with TIMESTAMPED layout ---\n");
    policy = create_file_policy("/tmp/rdk_logger_test", "test_timestamp_log", 
                               1024 * 1024, 5);
    if (policy) {
        config.pCategoryName = "LOG.RDK.FILE.TIMESTAMP";
        config.layout = RDKLOG_FORMAT_WITH_DATETIME;
        config.pFilePolicy = policy;
        
        ret = rdk_logger_ext_init(&config);
        if (ret == RDK_SUCCESS) {
            test_log_levels("LOG.RDK.FILE.TIMESTAMP");
        } else {
            printf("Failed to initialize timestamped file logger\n");
        }
        free_file_policy(policy);
    }
    
    // Test with comcast layout
    printf("--- Testing FILE with COMCAST layout ---\n");
    policy = create_file_policy("/tmp/rdk_logger_test", "test_comcast_log", 
                               1024 * 1024, 5);
    if (policy) {
        config.pCategoryName = "LOG.RDK.FILE.COMCAST";
        config.layout = RDKLOG_FORMAT_WITH_THREADID;
        config.pFilePolicy = policy;
        
        ret = rdk_logger_ext_init(&config);
        if (ret == RDK_SUCCESS) {
            test_log_levels("LOG.RDK.FILE.COMCAST");
        } else {
            printf("Failed to initialize comcast file logger\n");
        }
        free_file_policy(policy);
    }
}

// Test different log level filtering
void test_log_level_filtering() {
    printf("\n\n########## TESTING LOG LEVEL FILTERING ##########\n");
    
    rdk_logger_ext_config_t config = {0};
    config.appender = RDKLOG_OUTPUT_CONSOLE;
    config.layout = RDKLOG_FORMAT_WITH_DATETIME;
    config.pFilePolicy = NULL;
    
    // Test ERROR level filtering
    printf("\n--- Testing ERROR level filtering (should only show FATAL and ERROR) ---\n");
    config.pCategoryName = "LOG.RDK.LEVEL.ERROR";
    config.loglevel = RDK_LOG_ERROR;
    
    rdk_Error ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        test_log_levels("LOG.RDK.LEVEL.ERROR");
    }
    
    // Test INFO level filtering
    printf("\n--- Testing INFO level filtering (should show FATAL, ERROR, WARN, NOTICE, INFO) ---\n");
    config.pCategoryName = "LOG.RDK.LEVEL.INFO";
    config.loglevel = RDK_LOG_INFO;
    
    ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        test_log_levels("LOG.RDK.LEVEL.INFO");
    }
    
    // Test DEBUG level filtering
    printf("\n--- Testing DEBUG level filtering (should show all except TRACE) ---\n");
    config.pCategoryName = "LOG.RDK.LEVEL.DEBUG";
    config.loglevel = RDK_LOG_DEBUG;
    
    ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        test_log_levels("LOG.RDK.LEVEL.DEBUG");
    }
}

// Test file rotation
void test_file_rotation() {
    printf("\n\n########## TESTING FILE ROTATION ##########\n");
    
    system("mkdir -p /tmp/rdk_logger_rotation_test");
    
    rdk_LogOutput_File* policy = create_file_policy("/tmp/rdk_logger_rotation_test", 
                                                   "rotation_test", 1024, 3);  // Small file size
    if (!policy) {
        printf("Failed to create rotation policy\n");
        return;
    }
    
    rdk_logger_ext_config_t config = {0};
    config.pCategoryName = "LOG.RDK.ROTATION";
    config.appender = RDKLOG_OUTPUT_FILE;
    config.layout = RDKLOG_FORMAT_WITH_DATETIME;
    config.loglevel = RDK_LOG_INFO;
    config.pFilePolicy = policy;
    
    rdk_Error ret = rdk_logger_ext_init(&config);
    if (ret == RDK_SUCCESS) {
        printf("Writing many messages to trigger rotation...\n");
        for (int i = 0; i < 100; i++) {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ROTATION", 
                   "Rotation test message %d - this is a long message to fill up the file quickly so we can test rotation functionality\n", i);
        }
        printf("Check /tmp/rdk_logger_rotation_test/ for rotated files\n");
        system("ls -la /tmp/rdk_logger_rotation_test/");
    } else {
        printf("Failed to initialize rotation logger\n");
    }
    
    free_file_policy(policy);
}
// Test multiple modules
void test_multiple_modules() {
    printf("\n\n########## TESTING MULTIPLE MODULES ##########\n");
    
    // Module 1: High verbosity (DEBUG)
    rdk_logger_ext_config_t config1 = {0};
    config1.pCategoryName = "LOG.RDK.TEST";
    config1.appender = RDKLOG_OUTPUT_CONSOLE;
    config1.layout = RDKLOG_FORMAT_ONLY_TEXT;
    config1.loglevel = RDK_LOG_DEBUG;
    config1.pFilePolicy = NULL;
    
    // Module 2: Low verbosity (ERROR only)
    rdk_logger_ext_config_t config2 = {0};
    config2.pCategoryName = "LOG.RDK.TEST2";
    config2.appender = RDKLOG_OUTPUT_CONSOLE;
    config2.layout = RDKLOG_FORMAT_WITH_DATETIME;
    config2.loglevel = RDK_LOG_ERROR;
    config2.pFilePolicy = NULL;
    
    printf("--- Setting up MODULE1 with DEBUG level ---\n");
    rdk_Error ret = rdk_logger_ext_init(&config1);
    
    printf("--- Setting up MODULE2 with ERROR level ---\n");
    rdk_Error ret1 = rdk_logger_ext_init(&config2);
    if (ret == RDK_SUCCESS) {
        printf("--- Testing MODULE1 (should show all messages) ---\n");
        RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST", "This is a FATAL message - system is unusable\n");
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "This is an ERROR message - error condition\n");
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TEST", "This is a WARNING message - warning condition\n");
        RDK_LOG(RDK_LOG_NOTICE, "LOG.RDK.TEST", "This is a NOTICE message - normal but significant\n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "This is an INFO message - informational\n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TEST", "This is a DEBUG message - debug information\n");
        RDK_LOG(RDK_LOG_TRACE, "LOG.RDK.TEST", "This is a TRACE message - trace information\n");
    }
    if (ret1 == RDK_SUCCESS) {
        printf("--- Testing MODULE2 (should show only FATAL and ERROR) ---\n");
        RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.TEST2", "This is a FATAL message - system is unusable\n");
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST2", "This is an ERROR message - error condition\n");
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TEST2", "This is a WARNING message - warning condition\n");
        RDK_LOG(RDK_LOG_NOTICE, "LOG.RDK.TEST2", "This is a NOTICE message - normal but significant\n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST2", "This is an INFO message - informational\n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TEST2", "This is a DEBUG message - debug information\n");
        RDK_LOG(RDK_LOG_TRACE, "LOG.RDK.TEST2", "This is a TRACE message - trace information\n");
    }
}
// Test error conditions
void test_error_conditions() {
    printf("\n\n########## TESTING ERROR CONDITIONS ##########\n");
    
    // Test NULL category name
    printf("--- Testing NULL category name ---\n");
    rdk_logger_ext_config_t config = {0};
    config.pCategoryName = NULL;
    config.appender = RDKLOG_OUTPUT_CONSOLE;
    config.layout = RDKLOG_FORMAT_WITH_DATETIME;
    config.loglevel = RDK_LOG_INFO;
    config.pFilePolicy = NULL;
    
    rdk_Error ret = rdk_logger_ext_init(&config);
    printf("Result with NULL category: %s\n", (ret == RDK_SUCCESS) ? "SUCCESS" : "FAILED");
    
    // Test file with NULL policy
    printf("--- Testing file appender with NULL policy ---\n");
    config.pCategoryName = "LOG.RDK.ERROR.TEST";
    config.appender = RDKLOG_OUTPUT_FILE;
    config.pFilePolicy = NULL;
    
    ret = rdk_logger_ext_init(&config);
    printf("Result with NULL file policy: %s\n", (ret == RDK_SUCCESS) ? "SUCCESS" : "FAILED");
    
    // Test invalid log directory
    printf("--- Testing invalid log directory ---\n");
    rdk_LogOutput* invalid_policy = create_file_policy("/invalid/nonexistent/directory", 
                                                          "test", 1024, 1);
    if (invalid_policy) {
        config.appender = RDKLOG_OUTPUT_FILE;
        config.pFilePolicy = invalid_policy;
        
        ret = rdk_logger_ext_init(&config);
        printf("Result with invalid directory: %s\n", (ret == RDK_SUCCESS) ? "SUCCESS" : "FAILED");
        
        free_file_policy(invalid_policy);
    }
}

// Test basic RDK_LOGGER_INIT functionality
void test_basic_init() {
    printf("\n\n########## TESTING BASIC INIT ##########\n");
    
    // Test basic logger init (should already be done, but test again)
    rdk_Error ret = RDK_LOGGER_INIT();
    printf("Basic RDK_LOGGER_INIT result: %s\n", (ret == RDK_SUCCESS) ? "SUCCESS" : "FAILED");
    
    // Test basic logging without extended init
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.BASIC", "Basic logging test message\n");
}

int main(int argc, char* argv[]) {
    printf("RDK Logger Test Application\n");
    printf("===========================\n");
    
    // First initialize the basic logger
    rdk_Error ret = RDK_LOGGER_INIT();
    if (ret != RDK_SUCCESS) {
        printf("Failed to initialize RDK logger\n");
        return 1;
    }
    
    if (argc > 1) {
        // Run specific test
        if (strcmp(argv[1], "basic") == 0) {
            test_basic_init();
        } else if (strcmp(argv[1], "stdout") == 0) {
            test_stdout_appender();
        } else if (strcmp(argv[1], "file") == 0) {
            test_file_appender();
        } else if (strcmp(argv[1], "levels") == 0) {
            test_log_level_filtering();
        } else if (strcmp(argv[1], "rotation") == 0) {
            test_file_rotation();
        } else if (strcmp(argv[1], "modules") == 0) {
            test_multiple_modules();
        }
        else if (strcmp(argv[1], "errors") == 0) {
            test_error_conditions();
        } else {
            printf("Unknown test: %s\n", argv[1]);
            printf("Available tests: basic, stdout, file, levels, rotation, modules, errors\n");
            return 1;
        }
    } else {
        // Run all tests
        test_basic_init();
        test_stdout_appender();
        test_file_appender();
        test_log_level_filtering();
        test_file_rotation();
        test_multiple_modules();
        test_error_conditions();
    }
    
    // Cleanup
    rdk_logger_deinit();
    
    printf("\n\nTest completed. Check output files in /tmp/rdk_logger_test/ and /tmp/rdk_logger_rotation_test/\n");
    return 0;
}
