#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <gtest/gtest.h>
#include "rdk_logger.h"
#include "log4c.h"
#include "test_utils.h"

class RdkLoggerExtInit : public ::testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
};

TEST_F(RdkLoggerExtInit, CreatesAppenderAndSetsLevel) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "gtest_rdkunittest.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024;
            testPolicy.fileCountMax = 3;
            rdk_logger_ext_config_t cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.loglevel = RDK_LOG_TRACE;
            cfg.output = RDKLOG_OUTPUT_FILE;
            cfg.format = RDKLOG_FORMAT_WITH_TS;
            cfg.pFilePolicy = &testPolicy;
            rdk_Error ret = rdk_logger_ext_init(&cfg);
            ASSERT_EQ(ret, RDK_SUCCESS) << "rdk_logger_ext_init failed";


            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", testPolicy.fileLocation, testPolicy.fileName);


            log4c_appender_t* app = log4c_appender_get(fullpath);
            ASSERT_NE(app, nullptr) << "Appender not created by rdk_logger_ext_init: " << fullpath;

            for (int i = 0; i < 50; i++)
            {
                RDK_LOG(RDK_LOG_TRACE, "LOG.RDK.RTMESSAGE", "trace-logging\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RTMESSAGE", "debug-logging\n");
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.TEST", "warn-logging\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "info-logging\n");
            }
    });
}


TEST_F(RdkLoggerExtInit, StdoutAppenderAndLayout) {
    RUN_IN_FORK({
            rdk_logger_ext_config_t cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.loglevel = RDK_LOG_DEBUG;
            cfg.output = RDKLOG_OUTPUT_CONSOLE;
            cfg.format = RDKLOG_FORMAT_PLAINTEXT;
            cfg.pFilePolicy = NULL;
            cfg.pModuleName = (char*)"LOG.RDK";
            rdk_Error ret = rdk_logger_ext_init(&cfg);
            ASSERT_EQ(ret, RDK_SUCCESS) << "rdk_logger_ext_init failed for Stdout";
            char appender_name[128];
            const char* category_name = cfg.pModuleName ? cfg.pModuleName : "LOG.RDK";
            snprintf(appender_name, sizeof(appender_name), "%s.stdout", category_name);
            log4c_appender_t* app = log4c_appender_get(appender_name);
            ASSERT_NE(app, nullptr) << "Stdout appender not found";

            for (int i = 0; i < 50; i++)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RTMESSAGE", "error-logging\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RTMESSAGE", "debug-logging\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "info-logging\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TEST", "debug-logging\n");
            }
    });
}


TEST_F(RdkLoggerExtInit, ComcastDatedViaExtInit) {
    RUN_IN_FORK({
            rdk_LogOutput_File testPolicy;
            strncpy(testPolicy.fileName, "gtest_comcast_unittest.log", sizeof(testPolicy.fileName)-1);
            testPolicy.fileName[sizeof(testPolicy.fileName) - 1] = '\0';
            strncpy(testPolicy.fileLocation, "/tmp", sizeof(testPolicy.fileLocation)-1);
            testPolicy.fileLocation[sizeof(testPolicy.fileLocation) - 1] = '\0';
            testPolicy.fileSizeMax = 1024;
            testPolicy.fileCountMax = 2;
            rdk_logger_ext_config_t cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.loglevel = RDK_LOG_ERROR;
            cfg.output = RDKLOG_OUTPUT_FILE;
            cfg.format = RDKLOG_FORMAT_DETAIL_WITH_TS;
            cfg.pFilePolicy = &testPolicy;

            rdk_Error ret = rdk_logger_ext_init(&cfg);
            ASSERT_EQ(ret, RDK_SUCCESS) << "rdk_logger_ext_init failed for Comcast layout";

            for (int i = 0; i < 50; i++)
            {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.RTMESSAGE", "error-logging\n");
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.RTMESSAGE", "debug-logging\n");
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "info-logging\n");
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TEST", "debug-logging\n");
            }
    });
}
