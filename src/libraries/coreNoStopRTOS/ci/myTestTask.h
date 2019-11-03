/* ************************************************************************************************* *
 * (c) Copyright 2019 Jonas Bjurel (jonasbjurel@hotmail.com)                                         *
 *                                                                                                   *
 * Licensed under the Apache License, Version 2.0 (the "License");                                   *
 * you may not use this file except in compliance with the License.                                  *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0                *
 *                                                                                                   *
 * Unless required by applicable law or agreed to in writing, software distributed under the License *
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express- *
 * or implied.                                                                                       *
 * See the License for the specific language governing permissions and limitations under the         *
 * licence.                                                                                          *
 *                                                                                                   *
 * ************************************************************************************************* */

 /* ************************************************************************************************ *
  * Project:			 coreNoStopRTOS/www.......                                                   *
  * Name:				 myTestTask.h                                                                *
  * Created:			 2019-09-27                                                                  *
  * Originator:		     Jonas Bjurel                                                                *
  * Short description:   Provides CI (Continous Integration) test stimuli for coreNoStopRTOS         *
  *                      functions...                                                                *
  * Resources:           github, WIKI .....                                                          *
  * ************************************************************************************************ */

#ifndef arduino_HEADER_INCLUDED
	#define arduino_HEADER_INCLUDED
	#include <Arduino.h>
#endif



#ifndef myTestTask_HEADER_INCLUDED
    #include "../src/log.h"
    #include "../src/init.h"
	#define  myTestTask_HEADER_INCLUDED
	void myTestTaskInit(void);
	void myStaticTestTask(task_desc_t* myTask);
	void myDynamicTestTask(char* taskName);
#endif
