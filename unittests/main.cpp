/*
 * If not stated otherwise in this file or this component's LICENSE file
 * the following copyright and licenses apply:
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
#include "gtest/gtest.h"

char gtestReportPath[] = "xml:/tmp/Gtest_Report/rbus_gtest_report.xml";

GTEST_API_ int main(int argc, char* argv[])
{
  ::testing::GTEST_FLAG(output) = gtestReportPath;
  ::testing::InitGoogleTest(&argc, argv);
  int ret = 0;

  ret = RUN_ALL_TESTS();

  if(ret)
      printf("Gtest returned with error : %d !!!\n",ret);

  return 0;
}
