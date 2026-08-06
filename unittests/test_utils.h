#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <gtest/gtest.h>
#include <unistd.h>
#include <sys/wait.h>
#include "log4c.h"
#include "rdk_logger.h"

// Macro to run test code in a forked process
#define RUN_IN_FORK(test_body) \
    do { \
        pid_t pid = fork(); \
        ASSERT_NE(pid, -1) << "fork failed"; \
        if (pid == 0) { \
            log4c_init(); \
            test_body; \
            log4c_fini();  \
            exit(0); \
        } else { \
            int status = 0; \
            waitpid(pid, &status, 0); \
            ASSERT_TRUE(WIFEXITED(status)); \
            ASSERT_EQ(WEXITSTATUS(status), 0); \
        } \
    } while (0)
#endif // TEST_UTILS_H
