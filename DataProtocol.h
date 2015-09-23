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

#define PKT_HEADER  0xFF  //Nag³ówek - znak wyró¿niaj¹cy pocz¹tek pakietu
#define MAX_PKTLEN 40     //Maksymalna d³ugoœæ odbieranego pakietu

typedef struct
{
	uint8_t		Header;		//Nag³ówek pakietu
	uint8_t		Cmd;		//Rozkaz polecenia
	uint16_t	Len;		//D³ugoœæ pola danych
	uint8_t		Data[0];	//Pole danych o ZMIENNEJ d³ugoœci
	uint16_t	CRC16;      //CRC ca³ego pakietu
} Pakiet;

bool isPacket(uint8_t *pkt, CircBuffer *recbuf);

#endif /* DATAPROTOCOL_H_ */