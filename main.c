/*
 * Projekt.c
 *
 * Created: 2014-12-29 21:03:15
 *  Author: mysticaL-
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <string.h>
#include <stdio.h>

#include "RingBuffer.h"
#include "TxDMABuffer.h"
#include "usart.h"
#include "DataProtocol.h"
#include "ADC.h"
#include "ClockConfig.h"


CircBuffer recBuf;				//Deklaracja struktury bufora ko�owego s�u��cego do odbioru danych
uint8_t bufor[MAX_PKTLEN];
Pakiet *pkt=(Pakiet*)bufor;		// rzutowanie tablicy uint8_t na wska�nik typu Pakiet i przypisanie wska�nikowi *pkt
uint8_t global_counter_send = 0;

// Inicjalizacja komunikacji pomi�dzy modu�em BT,a MCU

void USART_init()
{
	PORTF_OUTSET=PIN3_bm;
	PORTF_DIRSET=PIN3_bm;
	USARTF0.CTRLB=USART_RXEN_bm | USART_TXEN_bm;	//W��cz odbiornik oraz nadajnik USART
	USARTF0.CTRLC=USART_CHSIZE_8BIT_gc;				//Ramka 8 bit�w, bez parzysto�ci, 1 bit stopu
	usart_set_baudrate(&USARTF0, 115200, F_CPU);
	USARTF0.CTRLA=USART_RXCINTLVL_HI_gc;			//Odblokuj przerwania odbiornika
	DMA_init();										//Zainicjuj DMA dla nadajnika USARTF0
}

void CheckForPacket_TimerInit()
{
	TCC1.CTRLB=TC_WGMODE_NORMAL_gc;        //Zwyk�y tryb pracy timera
	TCC1.PER=0;								
	TCC1.CCA=0;                            
	TCC1.CTRLA=TC_CLKSEL_DIV1024_gc;
	TCC1.INTCTRLA = TC_OVFINTLVL_MED_gc;    //W��czenie przerwania
}

// Przerwanie wywo�ywane w zwi�zku z przyj�ciem znaku do MCU

ISR(USARTF0_RXC_vect)
{
	cb_Add(&recBuf, USARTF0_DATA);		
}

bool doCmd (Pakiet *cmd_pkt);

ISR(TCC1_OVF_vect)
{
	if(isPacket(bufor, &recBuf))
	{	
		if(doCmd(pkt))
			PORTD_OUTTGL = PIN5_bm;
	}
}

int main(void)
{
	if (RC32M_en())
	{
		CPU_CCP = CCP_IOREG_gc;
		CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
	}
	
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	PORTD_DIR |= 0xFF;			// port D jako wyj�cia mikrokontrolera
	PORTD_OUTSET = 0x0F;		// port D - usatwienie stanu wysokiego
	
	USART_init();
	ADC_Init();
	CheckForPacket_TimerInit();

	
	
    while(1)
    {

    }
}

bool doCmd (Pakiet *cmd_pkt) 
{
	bool cmd_do = false;
	
	switch(cmd_pkt->Cmd)
	{
		case 0x01:
		{
			PORTD_OUTSET = cmd_pkt->Data[0];
			cmd_do = true;
			break;
		}
		case 0x02:
		{
			PORTD_OUTCLR = cmd_pkt->Data[0];
			cmd_do = true;
			break;
		}
		case 0x03: // Pomiar ADC
		{   

			/*
			TCC0.CTRLA=TC_CLKSEL_DIV256_gc;
			//TCC0.CTRLA= cmd_pkt->Data[0];
			//TCC0.PER = ((uint16_t)cmd_pkt->Data[1] << 8) | cmd_pkt->Data[2];
			TCC0.CNT = 0;
			cmd_do = true;
			break;
			*/
		}
		
		// W��cz pomiar ADC kana� 0 - pr�bkowanie 10Hz
		case  0x04: 
		{
			TCC0.CTRLA=TC_CLKSEL_DIV256_gc;
			//SendPktPortStatus(global_counter_send);
			cmd_do = true;
			break;
		}
		// Wy��cz pomiar ADC kana� 0
		case  0x05:
		{
			TCC0.CTRLA=TC_CLKSEL_OFF_gc;
			cmd_do = true;
			break;
		}
		default:
		{
			cmd_do = false;
			PORTD_OUTTGL = PIN3_bm;	
			break;
		}
			
	}

	return cmd_do;
}

