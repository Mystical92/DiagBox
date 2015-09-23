/*
 * DataProtocol.c
 *
 * Created: 2015-01-09 14:26:30
 *  Author: mysticaL-
 */ 

#include "DataProtocol.h"

bool isPacket(uint8_t *pkt, CircBuffer *recbuf)
{
	
	bool ret=false;
	if(recbuf->Count>sizeof(Pakiet))		//Sprawdzamy zmienn¹ cout naszego bufora struktury CircBuffer,
	{										//jeœli iloœæ bajtów jest mniejsza od struktury Pakiet to zwracamy flase
		pkt[0]=cb_Read(recbuf);				
		if(pkt[0]==PKT_HEADER)				//Sprawdzamy nag³ówek
		{
			uint16_t crc=_crc_xmodem_update(0xffff, pkt[0]);   //Inicjujemy CRC
			uint16_t len=0;
			//USART_putchar(&USARTF0,pkt[0]);
			for(uint8_t pos=1; pos<(len+sizeof(Pakiet)); pos++)
			{
				pkt[pos]=cb_Read_Block(recbuf);
				//USART_putchar(&USARTF0,pkt[pos]);
				if(pos==offsetof(Pakiet, Len)) 
				{
					len = (pkt[pos]<<8 | pkt[pos+1]);	//W momencie dojœcia do danej odpowiedzialnej za d³ugoœæ pola danych Data[0],
				}										//uaktualniana zostaje zmienna len, w ten sposób zanmy rozmiar pakietu
				crc=_crc_xmodem_update(crc, pkt[pos]);
			}
			if(crc==0) ret=true;	
		}
	}
	return ret;
}

