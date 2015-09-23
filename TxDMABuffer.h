/*
 * bufusart.h
 *
 * Created: 2013-01-24 18:12:26
 *  Author: tmf
 */ 


#ifndef BUFUSART_H_
#define BUFUSART_H_

#include "RingBuffer.h"
#include "ADC.h"

#define CB_SEND_MAXTRANS  100         //Maksymalna liczba elementów bufora

typedef struct
{
	uint8_t Beg;							//Pierwszy element bufora
	uint8_t Count;							//Liczba elementów w buforze
	uint8_t *elements[CB_SEND_MAXTRANS];	//Elementy bufora
} CircSendBuffer;

bool cb_Send_Add(CircSendBuffer *cb, uint8_t *elem);
uint8_t *cb_Send_Read(CircSendBuffer *cb);

static inline bool cb_Send_IsFull(CircSendBuffer *cb)
{
	return cb->Count == CB_SEND_MAXTRANS;
}

static inline bool cb_Send_IsEmpty(CircSendBuffer *cb)
{
	return cb->Count == 0;
}

extern CircBuffer recBuf;
extern CircSendBuffer sendBuf;

extern volatile uint8_t cmdrec;

//void USART_send_buf_F(CircSendBuffer *buf, const char *txt);
//void SendPktADC(uint8_t ADC_Data[][ADC_BUF_SIZE], uint8_t bufferPage);
void SendPktPortStatus(uint8_t data);

void DMA_init();								 //Zainicjuj kana³ CH0 DMA dla nadajnika USARTC0
void DMA_InitTransfer(void *src, uint16_t len); //Zainicjuj jeden transfer DMA z podanego adresu, len bajtów
uint16_t doCRC16(uint8_t *Dane, uint16_t size);
#endif /* BUFUSART_H_ */