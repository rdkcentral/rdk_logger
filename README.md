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
LOG.RDK.DEFAULT=WARNING
# All modules not listed below will log WARN, ERROR, and FATAL messages

LOG.RDK.SI=INFO
# LOG.RDK.SI will log INFO, NOTICE, WARN, ERROR, and FATAL messages

LOG.RDK.FOO = NONE
#All logs will be disabled for this module

## Runtime Control

Use the `rdklogctrl` utility to change log levels for modules at run time.
