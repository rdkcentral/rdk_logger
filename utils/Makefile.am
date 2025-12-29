##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = rdklogctrl

rdklogctrl_SOURCES=rdklogctrl.c
rdklogctrl_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -I./usr/include
rdklogctrl_LDFLAGS = -L/usr/lib

bin_PROGRAMS       += rdkLogMileStone
rdkLogMileStone_SOURCES = rdklogmilestone.c $(top_srcdir)/src/rdk_logger_milestone.c
rdkLogMileStone_CFLAGS = -I$(top_srcdir)/include
rdkLogMileStone_LDFLAGS = -L/usr/lib


bin_PROGRAMS            += onboarding_log
onboarding_log_SOURCES  = rdk_logger_onboard_main.c $(top_srcdir)/src/rdk_logger_onboard.c
onboarding_log_CFLAGS   = -I$(top_srcdir)/include
onboarding_log_LDFLAGS  = -L/usr/lib
