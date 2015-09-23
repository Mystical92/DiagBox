/*
 * ADC.h
 *
 * Created: 2013-07-28 20:40:18
 *  Author: tmf
 */


#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

typedef struct
{
	int16_t CH[2];			//4 kana³y ADC
} ADC_Result_t;

#define ADC_BUF_SIZE 100	//Wielkoœæ bufora strony. Je¿eli damy 500 to bêdzie 500 sampli dla 4 kana³ów 
							//- teoretycznie 500ms bo timer przeyrwa co 1ms
							
extern ADC_Result_t ADC_Res[2][ADC_BUF_SIZE];	//Bufor na dane
extern volatile uint8_t ADC_Res_Page;			//Strona zawieraj¹ca wyniki

void ADC_Init();


#endif /* ADC_H_ */