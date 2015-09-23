/*
 * bufusart.c
 *
 * Created: 2013-01-24 18:13:44
 *  Author: tmf
 */ 

#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "usart.h"
#include "malloc.h"
#include "DataProtocol.h"
#include "TxDMABuffer.h"


CircSendBuffer sendBuf;
uint8_t *DMA_buf;   //WskaŸnik na bufor danych do transmisji 

bool cb_Send_Add(CircSendBuffer *cb, uint8_t *elem)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)       //Trwa nadawanie, wiêc tylko dodajemy do kolejki
	{
	    if((DMA_CH2_CTRLB & (DMA_CH_CHBUSY_bm | DMA_CH_TRNIF_bm))==0)
		{  //Musimy zainicjowaæ transfer
			DMA_buf=elem;
			DMA_InitTransfer(DMA_buf, *(DMA_buf + offsetof(Pakiet,Len)) + sizeof(Pakiet));
			
			PORTD.OUTTGL = PIN5_bm;
			
			return true;
		}
		if(cb_Send_IsFull(cb)) return false;	//Czy jest miejsce w kolejce?
		uint8_t end = (cb->Beg + cb->Count) % CB_SEND_MAXTRANS;
		cb->elements[end] = elem;				//Dodaj transakcjê
		++cb->Count;							//Liczba elementów w buforze
	}	
	return true;      //Wszystko ok
}

uint8_t *cb_Send_Read(CircSendBuffer *cb)
{
	uint8_t *elem;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(cb_Send_IsEmpty(cb)) return 0;       //Bufor pusty, nie mo¿na zwróciæ elementu
		elem = cb->elements[cb->Beg];
		cb->Beg = (cb->Beg + 1) % CB_SEND_MAXTRANS;
		-- cb->Count;                        //Zmniejszamy liczbê elementów pozosta³ych
	}		                                 //w buforze
	return elem;
}

void DMA_init()
{
	DMA.CH2.DESTADDR0=(uint8_t)(uint16_t)&USARTF0_DATA;	//Adres rejestru danych USART
	DMA.CH2.DESTADDR1=((uint16_t)&USARTF0_DATA)>>8;
	DMA.CH2.DESTADDR2=0;
	DMA.CTRL=DMA_ENABLE_bm;								//Odblokuj kontroler, round robin
	DMA.CH2.CTRLB=DMA_CH_TRNINTLVL_MED_gc;				//Odblokuj przerwanie koñca transakcji DMA
	DMA.CH2.ADDRCTRL=DMA_CH_SRCDIR_INC_gc;				//Adres Ÿród³owy jest inkrementowany przy transferze, docelowy nie
	DMA.CH2.TRIGSRC=DMA_CH_TRIGSRC_USARTF0_DRE_gc;		//Wyzwalanie kana³u - wolne miejsce w buforze nadajnika
}

void DMA_InitTransfer(void *src, uint16_t len)
{
	DMA.CH2.SRCADDR0=(uint16_t)src;						//Adres rejestru wybranych danych
	DMA.CH2.SRCADDR1=((uint16_t)src)>>8;
	DMA.CH2.SRCADDR2=0;									//Nie korzystamy z najstarszych 8 bitów adresu
	DMA.CH2.TRFCNT=len;									//Iloœæ danych do przeransferowania
	DMA.CH2.CTRLA=DMA_CH_ENABLE_bm | DMA_CH_SINGLE_bm;	//Zainicjuj transfer DMA - tryb single shot, 1 bajt/transfer
}

ISR(DMA_CH2_vect)					//Koniec poprzedniego transferu, sprawdŸmy czy czeka kolejny
{
	DMA_CH2_CTRLB|=DMA_CH_TRNIF_bm;	//Kasujemy flagê - ze wzglêdu na wspó³dzielenie wektora nie jest kasowana automatycznie
	/*
	free(DMA_buf);
	if(cb_Send_IsEmpty(&sendBuf)==false)
	{								//Czekaj¹ dane
		DMA_buf=cb_Send_Read(&sendBuf);
		DMA_InitTransfer(DMA_buf, *(DMA_buf + offsetof(Pakiet,Len)) + sizeof(Pakiet));
	}
	*/
}

uint16_t doCRC16(uint8_t *Dane, uint16_t size)
{
	CRC.CTRL=CRC_RESET_RESET1_gc;	//Zainicjowanie wartoœæi CHECKSUM 0xFFFFFFFF
	CRC.CTRL=CRC_SOURCE_IO_gc;		//CRC16, dane z rejestru IO
	for(;size>0; size--) CRC.DATAIN=*(Dane++);
	CRC.STATUS=CRC_BUSY_bm;			//Koniec danych
	return CRC.CHECKSUM0 | CRC.CHECKSUM1<<8;
}


void SendPktADC(uint8_t ADC_Data[][ADC_BUF_SIZE], uint8_t bufferPage)
{
	/*
	uint8_t *pkt_n = malloc_re( sizeof(Pakiet) + (sizeof(ADC_Result_t)*ADC_BUF_SIZE) );	// Dynamiczne przydzielenie pamiêci 
	*pkt_n = PKT_HEADER;																		// Nag³ówek pakietu 
	*(pkt_n+1) = 0x02;																			// Numer rozkazu
	*(pkt_n+2) = sizeof(ADC_Result_t)*ADC_BUF_SIZE;												// Oblicza i przypisyje d³ugoœæ bloku Data[]
	
	for(uint8_t i = 0; i<= ADC_BUF_SIZE;i++)
	{
		*(pkt_n+ 3 + i) = ADC_Data[bufferPage][i];
	}
		
	uint16_t crc16=doCRC16(pkt_n, 3);	// Oblicza CRC16
	*(pkt_n+3) = crc16>>8;				// Przypisanie straszego bajtu sumy CRC16
	*(pkt_n+3+1) = crc16 & 0xFF;		// Przypisanie m³odszego bajtu sumy CRC16
	*/
	//cb_Send_Add(&sendBuf, );		// Dodanie pakietu do bufora 
	
}


void SendPktPortStatus(uint8_t data)
{
	uint8_t *pkt_new = malloc_re(sizeof(Pakiet));	// Dynamiczne przydzielenie pamiêci 
	*(pkt_new+0) = PKT_HEADER;						// Nag³ówek pakietu 
	*(pkt_new+1) = 0x04;							// Numer rozkazu
	*(pkt_new+2) = 0x02;							// Oblicza i przypisyje d³ugoœæ bloku Data[]
	*(pkt_new+3) = (uint8_t)PORTD.OUT;
	*(pkt_new+4) = data;
	uint16_t crc16 = doCRC16(pkt_new, 5);			// Oblicza CRC16
	*(pkt_new+ 5) = crc16>>8;						// Przypisanie straszego bajtu sumy CRC16
	*(pkt_new+ 6) = crc16 & 0xFF;					// Przypisanie m³odszego bajtu sumy CRC16
	cb_Send_Add(&sendBuf, pkt_new);					// Dodanie pakietu do bufora 
}