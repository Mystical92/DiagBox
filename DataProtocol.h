/*
 * DataProtocol.h
 *
 * Created: 2015-01-09 14:29:09
 *  Author: mysticaL-
 */ 


#ifndef DATAPROTOCOL_H_
#define DATAPROTOCOL_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // Makro offset
#include <util/crc16.h>
#include "RingBuffer.h"

#define PKT_HEADER  0xFF  //Nag��wek - znak wyr�niaj�cy pocz�tek pakietu
#define MAX_PKTLEN 40     //Maksymalna d�ugo�� odbieranego pakietu

typedef struct
{
	uint8_t		Header;		//Nag��wek pakietu
	uint8_t		Cmd;		//Rozkaz polecenia
	uint16_t	Len;		//D�ugo�� pola danych
	uint8_t		Data[0];	//Pole danych o ZMIENNEJ d�ugo�ci
	uint16_t	CRC16;      //CRC ca�ego pakietu
} Pakiet;

bool isPacket(uint8_t *pkt, CircBuffer *recbuf);

#endif /* DATAPROTOCOL_H_ */