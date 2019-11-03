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

/* ************************************************************************************************* *
 * Project:		        coreNoStopRTOS/www.......                                                    *
 * Name:				init.h                                                                       *
 * Created:			    2019-09-27                                                                   *
 * Originator:			Jonas Bjurel                                                                 *
 * Short description:	Provides a FreeRtos shim-layer for nearly non-stop real time systems with    *
 *					    the prime use-case for model air-craft on-board systems.                     *
 * Resources:           github, WIKI .....                                                           *
 * ************************************************************************************************* */

#ifndef arduino_HEADER_INCLUDED
	#define arduino_HEADER_INCLUDED
	#include <Arduino.h>
#endif

#ifndef init_HEADER_INCLUDED
#define init_HEADER_INCLUDED
    #include "log.h"

	struct task_resource_t {
		task_resource_t* prevResource_p;
		void* taskResource_p;
		task_resource_t* nextResource_p;
	};

/* ********************************************************************************************* */
/* Static task parameters                                                                        */
/* Description: Defines static tasks parameters                                                  */
/*              Defined parameters such as task name, affinity, etc..                            */
/*				As well as dynamic parameters for statistics etc.                                */
/*                                                                                               */
	struct task_desc_t {
		// Task definitions
		TaskFunction_t pvTaskCode;
		char* pcName; // A descriptive name for the task
		uint32_t usStackDepth; //The size of the task stack specified as the number of bytes [1024]
		void* pvParameters; //Pointer that will be used as the parameter for the task being created
		UBaseType_t uxPriority; //The priority at which the task should run. 0-3 where 3 is the highest
		TaskHandle_t pvCreatedTask; //Created task handle
		BaseType_t xCoreID; //Afinity to core (0|1|tskNO_AFFINITY)
		uint32_t WatchdogTimeout; //Watchdog timeout in ms*10, 0 means no watch dog
		uint8_t watchdogHighWatermarkWarnPerc; // Warn watchdog high watermark in % [60]
		uint8_t stackHighWatermarkWarnPerc; // Warn watchdog high watermark in % [60]
		uint32_t heapHighWatermarkKBWarn; // Warn level for the heap in KB.
		uint8_t escalationRestartCnt; // Number of task restarts escalating to a system restart, 0 means never [3]

		// Dynamic task run-time data
		uint32_t watchDogTimer; // Watchdog ticks/runs
		SemaphoreHandle_t taskWatchDogMutex; // Recusrsive mutex handle with task scope
		bool watchdogWarn; // The watchdog counter has reached warning levels
		uint32_t watchdogHighWatermarkAbs; // The maximum watchdog count ever experienced, latched.
		uint32_t watchdogHighWatermarkWarnAbs; // Calculated watchdog warning watermark
		uint32_t heapCurr; //Current heap usage (Bytes)
		uint32_t heapHighWatermarkAbs; // The maximum heap usage watermark observed 
		int heapHighWatermarkWarnAbs; //Calculated heap usage warn watermark
		bool heapWarn; // The heap usage has reached warn levels
		int stackHighWatermarkAbs; // The maximum stack usage high watermark ever observed 
		int stackHighWatermarkWarnAbs; // Calculated stack usage warn watermark
		uint8_t restartCnt; // Number of task restarts since system restart
		task_resource_t* dynMemObjects_p; // List of allocated heap objects to be released at task restart/crash
	};

	
/* ********************************************************************************************* */
/* Init system parameters                                                                        */
/* Description: System parameters defining resource usage and scalability metrics	             */
/*                                                                                               */
	#define MAX_NO_OF_INIT_FUNCTIONS 32         // Maximum number of user init functions
	#define MAX_NO_OF_STATIC_TASKS 64           // Maximum number of user static tasks
    #define _TASK_WATCHDOG_MS_ 10               // ms between init watchdog runs
	#define _WATCHDOG_DISABLE_ UINT32_MAX       //Applied to task_desc_t.watchDogTimer to temporallily 
	                                            //disable the watchdog
    #define _WATCHDOG_DISABLE_MONIT_ UINT32_MAX //Used in startStaticTask(...) and 
	                                            //startDynamicTask(...) to disable watchdog, but
	                                            //still monitor the task.
    #define _WATCHDOG_DISABLE_NO_MONIT_ 0       //Used in startStaticTask(...) and 
	                                            //startDynamicTask(...) to disable watchdog, and
	                                            //not monitor the task.
	extern char logstr[320];		            //TODO: Move to log.c and log.h

/* ********************************************************************************************* */
/* init methods Return codes                                                                     */
/* Description: Following return codes are used by init methods                 	             */
/*                                                                                               */
	#define _TASKS_OK_ 0                   // OK Return code
	#define _TASK_EXIST_ 0                 // The task queried exist
	#define _TASKS_RESOURCES_EXHAUSTED_ 1  // System resources exhausted, the operation failed
	#define _TASKS_API_PARAMETER_ERROR_ 2  // API parameter error, the operation failed
	#define _TASKS_GENERAL_ERROR_ 3        // A general , the operation failed
	#define _TASKS_NOT_FOUND_ 4            // The Task could not be found, the operation failed
	#define _TASK_DELETED_ 4               // The task queried has been deleted
	#define _TASKS_MUTEX_TIMEOUT_ 5        // The mutex couldn't be taken within given time-out
										   // specified, the operation failed

/* ********************************************************************************************* */
/* tasks class definitions                                                                       */
/* Type: Platform Helpers                                                                        */
/* Description: TODO	                                                                         */
/*                                                                                               */
	class tasks {
/*  Public tasks class methods:                                                                  */
	public:
		tasks(); // Clas object constructor
		~tasks(); // Destructor not supported, if called it will cause a system panic
		static void startInit(void); // Initializing and starting init, which will call user defined init functions
		void kickTaskWatchdogs(task_desc_t* task_p); // User task method to kick the watchdog
		void taskPanic(task_desc_t* task); // User initialized task panic, causing task garbage collection and possibly a task restart
		void* taskMalloc(task_desc_t* task_p, int size); // Static tasks method for allocating init supervised memory
		uint8_t taskMfree(task_desc_t* task_p, void* mem_p); //Static tasks method for freeing init supervised memory
		uint8_t getTidByTaskDesc(uint8_t* tid, task_desc_t* task); // Get init Task ID (TID) by task handle
		uint8_t startStaticTask(TaskFunction_t taskFunc, const char* taskName, void* taskParams, uint32_t stackDepth, UBaseType_t taskPrio,
								BaseType_t taskPinning, uint32_t watchdogTimeout, uint8_t watchdogWarnPerc, uint8_t stackWarnPerc,
								uint32_t heapWarnKB, uint8_t restartEscalationCnt); //Start a static task - monitored by init
		TaskHandle_t startDynamicTask(TaskFunction_t taskFunc, char taskName[20], void* taskParams, uint32_t stackDepth, UBaseType_t taskPrio,
									  BaseType_t taskPinning); // Start a dynamic task, not monitored by init

/*  Private tasks class variables & methods:                                                     */
	private:
		//private tasks class variables:
		static void* initFunc[MAX_NO_OF_INIT_FUNCTIONS]; //List of init functions called at system restart
		static bool tasksObjExist; //Only one tasks object may be instantiated
		static bool schedulerRunning; // Is the scheduler running?
		static task_desc_t* classTasktable[MAX_NO_OF_STATIC_TASKS]; //Static Task/TID table
		static SemaphoreHandle_t globalInitMutex; //Global mutex handle
		static TimerHandle_t watchDogTimer; //Watchdog timer handle
		static uint8_t noOfTasks; //Number of registered static tasks
		static uint32_t watchdogOverruns; //Number of watchdog overruns (skipped watchdog runs)
		static uint32_t watchdogTaskCnt; //Number of watchdog runs
		static uint32_t hunderedMsCnt; //System uptime in 100 ms intervals
		static uint32_t secCnt; //System uptime in seconds
		static uint32_t tenSecCnt; //System uptime in ten second intervals
		static uint32_t minCnt ; //System uptime in minutes
		static uint32_t hourCnt; //System uptime in hours
		static SemaphoreHandle_t watchdogMutex;
		//private methods:
		uint8_t startTaskByName(char* pcName, bool incrementRestartCnt, bool initiateStat); // pcName: The human reading task name, initiateStat:[_TASKS_INITIATE_STATS_ | _TASKS_DONOT_INITIATE_STATS_]
		uint8_t startTaskByTid(uint8_t tid, bool incrementRestartCnt, bool initiateStat);	// tid: task identifier/tasks index, initiateStat:[_TASKS_INITIATE_STATS_ | _TASKS_DONOT_INITIATE_STATS_]
		uint8_t stopTaskByName(char* pcName);												// pcName: The human reading task name
		uint8_t stopTaskByTid(uint8_t tid);													// tid: task identifier/tasks index,
		uint8_t getTaskDescByName(char* pcName, task_desc_t** task);						// pcName: The human reading task name , task* pointer to resulting task descriptor
		uint8_t getTaskDescByTID(uint8_t tid, task_desc_t** task);							// Get task desc by static task tid
		uint8_t getStalledTasks(uint8_t* tid, task_desc_t* task);							// Find stalled tasks, returns for every stalled task, to continue the search a new search with the subsequent TID must me called PROBABLY DOESNT WORK IN CURREN VARIAN **tid should be provided
		uint8_t startAllTasks(bool incrementRestartCnt, bool initiateStat);					//Start all registered task, optionally increment the RestartCnt, or initialize the dynamic task dynamic/statistics data.
		uint8_t stopAllTasks();																//Stop all static tasks, and perform all necessary garbage collection.
		uint8_t clearStats(task_desc_t* task);												// Clear dynamic and statistics data
		uint8_t startTaskByTaskDesc(task_desc_t* task, bool incrementRestartCnt, bool initiateStat); // Start static task by task* description. 
		uint8_t stopTaskByTaskDesc(task_desc_t* task);										// Stop static task by task* description. 
		uint8_t checkIfTaskAlive(task_desc_t* task);										// Check if static task by task* description is alive. 
		uint8_t globalInitMutexTake(TickType_t timeout = portMAX_DELAY);					//Take a global initd task class mutex.
		uint8_t globalInitMutexGive();														//Give a global initd task class mutex.
		uint8_t taskInitMutexTake(task_desc_t* task, TickType_t timeout = portMAX_DELAY);	//Take a task scope mutex with an optional timeout.
		uint8_t taskInitMutexGive(task_desc_t* task);										//Give a task scope mutex.
		static void runTaskWatchdogs(TimerHandle_t xTimer);									// Task watchdog repetive runs.
		uint8_t taskResourceLink(task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p); //Link in an arbitrary resource to arbitrary resource list.
		uint8_t taskResourceUnLink(task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p); //Un-ink an arbitrary resource from an arbitrary resource list.
		uint8_t findResourceDescByResourceObj(void* taskResourceObj_p, task_resource_t* taskResourceRoot_p, task_resource_t** taskResourceDesc_pp);
		uint8_t taskMfreeAll(task_desc_t* task_p);											// Free all memory chunks dynamically allocated by a static task having used "taskMalloc"
		void incHeapStats(task_desc_t* task_p, int size);									// Increase heap usage statistics for a static Task
		void decHeapStats(task_desc_t* task_p, int size);									// Decrease heap usage statistics for a static Task
	};
	extern tasks initd;

#endif
