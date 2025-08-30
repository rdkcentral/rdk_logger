/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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

/**
 * @file rdk_debug.c
 * This source file contains the APIs for RDK debug.
 */

/**
* @defgroup rdk_logger
* @{
* @defgroup src
* @{
**/


#include <rdk_logger.h>
#include <rdk_debug_priv.h>

#include <string.h> // memset
#include <rdk_utils.h>
#include <stdarg.h>

/**
 * @brief Initialize the underlying MPEOS debug support. This API must be called only once per boot cycle.
 * @return None.
 */
static rdk_logger_Bool inited = FALSE;
void rdk_dbg_init()
{

    if (!inited)
    {
        rdk_dbg_priv_init();
        inited = TRUE;
        rdk_dbg_priv_config();
    }
}

void rdk_dbg_deinit()
{
  if (inited)
  {
      inited = FALSE;
  }
}

/**
 * @brief Send a debugging message to the debugging window. It is appended to the log output based
 * on configurations set in the environment file.
 *
 * @param[in] level The debug logging level.
 * @param[in] module The name of the module for which this message belongs to, it is mentioned in debug.ini.
 * @param[in] format Printf style string containing the log message.
 */
void rdk_logger_msg_printf(rdk_LogLevel level, const char *module, const char *format, ...)
{
    int num;
    va_list args;

    va_start(args, format);
    rdk_dbg_priv_log_msg(level, module, format, args);
    va_end(args);
}

void rdk_logger_msg_vsprintf(rdk_LogLevel level, const char *module, const char *format, va_list args)
{
    rdk_dbg_priv_log_msg(level, module, format, args);
}

/**
 * @brief Function to sets a specific log level of a module.
 *
 * @param[in] module The module name or category for for which the log level shall be checked
 * @param[in] level The debug logging level.
 *
 * @return Returns TRUE, if debug log level enabled successfully else returns FALSE.
 */
rdk_logger_Bool rdk_logger_enable_logLevel(const char *pModuleName, rdk_LogLevel logLevel, rdk_logger_Bool enableLogLvl)
{
    if (!pModuleName)
        return FALSE;

    const char* logLevelName = rdk_loglevelToString(logLevel, enableLogLvl);

    if (!logLevelName)
        return FALSE;

    rdk_dbg_priv_reconfig (pModuleName, logLevelName);
    return TRUE;
}

