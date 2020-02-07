/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                       ***wireWrapper header file***BEGIN***                                   */
/* ********************************************************************************************* */

#ifdef ................

#include <Wire.h>
//and many more

/* ********************************************************************************************* */
/* wireWrap data type definitions                                                                */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */

 //parameter provides uS allowed to spend in the callback routine
typedef void(transactCallback*)(void* transactHandle_p, void* transactCallbackParam_p);
typedef void(transactResCallback*)(void* transactHandle_p, void* transactResCallbackParam_p);

struct wireProp_t {
	TwoWire bus
	wireMode_t wiremode;
	wireSpeed_t wirespeed;
	wireState_t wireState;
	uint8_t sda;
	uint8_t scl;
}

struct wireTransact {
	wireTransact* prev_p;
	wwWireWrapTransactQoS_t prio;
	transactCallback* transactCallback_p;
	void* transactCallbackParam_p;
	transactResCallback* transactResCallback_p;
	void* transactResCallbackParam_p;
	wireTransact* next;
}

//API return codes
//Needs alignment with the CoreOS return codes
const uint8_t _WW_OK_ = 0;
const uint8_t _WW_I2CONFLICT_ = 1;
const uint8_t _WW_RESOURCE_CONFLICT_ = 2;
const uint8_t _WW_INIT_FAILURE_ = 3;
const uint8_t _WW_GENERAL_FAILURE_ = 4;
const uint8_t _WW_TRANSACTION_NOT_ACTIVE_ = 5;
const uint8_t _WW_OUT_OF_RESOURCES_ = 6;
const uint8_t _WW_API_ERROR_ = 7;

//Below needs stricter definition and purpose
enum wwWireWrapTransactQoS_t { _WW_TRANSACT_QOS_BEF = 0, _WW_TRANSACT_QOS_WFQ_1 = 8, _WW_TRANSACT_QOS_WFQ_1 = 2, _WW_TRANSACT_QOS_STRICT_PRIO = 3 };

enum wireSpeed_t {_100Kb_ = 0, 400Kb = 1, _1Mb_, _3_2Mb_};
enum wireMode_t { _HW_, _SW_ };
//I2C Wirwe State should those be aligned with the API return codes?
enum wireState_t { _WW_WIRE_IDLE_ = 0, _WW_WIRE_UNINIT_, _WW_WIRE_GENERAL_FAIL_, _WW_WIRE_IN_USE_ = 3, _WW_WIRE_RECOVERY_ = 4 };

_MAX_NO_WIRES_ = 2;

/* ********************************************************************************************* */
/* wireWrap class definitions                                                                    */
/* Type: User, Helpers & RTOS                                                                    */
/* Description: TODO                                                                             */
class wireWrap {
public:
	wireWrap(); //Initiate a new I2C channel object (but not the I2C channel it self)
	~wireWrap(); //Destruction of channel object not supported, will cause panic
	uint8_t wireInit(uint8_t sda, uint8_t scl, wireSpeed_t wireSpeed, wireMode_t wireMode  = _HW_); //Initiate an I2C channel
	void* requestWireTransact(uint8_t prio, TickType_t timeout, transactCallback* tcb, void* tcbParam, resultCallback* trcb, void* trcbParam); //Requests an I2C transaction slot of length of 
											 // max timeOut ticks. The callback runs to completion
											 // and may therefore never block for any significant time
											 // if time-out occurs, the wireWrap worker process running the
											 // calback is restarted causing the callback to instantly be terminated.
											 // When the call back returns or if time-out and the resultcallback is called
	void* ISRrequestTransaction(uint8_t prio, TickType_t timeout, transactCallback cb resultCallback rcb, void* cbParam); //much the same as above, but from and ISR
	wireState_t wireStatus();	//Requests the i2c channel status
	uint8_t wireTransactStatus(void* transactHandle_p); //Requests the status on a transaction 
	uint8_t wireWrite(uint8_t baseAddr, uint8_t* data_p, uint8_t size = 1); //Write to an i2c address
	uint8_t wireRead(uint8_t baseAddr, uint8_t* data_p, uint8_t size = 1); //Read from an i2c address
	uint8_t wireAbortTransact(void* transactHandle_p);

private:
	// common private class data structures
	static uint8_t noOfCreatedWires;
	static SemaphoreHandle_t globalWireMutex;
	// private class data structures
	SemaphoreHandle_t wireMutex;
	wireProp_t wireProperties;
	transact* wireTransactQueue[_MAX_I2C_PRIOS_];
	uint8_t wireTransactPrioQueueDepth[_MAX_I2C_PRIOS_]; //define constant _MAX_PRIO_QUEUE_DEPTH[_MAX_I2C_PRIOS_]
	uint8_t wireTransactTotQueueDepth; //define constant _MAX_TOT_QUEUE_DEPTH
	uint32_t wireTransactPrioDrops[_MAX_I2C_PRIOS_]; 
	//Define statistics and watermarks and escalations more carefully!!

};
/* ********************************************************************************************* */
/*                     ***END***wireWrapper heading file***END***                                */
/* ********************************************************************************************* */
/* ********************************************************************************************* */
