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
  * Name:				 mq.h                                                                        *
  * Created:			 2019-09-27                                                                  *
  * Originator:		     Jonas Bjurel                                                                *
  * Short description:   Provides a very basic FreeRTOS message queue based on FreeRtos queues       *
  * Resources:           github, WIKI .....                                                          *
  * ************************************************************************************************ */

#ifndef mq_HEADER_INCLUDED
#define mq_HEADER_INCLUDED
#include <Arduino.h>
#include "init.h"
#include "log.h"

// mQBuffer defines an unsorted linked list of messages posted to the message queue.
struct messageBuffer_t { 
	messageBuffer_t* prev;
	char* topic;
	uint8_t prio;
	TickType_t ttl;
	task_desc_t* senderTask_p;
	void* buff_p;
	messageBuffer_t* next;
};

// topicSubscriber_t defines a list of subscribers to a particular topic
struct topicSubscriber_t {
	struct topicSub_t* prev;
	mQ* mQ_p;
	struct topicSub_t* next;
};

// message_p_queue defines a list of message object pointers
struct message_p_queue {
	message_p_queue* prev;
	messageBuffer_t* messageObj_p;
	message_p_queue* next
};

class mQ {
public:
	mq();
	~mq();
	uint8_t* mQClientTopicCreate(int topic) {
	}
	uint_8 mQClientTopicSubscribe(int topic) {
		topicSubscriber_t* subscription_p = malloc(sizeof(topicSubscriber_t));
		subscription_p->mQ_p = this;
		if(uint8_t res = <topicSubscriber_t>linkInObj(topicSubscriberRoot_p, subscription_p, topic);
			// LOG and return error
	}

	struct messageBuffer_t {
		messageBuffer_t* prev;
		int topic;
		uint8_t prio;
		TickType_t ttl;
		TickType_t postTime
		task_desc_t* senderTask_p;
		void* buff_p;
		messageBuffer_t* next;
	}

	uint_8 mQClientTopicPost(int topic, uin8_t prio, int ttl, messageBuffer_t* buff_p, task_desc_t* myTask_p;) {
		mqMessageBuffer_p = mQ_allocMessageBuffer(); //recycle buffers as much as we can
		//Check API input parameters
		mqMessageBuffer_p->topic = topic;
		mqMessageBuffer_p->prio = prio;
		mqMessageBuffer_p->ttl = ttl;
		mqMessageBuffer_p->postTime = xTaskGetTickCount()
		mqMessageBuffer_p->senderTask_p = myTask_p;
		mqMessageBuffer_p->buff_p = buff_p;

		<topicSubscriber_t>linkInObj(topicSubscriberRoot_p, subscription_p, topic);
	}

private:
	static topicSubscriber_t*[MAX_MQ_TOPICS] topicSubscriberRoot_p;
	static messageBuffer_t* messageBufferRoot_p;
	message_p_queue*[_MAX_MQ_PRIORITIES_] incommingClientQ;

};


mQstatus_t* tasks::mQClientTopicGet(mQDesc_t* mQDesc_p, void* receivedTopic, topic = ) {

}


/* ********************************************************************************************* */
/* tasks class definitions                                                                       */
/* Type: Platform Helpers                                                                        */
/* Description: TODO	                                                                         */

/* tasks methods Return codes                                                                    */
#define _TASKS_OK_ 0
#define _TASKS_RESOURCES_EXHAUSTED_ 1
#define _TASKS_API_PARAMETER_ERROR_ 2
#define _TASKS_GENERAL_ERROR_ 3
#define _TASKS_NOT_FOUND_ 4
#define _TASKS_MUTEX_TIMEOUT_ 5

#define _TASK_EXIST_ 0
#define _TASK_DELETED_ 1



#endif