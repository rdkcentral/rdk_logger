# RDK Logger

RDK Logger is a general-purpose logging mechanism for RDK components. It supports multiple log levels and modules, with log levels configured via a configuration file (`debug.ini`). Each module can have its own log level, and these can be changed at runtime using the provided CLI utility.

## Features

- Abstracts logging client from underlying logging utility.
- Dynamically enables/disables logging level at run time.
- Provides a structured log format including timestamp, module name, log level, and message.
- Controls log level independently for each component/module.
- Enables logging globally via a single configuration value.
- Controls initial log level for each component/module from configuration file (`debug.ini`) at startup.
- Prints formatted data to stdout or log files.
- Supports separate log files for each module if configured.

## Configuration

Logging levels and modules are configured in `debug.ini`. If a module is not explicitly configured in `debug.ini`, it inherits the log level from `LOG.RDK.DEFAULT`.

See `debug.ini.sample` for an example configuration file.
The sample file includes comments explaining how to set log levels for each module and what each log level enables.

For example:
```
LOG.RDK.DEFAULT=WARNING
LOG.RDK.FOO=DEBUG
LOG.RDK.BAR=NONE
```

- The component `FOO` will print DEBUG, INFO, WARNING, ERROR, and FATAL messages
- The component `BAR` prints `nothing`
- All the other components prints WARNING and higher log levels like ERROR, and FATAL as defined by `DEFAULT`

## Runtime Control

Use the `rdklogctrl` utility to change log levels for modules at run time.

```bash
# Set log level for a module
rdklogctrl <process_name> <module_name> <log_level>

# Examples
rdklogctrl myapp LOG.RDK.NETWORK DEBUG
rdklogctrl receiver LOG.RDK.AUDIO INFO

# Available levels: FATAL, ERROR, WARN, NOTICE, INFO, DEBUG, TRACE, NONE
```

## Quick Start

### 1. Include Headers
```c
#include "rdk_logger.h"
```

### 2. Initialize and Use
```c
int main() {
    // Initialize logger
    if (RDK_LOGGER_INIT() != RDK_SUCCESS) {
        fprintf(stderr, "Failed to initialize RDK Logger\n");
        return -1;
    }
    
    // Log messages
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.MYAPP", "Application started\n");
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MYAPP", "Error: %s\n", strerror(errno));
    
    // Cleanup
    rdk_logger_deinit();
    return 0;
}
```

### 3. Build and Link
```bash
gcc myapp.c -lrdkloggers -llog4c -lglib-2.0
```

## 📚 Complete Documentation

This repository includes comprehensive documentation for all aspects of RDK Logger:

### 📖 [API Documentation](docs/API_DOCUMENTATION.md)
**Complete developer reference**
- Detailed API function documentation with parameters and return values
- Architecture overview and capabilities
- Configuration system and debug.ini format
- Runtime control mechanisms and utilities
- Performance optimization and best practices
- Complete real-world examples and usage patterns

### ⚡ [Quick Reference](docs/QUICK_REFERENCE.md)
**Essential information for daily use**
- Core function syntax and common patterns
- Log level hierarchy and usage guidelines
- Configuration snippets and examples
- Runtime control commands (rdklogctrl)
- Build system integration examples
- Debugging tips and test programs

### 🔄 [Migration Guide](docs/MIGRATION_GUIDE.md)
**Transitioning from other logging systems**
- Step-by-step migration from printf/fprintf, syslog, and custom logging
- Log level mapping between different systems
- Configuration conversion techniques
- Common migration pitfalls and solutions
- Validation scripts and testing approaches

### 🛠️ [Troubleshooting Guide](docs/TROUBLESHOOTING.md)
**Solutions for common issues**
- Initialization failures and configuration problems
- Missing log output debugging
- Runtime control issues
- Performance and memory problems
- Multi-threading complications
- Build and linking solutions

## 🚀 Getting Started Paths

### For New Users
1. **Read** the [Quick Start](#quick-start) section above
2. **Study** [API Documentation](docs/API_DOCUMENTATION.md) for comprehensive understanding
3. **Reference** [Quick Reference](docs/QUICK_REFERENCE.md) for daily syntax needs

### For Migrating from Other Systems
1. **Identify** your current logging system in [Migration Guide](docs/MIGRATION_GUIDE.md)
2. **Follow** the step-by-step migration process
3. **Validate** using provided migration scripts and tests

### For Troubleshooting
1. **Check** [Troubleshooting Guide](docs/TROUBLESHOOTING.md) for your specific issue
2. **Use** debugging techniques and tools provided
3. **Reference** [Quick Reference](docs/QUICK_REFERENCE.md) for correct syntax

## Log Levels

| Level | When to Use |
|-------|-------------|
| **FATAL** | System crash, unusable |
| **ERROR** | Operation failures |
| **WARN** | Potential problems |
| **NOTICE** | Important events |
| **INFO** | General information |
| **DEBUG** | Debugging details |
| **TRACE** | Function tracing |

## Build Dependencies

- **liblog4c-dev** - Log4C development headers
- **libglib2.0-dev** - GLib development headers
- **autotools** - For building from source

## Repository Structure

```
rdk_logger/
├── include/           # Header files (rdk_logger.h, etc.)
├── src/              # Source files
├── utils/            # Utility programs (rdklogctrl)
├── test/             # Test programs
├── unittests/        # Unit tests
├── debug.ini.sample  # Example configuration
└── docs/             # Comprehensive documentation
```
