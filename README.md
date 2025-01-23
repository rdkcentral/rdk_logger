Overview:
---------

The rdklogger is a general-purpose logging mechanism that utilizes log4c for formatting and supports multiple log levels and modules. The log level for each module is read from a configuration file (debug.ini, typically located at /etc/debug.ini) at startup, where log modules and their enabled log levels are defined.

rdklogger supports overriding the configuration at runtime by providing a persistent path to files like /opt/debug.ini or /nvram/debug.ini. It includes a CLI utility called rdklogctrl, which is used to dynamically change log levels for modules at runtime.

Internally, rdklogger leverages log4c for its formatting, log writing, and log rotation capabilities. One unique feature of rdklogger is that you do not set a single severity level above which no logs with greater severity would be logged. Instead, each level (ERROR, WARN, INFO, DEBUG, etc.) can be independently enabled or disabled. This means you could, for example, disable all ERROR logs while enabling all DEBUG logs.
