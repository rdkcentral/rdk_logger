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


#include <rdk_debug.h>
#include <rdk_debug_priv.h>

#include <string.h> // memset
#include <rdk_utils.h>
#include <stdarg.h>

extern int global_count;

/**
 * @brief Touch the file which can be used to check whether to log or not.
 *
 * @param[in] pszFile Character string representing name of the file to be created.
 * @return None.
 */
static void TouchFile(const char * pszFile)
{
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            fclose(fp);
        }
    }
}

/**
 * @brief Dump the debug log. It will Dump all the current settings so that an analysis of a log
 * file will include what logging information to expect.
 *
 * @param[in] path Character string representing path of the temp file to be created.
 * @return None.
 */
void rdk_dbgDumpLog(const char* path)
{
    rdk_dbg_priv_DumpLogConfig(path);

    TouchFile(path);
}

/**
 * @brief Initialize the underlying MPEOS debug support. This API must be called only once per boot cycle.
 * @return None.
 */
static rdk_logger_Bool inited = FALSE;
void rdk_dbgInit()
{

    if (!inited)
    {
        rdk_dbg_priv_Init();
        inited = TRUE;
    }
}

void rdk_dbgDeinit()
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
void rdk_logger_msg_printf(rdk_LogLevel level, const char *module,
        const char *format, ...)
{
    int num;
    va_list args;

    va_start(args, format);
    rdk_debug_priv_log_msg( level, module,
                format, args);
    va_end(args);
}

void rdk_logger_msg_vsprintf(rdk_LogLevel level, const char *module,
        const char *format, va_list args)
{
    rdk_debug_priv_log_msg( level, module,
                format, args);
}

