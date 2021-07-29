#ifndef LIGHTMODBUS_SLAVE_H
#define LIGHTMODBUS_SLAVE_H

#include <stdint.h>
#include <stddef.h>
#include "base.h"

typedef struct ModbusSlave ModbusSlave;

/**
	\brief A pointer to request parsing function
*/
typedef ModbusErrorInfo (*ModbusSlaveParsingFunction)(
	ModbusSlave *status,
	uint8_t address,
	uint8_t function,
	const uint8_t *requestPDU,
	uint8_t requestLength);

/**
	\brief Associates Modbus function ID with a pointer to a parsing function
*/
typedef struct ModbusSlaveFunctionHandler
{
	uint8_t id;
	ModbusSlaveParsingFunction ptr;
} ModbusSlaveFunctionHandler;

/**
	\brief Determines type of request made to the register callback function
*/
typedef enum ModbusRegisterQuery
{
	MODBUS_REGQ_R_CHECK, //!< Request for read access
	MODBUS_REGQ_W_CHECK, //!< Request for write access
	MODBUS_REGQ_R,       //!< Read request
	MODBUS_REGQ_W        //!< Write request
} ModbusRegisterQuery;

/**
	\brief Contains arguments for the register callback function
*/
typedef struct ModbusRegisterCallbackArgs
{
	ModbusDataType type;        //!< Type of accessed data
	ModbusRegisterQuery query;  //!< Type of request made to the register
	uint16_t index;             //!< Index of the register
	uint16_t value;             //!< Value of the register
	uint8_t function;           //!< Function accessing the register
} ModbusRegisterCallbackArgs;

/**
	\brief A pointer to callback for performing all register operations

	Returning any error from this function results in a 'slave failure' exception
	being returned to the master device.

	\todo requirements description
*/
typedef ModbusError (*ModbusRegisterCallback)(
	ModbusSlave *status,
	const ModbusRegisterCallbackArgs *args,
	uint16_t *result);

/**
	\brief A pointer to a callback called when a Modbus exception is generated (for slave)
*/
typedef void (*ModbusSlaveExceptionCallback)(
	ModbusSlave *status,
	uint8_t address,
	uint8_t function,
	ModbusExceptionCode code);

/**
	\brief A pointer to allocator function called to allocate frame buffers (for slave)
*/
typedef ModbusError (*ModbusSlaveAllocator)(
	ModbusSlave *status,
	uint8_t **ptr,
	uint16_t size,
	ModbusBufferPurpose purpose);

/**
	\brief Slave device status
*/
typedef struct ModbusSlave
{
	ModbusSlaveAllocator allocator;                 //!< A pointer to allocator function (required)
	ModbusRegisterCallback registerCallback;        //!< A pointer to register callback (required)
	ModbusSlaveExceptionCallback exceptionCallback; //!< A pointer to exception callback (optional)
	const ModbusSlaveFunctionHandler *functions;    //!< A pointer to an array of function handlers (required)
	uint8_t functionCount;                          //!< Number of function handlers in the array (`functions`)
	
	void *context; //!< User's context pointer

	//! Stores slave's response to master
	ModbusFrameBuffer response;
	uint8_t address; //!< Slave's address/ID

} ModbusSlave;


LIGHTMODBUS_RET_ERROR modbusSlaveInit(
	ModbusSlave *status,
	uint8_t address,
	ModbusSlaveAllocator allocator,
	ModbusRegisterCallback registerCallback,
	ModbusSlaveExceptionCallback exceptionCallback,
	const ModbusSlaveFunctionHandler *functions,
	uint8_t functionCount);

void modbusSlaveDestroy(ModbusSlave *status);
void modbusSlaveSetUserPointer(ModbusSlave *status, void *ptr);
void *modbusSlaveGetUserPointer(ModbusSlave *status);

LIGHTMODBUS_RET_ERROR modbusBuildException(
	ModbusSlave *status,
	uint8_t address,
	uint8_t function,
	ModbusExceptionCode code);

LIGHTMODBUS_WARN_UNUSED ModbusError modbusSlaveDefaultAllocator(ModbusSlave *status, uint8_t **ptr, uint16_t size, ModbusBufferPurpose purpose);
LIGHTMODBUS_WARN_UNUSED ModbusError modbusSlaveAllocateResponse(ModbusSlave *status, uint16_t size);
void modbusSlaveFreeResponse(ModbusSlave *status);

LIGHTMODBUS_RET_ERROR modbusParseRequest(ModbusSlave *status, uint8_t address, const uint8_t *request, uint8_t requestLength);
LIGHTMODBUS_RET_ERROR modbusParseRequestPDU(ModbusSlave *status, uint8_t address, const uint8_t *request, uint8_t requestLength);
LIGHTMODBUS_RET_ERROR modbusParseRequestRTU(ModbusSlave *status, const uint8_t *request, uint16_t requestLength);
LIGHTMODBUS_RET_ERROR modbusParseRequestTCP(ModbusSlave *status, const uint8_t *request, uint16_t requestLength);

extern ModbusSlaveFunctionHandler modbusSlaveDefaultFunctions[];
extern const uint8_t modbusSlaveDefaultFunctionCount;

#endif