#include "wireWrap.h" 


// Wire/I2c Channel assignment 
//static uint8_t chStatus a descriptor of active I2C channels [0..7] constructs pointing to constructs of [SDA,SCL,*users], where users is a linked list: [*prev, *current, *next]
// pointing at these constructs [wireWrap objHandle]

//Transaction queues
//7 prio queues with a [0..7] constructs for every prio, every queue has a linked [*prev, *current, *next] construct which points to constructs of work orders [Order status........]
// of every queue have a linked list of items in queue order: transactio....


/* ***********************************************************************************************
   initialization of static wireWrap class static data                                           */
ch wireWrap::channels[4] = { {_UNINIT, 0, 0, NULL}, {_UNINIT, 0, 0, NULL}, {_UNINIT, 0, 0, NULL}, {_UNINIT, 0, 0, NULL} };
transact* wireWrap::transactQueues[4][4] = { {NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL} };
uint8_t _maxPrioQueueDepth = 20;
uint8_t channelStatus = _UNINIT
/* ********************************************************************************************* */


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                  ***BEGIN***Class management functions***BEGIN***                             */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/* wireWrap class object constructor                                                             */
wireWrap::wireWrap() {
	wireMutex = xSemaphoreCreateRecursiveMutex();
}

/* ********************************************************************************************* */
/* wireWrap class object destructor                                                              */
// Todo
wireWrap::~wireWrap() {
	initd.taskPanic();
}
/* ********************************************************************************************* */
/*                      ***END***Class MANAGEMENT functions***END**                              */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                       ***BEGIN***Class user functions***BEGIN***                              */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/* wireWrap request channel init function                                                        */
/* Type: User                                                                                    */
/* Description: TODO    */

uint8_t wireWrap::wireInit(uint8_t sda, uint8_t scl, wireSpeed_t wireSpeed, wireMode_t wireMode = _HW_) {
	xSemaphoreTakeRecursive(globalWireMutex);
	if (wireProperties.wireState ~= _WW_WIRE_UNINIT_) {
		xSemaphoreGiveRecursive(globalWireMutex);
		return(_WW_RESOURCE_CONFLICT_);
	}
	if (noOfCreatedWires >= _MAX_NO_WIRES_ - 1) {
		xSemaphoreGiveRecursive(globalWireMutex);
		return(_WW_OUT_OF_RESOURCES_);
	}
	if (uint8_t res = createWire(uint8_t sda, uint8_t scl, wireSpeed_t wireSpeed, wireMode_t wireMode = _HW_)) {
		xSemaphoreGiveRecursive(globalWireMutex);
		return(res);
	}
	noOfCreatedWires++;
	this.wireProperties.wireState ~= _WW_WIRE_IDLE_
		xSemaphoreGiveRecursive(globalWireMutex);
	return(_WW_OK_);
}

/* ********************************************************************************************* */
/* wireStatus request wire status function                                                       */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */

wireState_t wireWrap::wireStatus() {
	return(wireProperties.wireState);
}

/* ********************************************************************************************* */
/* wireWrap request transaction function                                                         */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
void* wireWrap::requestWireTransact(wwWireWrapTransactQoS_t prio, TickType_t timeout, transactCallback* tcb, void* tcbParam, resultCallback* trcb, void* trcbParam); {
	void* transactDescriptor_p;
	xSemaphoreTakeRecursive(WireMutex);
	if (prio >= _MAX_I2C_PRIOS_ || tcb == NULL) {
		xSemaphoreGiveRecursive(WireMutex);
		return(NULL);
	}
	if (transactDescriptor_p = getTransactDescriptor() == NULL) {
		xSemaphoreGiveRecursive(WireMutex);
		return(NULL);
	}
	transactDescriptor_p->prio = prio;
	transactDescriptor_p->transactCallback_p = tcb;
	transactDescriptor_p->transactCallbackParam_p = tcbParam;
	transactDescriptor_p->transactResCallback_p = trcb;
	transactDescriptor_p->transactResCallbackParam_p = trcbParam;
	if (res = queueTransact(transactDescriptor_p)) {
		xSemaphoreGiveRecursive(WireMutex);
		return(NULL);
	}
	scheduleWireTransact();
	xSemaphoreGiveRecursive(WireMutex);
	return(transactDescriptor_p);
}

/* ********************************************************************************************* */
/* wireWrap abort transaction function                                                           */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
wireWrap::uint8_t wireAbortTransact(void* transactHandle_p) {
	xSemaphoreTakeRecursive(WireMutex);
	if (res = dequeueTransact(transactHandle)) {
		xSemaphoreGiveRecursive(WireMutex);
		return(res);
	}
	xSemaphoreGiveRecursive(WireMutex);
	return(_WW_OK_);
}

/* ********************************************************************************************* */
/* wireWrap Transaction status request                                                           */
/* Type: User                                                                                    */
/* Description: Provides status of an transaction (_TRANSACT_SCHED_, _TRANSACT_RUNNING_,         */
/*                                                 _TRANSACT_TIMEOUT_, _TRANSACT_NOEXIST_        */
/*                                                  Number in queue, and possibly max time       */
/*
uint8_t wireTransactStatus(void* transactHandle_p, transactStatus_t transactStatus_p, bool terse = false) {
	
}
*/

/* ********************************************************************************************* */
/* wireWrap write function                                                                       */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::write(uint8_t baseAddr, uint8_t* data_p, uint8_t size = 1) {
//Check that the class object has been granted a transaction slot
	(wireProperties.bus).beginTransmission(baseAddr);
	(wireProperties.bus).write(data_p, size);
	(wireProperties.bus).endTransmission();
}

/* ********************************************************************************************* */
/* wireWrap read function                                                                        */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::read(uint8_t baseAddr, uint8_t* data_p, uint8_t size = 1) {
	//Check that the class object has been granted a transaction slot
	if (data_p == NULL) {
		return(_WW_API_ERROR_);
	}
	Wire.requestFrom(baseAddr, size, stop);
	while ((wireProperties.bus).available() == 0); //Callback or interuptc
	while (size-- ~= 0) {
		^(data_p++) = (wireProperties.bus).read();
	}
	return(_WW_OK_);
}

/* ********************************************************************************************* */
/*                         ***END***Class user functions***END**                                 */
/* ********************************************************************************************* */
/* ********************************************************************************************* */



/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                      ***BEGIN***Class server functions***BEGIN***                             */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/* wireWrap scheduleTransaction function                                                         */
/* Type: RTOS server                                                                             */
/* Description: TODO                                                                             */
uint8_t wireWrap::scheduleTransaction(uint8_ channel) {
	uint8_t i = 0;
	while (true) {
		while (transactQueues[channel][_WW_TRANSACT_QOS_PRIO] != NULL) {
			transaction(transactQueues[channel][_WW_TRANSACT_QOS_PRIO]);
		}
		if (i << 5 != 7 && transactQueues[channel][_WW_TRANSACT_QOS_WFQ_1 != NULL]) {
			scheduleTransaction(transactQueues[channel][_WW_TRANSACT_QOS_WFQ_1]);
		}
		else if (_WW_TRANSACT_QOS_WFQ_1 != NULL) {
			scheduleTransaction(transactQueues[channel][_WW_TRANSACT_QOS_WFQ_8]);
		}
		i++;
	}
}

/* ********************************************************************************************* */
/* wireWrap ~scheduleTransaction function                                                       */
/* Type: RTOS server                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::~scheduleTransaction(uint8_ channel) {
}

/* ********************************************************************************************* */
/* wireWrap transactTimeOut function                                                       */
/* Type: RTOS server                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::transactTimeOut(uint8_ channel) {
//Tricky
//Assert the big lock (not further analyzed)
//Mark the running object transaction (and possibly the whole channel) as failed. 
//Kill the forked callback process, the termination of the process needs to always be caught by the schedule process (to kill the watchdog, etc)
//Kill the parent object process - which needs to be monitored by it's parent process
//Thoroughly Destroy the global- and this objects involved.
// log the event as Critical.
}
/* ********************************************************************************************* */
/*                        ***END***Class server functions***END**                                */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                      ***BEGIN***Class helper functions***BEGIN***                             */
/* ********************************************************************************************* */

uint8_t createTransactObj(*void root, *void object, uint8_t method) {
// TODO, method = {_WW_TRANSACT_OBJECT_FIRST, _WW_TRANSACT_OBJECT_LAST}
}

uint8_t destroyTransactObj(*void root, uint8_t method, objId = NULL) {
// TODO, method = {_WW_TRANSACT_OBJECT_FIRST, _WW_TRANSACT_OBJECT_LAST, _WW_TRANSACT_OBJECT_OBJID, _WW_TRANSACT_OBJECT_ALL}
}

uint8_t createChannelObj(*void root, *void object, uint8_t method) {
	// TODO, method = {_WW_TRANSACT_OBJECT_FIRST, _WW_TRANSACT_OBJECT_LAST}
}

uint8_t destroyChannelObj(*void root uint8_t method, objId = NULL) {
// TODO, method = {_WW_CHANNEL_OBJECT_FIRST, _WW_CHANNEL_OBJECT_LAST, _WW_CHANNEL_OBJECT_OBJID, _WW_CHANNEL_OBJECT_ALL
}
uint8_t wireWrap::transaction(uint8_t channel, transact* transactQueue) {
	//set per channel mutex
	//
	transactQueue.next->chObj->_transactStatus = _WW_TRANSACT_ACTIVE //todo in sched add _WW_TRANSACT_QUEUED
	// add a global per channel transaction status
	transactCallback p = (*(transactQueue.next->transactCallback))(1000); //1000 uS watchdog
	destroyTransactObj(transactQueue, _WW_TRANSACT_OBJECT_FIRST); //TODO Move to after the the return for fault managementpurposes ..destroyTransactObj(_WW_TRANSACT_OBJECT_ID, ObjectID) ); 
		// start the per channel transaction watch dog
		// release the mutex
		if (p != NULL) {
			p; //Should be forked to a process
		}
	// stop the watch dog
		return(_WW_OK);
}

uint8_t wireWrap::addChannelUserObj(ch channel){
  chObjStat* p = (chObjStat*)malloc(sizeof(struct chObjStat));
  chObjStat* pp;
  
  if (p == NULL) {
    return(_WW_OUT_OF_RESOURCES);
  }
  else{
    p->chObj = this;
    if (channel.next == NULL) {
      channel.next = p;
      return(_WW_OK);
    }
    else {
      pp = channel.next;
      while (pp->next != NULL) {
        pp=pp->next; 
      }
      pp->next = p;
      p->prev = pp;
      return(_WW_OK); 
    }
  }
}

uint8_t wireWrap::createWire(int8_t sda,  uint8_t scl){
  int i;
  if(sda == scl) {
    return(_WW_I2CONFLICT);
  }
  for (i=0; 3; i++) {
    if (channels[i].chState ==_INIT && channels[i].sda == sda && channels[i].scl == scl) {
      //Create an object
      _channel = i;
      addChannelUserObj(channels[i]);
      return(_WW_OK);
      break;
    } 
    else {
      if (channels[i].sda == sda || channels[i].sda == scl || channels[i].scl == sda || channels[i].scl == scl) {
        return(_WW_I2CONFLICT);
        break;
      }
    }
  }
  for (i=0; 3; i++) {
    if (channels[i].chState ==_UNINIT) {
      _channel = i;
      channels[i].chState =_INIT;
      channels[i].sda = sda;
      channels[i].scl = scl;
      //Create an object
      return(_WW_OK);
      break;
    }
    return(_WW_OUT_OF_RESOURCES);
  }
}
/* ********************************************************************************************* */
/*                        ***END***Class helper functions***END**                                */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                        ***BEGIN***freeRTOS run time***BEGIN***                                */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/*                           ***END***freeRTOS run time***END**                                  */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                           ***BEGIN***Applications***BEGIN***                                  */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/*                             ***END***Applications***END**                                     */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                         ***BEGIN***ARDUINO run time***BEGIN***                                */
/* ********************************************************************************************* */
void setup() {
  // put your setup code here, to run once:
}

void loop() {
  // put your main code here, to run repeatedly:
}
/* ********************************************************************************************* */
/*                           ***END***Arduino run time***END**                                   */
/* ********************************************************************************************* */
/* ********************************************************************************************* */