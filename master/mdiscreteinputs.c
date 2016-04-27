#include "../modlib.h"
#include "../parser.h"
#include "mtypes.h"

#include "mdiscreteinputs.h"

//Use external master configuration
extern MODBUSMasterStatus MODBUSMaster;

uint8_t MODBUSBuildRequest02( uint8_t Address, uint16_t FirstCoil, uint16_t CoilCount )
{
	//Build request02 frame, to send it so slave
	//Read multiple discrete inputs

	//Set frame length
	uint8_t FrameLength = 8;

	//Set output frame length to 0 (in case of interrupts)
	MODBUSMaster.Request.Length = 0;

	//Allocate memory for frame builder
	union MODBUSParser *Builder = (union MODBUSParser *) malloc( FrameLength );

	//Reallocate memory for final frame
	MODBUSMaster.Request.Frame = (uint8_t *) realloc( MODBUSMaster.Request.Frame, FrameLength );

	( *Builder ).Base.Address = Address;
	( *Builder ).Base.Function = 2;
	( *Builder ).Request02.FirstInput = MODBUSSwapEndian( FirstCoil );
	( *Builder ).Request02.InputCount = MODBUSSwapEndian( CoilCount );

	//Calculate CRC
	( *Builder ).Request02.CRC = MODBUSCRC16( ( *Builder ).Frame, FrameLength - 2 );

	//Copy frame from builder to output structure
	memcpy( MODBUSMaster.Request.Frame, ( *Builder ).Frame, FrameLength );

	//Free used memory
	free( Builder );

	MODBUSMaster.Request.Length = FrameLength;

	return 0;
}

void MODBUSParseResponse02( union MODBUSParser *Parser, union MODBUSParser *RequestParser )
{
	//Parse slave response to request 02 (read multiple discrete inputs)

	//Update frame length
	uint8_t FrameLength = 5 + ( *Parser ).Response02.BytesCount;
	uint8_t DataOK = 1;
	uint8_t i = 0;

	//Check frame CRC
	DataOK &= ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) & 0x00FF ) == ( *Parser ).Response02.Values[( *Parser ).Response02.BytesCount];
	DataOK &= ( ( MODBUSCRC16( ( *Parser ).Frame, FrameLength - 2 ) & 0xFF00 ) >> 8 ) == ( *Parser ).Response02.Values[( *Parser ).Response02.BytesCount + 1];

	if ( !DataOK )
	{
		//Create an exception when CRC is bad (unoficially, but 255 is CRC internal exception code)
		MODBUSMaster.Exception.Address = ( *Parser ).Base.Address;
		MODBUSMaster.Exception.Function = ( *Parser ).Base.Function;
		MODBUSMaster.Exception.Code = 255;
		MODBUSMaster.Error = 1;
		MODBUSMaster.Finished = 1;
		return;
	}

	//Check between data sent to slave and received from slave
	DataOK &= ( ( *Parser ).Base.Address == ( *RequestParser ).Base.Address );
	DataOK &= ( ( *Parser ).Base.Function == ( *RequestParser ).Base.Function );


	MODBUSMaster.Data = (MODBUSData *) realloc( MODBUSMaster.Data, sizeof( MODBUSData ) * MODBUSSwapEndian( ( *RequestParser ).Request02.InputCount ) );
	for ( i = 0; i < MODBUSSwapEndian( ( *RequestParser ).Request02.InputCount ); i++ )
	{
		MODBUSMaster.Data[i].Address = ( *Parser ).Base.Address;
		MODBUSMaster.Data[i].DataType = DiscreteInput;
		MODBUSMaster.Data[i].Register = MODBUSSwapEndian( ( *RequestParser ).Request02.FirstInput ) + i;
		MODBUSMaster.Data[i].Value = MODBUSReadMaskBit( ( *Parser ).Response02.Values, ( *Parser ).Response02.BytesCount, i );

	}

	//Set up data length - response successfully parsed
	MODBUSMaster.Error = !DataOK;
	MODBUSMaster.DataLength = MODBUSSwapEndian( ( *RequestParser ).Request02.InputCount );
	MODBUSMaster.Finished = 1;
}