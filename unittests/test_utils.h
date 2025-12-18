#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <gtest/gtest.h>
#include <unistd.h>
#include <sys/wait.h>

// Macro to run test code in a forked process
#define RUN_IN_FORK(test_body) \
    pid_t pid = fork(); \
    ASSERT_NE(pid, -1) << "fork failed"; \
    if (pid == 0) { \
        log4c_init(); \
        test_body; \
        rdk_logger_deinit(); \
        exit(::testing::Test::HasFailure() ? 1 : 0); \
    } else { \
        int status = 0; \
        waitpid(pid, &status, 0); \
        ASSERT_TRUE(WIFEXITED(status)); \
        if (WEXITSTATUS(status) != 0) { \
            FAIL() << "Child process failed with exit code " << WEXITSTATUS(status); \
        } \
    }

#endif // TEST_UTILS_H
