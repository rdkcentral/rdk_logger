# RDK Logger Migration Guide

## Overview

This guide helps developers migrate from other logging systems to RDK Logger and upgrade existing RDK Logger implementations.

## Migration from printf/fprintf

### Before (Legacy Code)
```c
#include <stdio.h>

int main() {
    printf("Application started\n");
    fprintf(stderr, "Error: Connection failed\n");
    printf("Debug: value = %d\n", value);
    return 0;
}
```

### After (RDK Logger)
```c
#include "rdk_logger.h"

#define MODULE_NAME "LOG.RDK.MYAPP"

int main() {
    // Initialize logger
    RDK_LOGGER_INIT();
    
    RDK_LOG(RDK_LOG_NOTICE, MODULE_NAME, "Application started\n");
    RDK_LOG(RDK_LOG_ERROR, MODULE_NAME, "Error: Connection failed\n");
    RDK_LOG(RDK_LOG_DEBUG, MODULE_NAME, "Debug: value = %d\n", value);
    
    rdk_logger_deinit();
    return 0;
}
```

## Migration from syslog

### Before (syslog)
```c
#include <syslog.h>

int main() {
    openlog("myapp", LOG_PID, LOG_USER);
    
    syslog(LOG_INFO, "Application started");
    syslog(LOG_ERR, "Connection failed: %s", strerror(errno));
    syslog(LOG_DEBUG, "Processing item %d", item_id);
    
    closelog();
    return 0;
}
```

### After (RDK Logger)
```c
#include "rdk_logger.h"

#define MODULE_NAME "LOG.RDK.MYAPP"

int main() {
    RDK_LOGGER_INIT();
    
    RDK_LOG(RDK_LOG_INFO, MODULE_NAME, "Application started\n");
    RDK_LOG(RDK_LOG_ERROR, MODULE_NAME, "Connection failed: %s\n", strerror(errno));
    RDK_LOG(RDK_LOG_DEBUG, MODULE_NAME, "Processing item %d\n", item_id);
    
    rdk_logger_deinit();
    return 0;
}
```

### Log Level Mapping

| syslog Level | RDK Logger Level | Description |
|--------------|------------------|-------------|
| LOG_EMERG    | RDK_LOG_FATAL    | System unusable |
| LOG_ALERT    | RDK_LOG_FATAL    | Action must be taken immediately |
| LOG_CRIT     | RDK_LOG_FATAL    | Critical conditions |
| LOG_ERR      | RDK_LOG_ERROR    | Error conditions |
| LOG_WARNING  | RDK_LOG_WARN     | Warning conditions |
| LOG_NOTICE   | RDK_LOG_NOTICE   | Normal but significant condition |
| LOG_INFO     | RDK_LOG_INFO     | Informational messages |
| LOG_DEBUG    | RDK_LOG_DEBUG    | Debug-level messages |

## Migration from Custom Logging

### Wrapper Approach
If you have existing custom logging macros, you can create wrapper functions:

```c
// Legacy logging system
#define OLD_LOG_ERROR(fmt, ...) old_log_function(ERROR_LEVEL, fmt, ##__VA_ARGS__)
#define OLD_LOG_INFO(fmt, ...)  old_log_function(INFO_LEVEL, fmt, ##__VA_ARGS__)

// Migration wrapper
#define LOG_ERROR(fmt, ...) RDK_LOG(RDK_LOG_ERROR, MODULE_NAME, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  RDK_LOG(RDK_LOG_INFO, MODULE_NAME, fmt, ##__VA_ARGS__)

// Now replace OLD_LOG_* with LOG_* throughout your codebase
```

### Gradual Migration
You can run both systems in parallel during migration:

```c
#include "rdk_logger.h"
#include "old_logger.h"

#define DUAL_LOG(level, old_level, fmt, ...) do { \
    RDK_LOG(level, MODULE_NAME, fmt, ##__VA_ARGS__); \
    old_log_function(old_level, fmt, ##__VA_ARGS__); \
} while(0)

// Use DUAL_LOG during migration, then switch to RDK_LOG
```

## Configuration Migration

### Converting Log Levels
Create a migration script to convert existing configuration:

```bash
#!/bin/bash
# migrate_config.sh

# Convert old format to debug.ini format
old_config="/etc/old_logger.conf"
new_config="/etc/debug.ini"

echo "# Migrated configuration" > $new_config
echo "LOG.RDK.DEFAULT=WARNING" >> $new_config

# Parse old configuration and convert
while IFS='=' read -r module level; do
    case $level in
        "error")   new_level="ERROR" ;;
        "warning") new_level="WARN" ;;
        "info")    new_level="INFO" ;;
        "debug")   new_level="DEBUG" ;;
        *)         new_level="NOTICE" ;;
    esac
    echo "LOG.RDK.$module=$new_level" >> $new_config
done < $old_config
```

## Advanced Migration Patterns

### Conditional Compilation
Support both old and new logging during transition:

```c
#ifdef USE_RDK_LOGGER
    #include "rdk_logger.h"
    #define APP_LOG(level, fmt, ...) RDK_LOG(level, MODULE_NAME, fmt, ##__VA_ARGS__)
    #define APP_LOG_INIT() RDK_LOGGER_INIT()
    #define APP_LOG_CLEANUP() rdk_logger_deinit()
#else
    #include "old_logger.h"
    #define APP_LOG(level, fmt, ...) old_log(level, fmt, ##__VA_ARGS__)
    #define APP_LOG_INIT() old_log_init()
    #define APP_LOG_CLEANUP() old_log_cleanup()
#endif

int main() {
    APP_LOG_INIT();
    APP_LOG(RDK_LOG_INFO, "Application started\n");
    // ... application code ...
    APP_LOG_CLEANUP();
    return 0;
}
```

### Performance Comparison
Test performance impact during migration:

```c
#include <time.h>

void performance_test() {
    clock_t start, end;
    int iterations = 10000;
    
    // Test RDK Logger performance
    start = clock();
    for (int i = 0; i < iterations; i++) {
        RDK_LOG(RDK_LOG_DEBUG, MODULE_NAME, "Test message %d\n", i);
    }
    end = clock();
    
    double rdk_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("RDK Logger: %f seconds for %d messages\n", rdk_time, iterations);
}
```

## Common Migration Issues

### Issue 1: Missing Newlines
**Problem**: RDK Logger expects newlines in format strings.
```c
// Wrong
RDK_LOG(RDK_LOG_INFO, MODULE_NAME, "Message");

// Correct
RDK_LOG(RDK_LOG_INFO, MODULE_NAME, "Message\n");
```

### Issue 2: Module Name Format
**Problem**: Incorrect module naming convention.
```c
// Wrong
RDK_LOG(RDK_LOG_INFO, "MYAPP", "Message\n");

// Correct
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.MYAPP", "Message\n");
```

### Issue 3: Initialization Order
**Problem**: Logging before initialization.
```c
// Wrong
int main() {
    RDK_LOG(RDK_LOG_INFO, MODULE_NAME, "Starting\n");  // Will fail
    RDK_LOGGER_INIT();
}

// Correct
int main() {
    RDK_LOGGER_INIT();
    RDK_LOG(RDK_LOG_INFO, MODULE_NAME, "Starting\n");
}
```

### Issue 4: Thread Safety
**Problem**: Initialization in multi-threaded environment.
```c
// Wrong - race condition
void thread_function() {
    RDK_LOGGER_INIT();  // Multiple threads calling init
    // ...
}

// Correct - initialize once in main thread
int main() {
    RDK_LOGGER_INIT();
    
    // Create threads after initialization
    pthread_create(&thread, NULL, thread_function, NULL);
}
```

## Testing Migration

### Validation Script
```bash
#!/bin/bash
# validate_migration.sh

echo "Testing RDK Logger migration..."

# Test 1: Configuration file exists
if [ ! -f /etc/debug.ini ]; then
    echo "ERROR: debug.ini not found"
    exit 1
fi

# Test 2: Module names follow convention
if ! grep -q "LOG.RDK." /etc/debug.ini; then
    echo "WARNING: No LOG.RDK modules found in config"
fi

# Test 3: Test application logging
./test_app > /tmp/test_output.log 2>&1

if grep -q "LOG.RDK" /tmp/test_output.log; then
    echo "SUCCESS: RDK Logger output detected"
else
    echo "ERROR: No RDK Logger output found"
    exit 1
fi

echo "Migration validation complete"
```

### Unit Test Example
```c
#include <assert.h>
#include "rdk_logger.h"

void test_migration() {
    // Test initialization
    assert(RDK_LOGGER_INIT() == RDK_SUCCESS);
    
    // Test logging levels
    assert(rdk_dbg_enabled("LOG.RDK.TEST", RDK_LOG_ERROR) == TRUE);
    
    // Test cleanup
    assert(rdk_logger_deinit() == RDK_SUCCESS);
    
    printf("Migration tests passed\n");
}
```

## Migration Checklist

- [ ] Review existing logging calls and categorize by log level
- [ ] Create module name mapping (old names → LOG.RDK.* names)
- [ ] Update configuration files to debug.ini format
- [ ] Replace logging function calls with RDK_LOG macros
- [ ] Add RDK_LOGGER_INIT() and rdk_logger_deinit() calls
- [ ] Update build system to link RDK Logger libraries
- [ ] Test logging output and configuration
- [ ] Validate runtime log level changes work
- [ ] Update documentation and deployment scripts
- [ ] Train team on new logging system usage

## Post-Migration Optimization

### 1. Performance Tuning
```c
// Use conditional logging for expensive debug operations
if (rdk_dbg_enabled(MODULE_NAME, RDK_LOG_DEBUG)) {
    char *debug_dump = expensive_debug_function();
    RDK_LOG(RDK_LOG_DEBUG, MODULE_NAME, "Debug: %s\n", debug_dump);
    free(debug_dump);
}
```

### 2. Configuration Optimization
```ini
# Production config - minimal logging
LOG.RDK.DEFAULT=ERROR

# Development config - detailed logging
LOG.RDK.DEFAULT=DEBUG
LOG.RDK.NETWORK=TRACE
LOG.RDK.DATABASE=DEBUG
```

### 3. Monitoring Integration
```c
// Add metrics for log message rates
void log_with_metrics(rdk_LogLevel level, const char *module, 
                      const char *format, ...) {
    static int log_count = 0;
    log_count++;
    
    if (log_count % 1000 == 0) {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.METRICS", 
                "Logged %d messages\n", log_count);
    }
    
    va_list args;
    va_start(args, format);
    rdk_logger_msg_vsprintf(level, module, format, args);
    va_end(args);
}
```

This migration guide provides a comprehensive approach to transitioning to RDK Logger while maintaining system stability and performance.