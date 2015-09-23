/*
 * RingBuffer.h
 *
 * Created: 2013-02-13 08:56:50
 *  Author: tmf
 */ 


#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>

#define CB_MAXTRANS  100		//Maksymalna liczba elementów bufora

typedef uint8_t CB_Element;		//Typ elementów w buforze

typedef struct
{
	CB_Element elements[CB_MAXTRANS];  //Elementy bufora
	uint8_t Beg;                       //Pierwszy element bufora
	volatile uint8_t Count;            //Liczba elementów w buforze
} CircBuffer;

static inline bool cb_IsFull(CircBuffer *cb)
{
	return cb->Count == CB_MAXTRANS;
}

static inline bool cb_IsEmpty(CircBuffer *cb)
{
	return cb->Count == 0;
}

bool cb_Add(CircBuffer *cb, CB_Element elem);
CB_Element cb_Read(CircBuffer *cb);
CB_Element cb_Read_Block(CircBuffer *cb);

#endif /* RINGBUFFER_H_ */