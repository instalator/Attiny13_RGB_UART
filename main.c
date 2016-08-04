/*
 * Created: 05.06.2016 13:59:35
 *  Author: instalator
 */ 
#define F_CPU 9600000
#include "uart13.h"

#define ADDRESS 1 // Адрес устройства
#define SIZE_RECEIVE_BUF  14
/*количество токенов*/
#define AMOUNT_PAR 4
#define TRUE   1
#define FALSE  0
char buf[SIZE_RECEIVE_BUF];
char *argv[AMOUNT_PAR];
uint8_t argc;
uint8_t i = 0;
uint8_t flag = 0;
uint8_t cnt = 0;
uint8_t R, G, B, buf_R, buf_G, buf_B;

void PARS_Init(void){
	argc = 0;
	argv[0] = buf;
	flag = FALSE;
	i = 0;
}
uint8_t PARS_StrToUchar(char *s){
	uint8_t value = 0;
	/*while(*s == '0'){
		s++;
	}*/
	while(*s){
		value += (*s - 0x30);
		s++;
		if (*s){
			value *= 10;
		}
	};
	return value;
}
        ISR(TIM0_COMPA_vect){
	        TXPORT = (TXPORT & ~(1 << TXD)) | ((txbyte & 0x01) << TXD); // Выставляем в бит TXD младший бит txbyte
	        txbyte = (txbyte >> 0x01) + 0x8000;							// Двигаем txbyte вправо на 1 и пишем 1 в старший разряд (0x8000)
	        if(txbitcount > 0){											// Если идет передача (счетик бит больше нуля),
		        txbitcount--;											// то уменьшаем его на единицу.
	        }
			if (++cnt == 0){ //счетчик перехода таймера через ноль
				buf_R = R; //значения длительности ШИМ
				buf_G = G;
				buf_B = B;
				//PORTB |= (1<<PINB2) | (1<<PINB3) | (1<<PINB4); //подаем 1 на все каналы
				if (R != 0){PORTB |= (1<<PINB2);}
					if (G != 0){PORTB |= (1<<PINB4);}
						if (B != 0){PORTB |= (1<<PINB3);}
			}	
			if (cnt == buf_R) PORTB &=~(1<<PINB2); //подаем 0 на канал
			if (cnt == buf_G) PORTB &=~(1<<PINB4); //по достижении
			if (cnt == buf_B) PORTB &=~(1<<PINB3); //заданной длительности.
        }
		
        ISR(TIM0_COMPB_vect){
	        if(RXPORT & (1 << RXD))			// Проверяем в каком состоянии вход RXD
	        rxbyte |= 0x80;				// Если в 1, то пишем 1 в старший разряд rxbyte
	        if(--rxbitcount == 0){			// Уменьшаем на 1 счетчик бит и проверяем не стал ли он нулем
		        TIMSK0 &= ~(1 << OCIE0B);	// Если да, запрещаем прерывание по сравнению OCR0B
		        TIFR0 |= (1 << OCF0B);		// Очищаем флаг прерывания (важно!)
		        GIFR |= (1 << INTF0);		// Очищаем флаг прерывания по INT0
		        GIMSK |= (1 << INT0);		// Разрешаем прерывание INT0
	        } else {
		        rxbyte >>= 0x01;			// Иначе сдвигаем вправо на 1 rxbyte
	        }
        }

        ISR(INT0_vect){
	        rxbitcount = 0x09;						// 8 бит данных и 1 стартовый бит
	        rxbyte = 0x00;							// Обнуляем содержимое rxbyte
	        if(TCNT0 < (BAUD_DIV / 2)){				// Если таймер не досчитал до середины текущего периода
		        OCR0B = TCNT0 + (BAUD_DIV / 10);	// То прерывание произойдет в текущем периоде спустя пол периода
	       // } else {
		     //   OCR0B = TCNT0 - (BAUD_DIV / 2);	// Иначе прерывание произойдет в уже следующем периоде таймера
	        }
	        GIMSK &= ~(1 << INT0);					// Запрещаем прерывание по INT0
	        TIFR0 |= (1 << OCF0A) | (1 << OCF0B);	// Очищаем флаг прерывания INT0
	        TIMSK0 |= (1 << OCIE0B);				// Разрешаем прерывание по OCR0B
        }

        void uart_send(uint8_t tb) {
	        while(txbitcount);				// Ждем пока закончится передача предыдущего байта
	        txbyte = (tb + 0xFF00) << 0x01; // Пишем в младшие разряды txbyte данные для передачи и сдвигаем влево на 1
	        txbitcount = 0x0A;				// Задаем счетчик байт равным 10
        }
	
        int16_t uart_recieve(uint8_t* rb){
	        if(rxbitcount < 0x09){	// Если счетчик бит на прием меньше 9
		        while(rxbitcount);	// Ждем пока завершится текущий прием
		        *rb = rxbyte;		// Пишем по адресу указателя принятый байт
		        rxbitcount = 0x09;	// Восстанавливаем значение счетчика бит
		        return (*rb);		// Возвращаемся
	        } else {
		        return (-1);		// Иначе возвращаем -1 (принимать нечего)
	        }
        }

        void uart_init(){
	        txbyte = 0xFFFF;		// Значение буфера на передачу - все единицы
	        rxbyte = 0x00;			// Значение буфера на прием - все нули
	        txbitcount = 0x00;		// Значение счетчика преедаваемых бит - ноль (ничего пока не передаем)
	        rxbitcount = 0x09;		// Значение счетчика бит на прием - 9 (ожидаем возможного приема)
	        
	        TXDDR |= (1 << TXD);		// Задаем направление порта на передачу как выход
	        RXDDR &= ~(1 << RXD);		// Задаем направление порта на прием как вход
	        TXPORT |= (1 << TXD);		// Пишем единицу в выход TXD
	        RXPORT |= (1 << RXD);		// Подтягиваем к единице вход RXD
	        OCR0A = BAUD_DIV;			// Задаем значение регистра OCR0A в соответствии с бодрейтом
	        TIMSK0 |= (1 << OCIE0A);	// Разрешаем прерывание TIM0_COMPA
	        TCCR0A |= (1 << WGM01);		// Режим таймера CTC (очистка TCNT0 по достижению OCR0A)
	        TCCR0B |= T_DIV;			// Задаем скорость счета таймера в соответствии с делителем
	        MCUCR |= (1 << ISC01);		// Задаем прерывание INT0 по заднему фронту импульса
	        GIMSK |= (1 << INT0);		// Разрешаем прерывание INT0
	        sei();						// Разрешаем прерывания глобально
        }

int main(void) {
	DDRB = (1<<PINB2) | (1<<PINB3) | (1<<PINB4); //Порты 2, 3 и 4 на выход
	uint8_t b = 0;
	char sym;
	R = 0; //начальные значения
	G = 0; //длительности ШИМ
	B = 0; //трёх каналов
	PARS_Init();
	uart_init(); //Инициализация UART
	
while (1) {
	if (uart_recieve(&b) >= 0){
		//uart_send(b);
		sym = b;
		if (sym != '\r'){
			if (i < SIZE_RECEIVE_BUF - 1){
				if (sym != 'R' && sym != 'G' && sym != 'B'){
					if (!argc){
						argv[0] = buf;
						argc++;
					}
					if (flag){
						if (argc < AMOUNT_PAR){
							argv[argc] = &buf[i];
							argc++;
						}
						flag = FALSE;
					}
					buf[i] = sym;
					i++;
					} else {
					if (!flag){
						buf[i] = 0;
						i++;
						flag = TRUE;
					}
				}
			}
			buf[i] = 0;
			} else {
			buf[i] = 0;
			if (argc){
				if (PARS_StrToUchar(argv[0]) == ADDRESS || PARS_StrToUchar(argv[0]) == 99){
					R = PARS_StrToUchar(argv[1]);
					G = PARS_StrToUchar(argv[2]);
					B = PARS_StrToUchar(argv[3]);
				}
			}
			argc = 0;
			flag = FALSE;
			i = 0;
		}
	}
}
	
}
