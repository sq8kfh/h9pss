/*
 * h9pss
 * Power supply switch
 *
 * Created by SQ8KFH on 2017-09-07.
 *
 * Copyright (C) 2017-2020 Kamil Palkowski. All rights reserved.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h> 
#include <util/delay.h>

#include "can/can.h"

volatile uint8_t output1 = 0;
volatile uint8_t output2 = 0;
	
#define LED_DDR DDRB
#define LED_PORT PORTB
#define LED_PIN PB1

#define BUTTON_DDR DDRB
#define BUTTON_PORT PORTB
#define BUTTON_PINx PINB
#define BUTTON_PIN PB0

void set_output(uint8_t o1, uint8_t o2) {
	if (o1) {
		PORTB |= _BV(PB5);
		output1 = 1;
	}
	else {
		PORTB &= ~_BV(PB5);
		output1 = 0;
	}
//	if (o2) {
//		PORTB |= _BV(PB6);
//		output2 = 1;
//	}
//	else {
//		PORTB &= ~_BV(PB6);
//		output2 = 0;
//	}
}

int main(void) {
	DDRB = 0xff ^ (1 << BUTTON_PIN); //button
	DDRC = 0xff;// ^ ((1 << PC6) | (1 << PC5) | (1 << PC4)); //ADC
	DDRD = 0xff ^ ((1 << PD5) | (1 << PD6)); //digital I
	DDRE = 0xff;

    BUTTON_DDR &= ~(1 << BUTTON_PIN);
    BUTTON_PORT |= (1 << BUTTON_PIN);
    
    PORTD |= ((1 << PD5) | (1 << PD6));

//	ADMUX = (1 << MUX3) | (1 << MUX1);
//	ADCSRA = (1 << ADEN) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);
//	ADCSRB = (1 << AREFEN);
//
	CAN_init();
	sei();
	
	_delay_ms(100);
	CAN_send_turned_on_broadcast();

	uint32_t led_counter = 0x1000;

    unsigned int button_count = 0;

	while (1) {
		if (led_counter == 0) {
			LED_PORT ^= (1<<LED_PIN);
			if (LED_PORT & (1<<LED_PIN)) {
				led_counter = 0x1000;
			}
			else {
				led_counter = 0x80000;
			}
		}
		--led_counter;

        if (!(BUTTON_PINx & (1 << BUTTON_PIN))) {
            if (button_count < 0xff00)
                ++button_count;
        }
        else {
            if (button_count > 50) {
                if (output1) output1 = 0;
                else output1 = 1;

                h9msg_t message;

                CAN_init_new_msg(&message);
                message.destination_id = H9MSG_BROADCAST_ID;
                message.type = H9MSG_TYPE_REG_INTERNALLY_CHANGED;
                message.dlc = 2;
                message.data[0] = 11;
                message.data[1] = output1;
                CAN_put_msg(&message);
            }
            button_count = 0;
        }

		set_output(output1, output2);
		
		h9msg_t cm;
		if (CAN_get_msg(&cm)) {
			LED_PORT |= (1<<LED_PIN);
			led_counter = 0x10000;
			if (cm.type == H9MSG_TYPE_GET_REG) {
				h9msg_t cm_res;
				CAN_init_response_msg(&cm, &cm_res);
				cm_res.dlc = 2;
				cm_res.data[0] = cm.data[0];

                uint32_t tmp;
                uint16_t adc;
				switch (cm.data[0]) {
                    case 10:
                        tmp = (PIND & (1 << PD6)) ? 0 : 1;
                        tmp <<= 1;
                        tmp |= (PIND & (1 << PD5)) ? 0 : 1;
                        cm_res.data[1] = tmp;
                        CAN_put_msg(&cm_res);
                        break;
					case 11:
						cm_res.data[1] = output1;
						CAN_put_msg(&cm_res);
						break;
//					case 11:
//						cm_res.data[1] = output2;
//						CAN_put_msg(&cm_res);
//						break;
//					case 12:
//                        ADMUX = (1 << MUX3) | (1 << MUX1);
//						ADCSRA |= (1<<ADSC); //ADSC: uruchomienie pojedynczej konwersji
//						cm_res.dlc = 3;
//						while(ADCSRA & (1<<ADSC)); //czeka na zako�czenie konwersji
//                        tmp = ADCL;
//                        adc = ADCH;
//						adc <<= 8;
//						adc |= tmp;
//						tmp = (uint32_t)625*5*adc/256;
//						cm_res.data[1] = (uint8_t)((tmp >> 8) & 0xff);
//						cm_res.data[2] = (uint8_t)(tmp & 0xff);
//						CAN_put_msg(&cm_res);
//						break;
//                    case 13:
//                        ADMUX = (1 << MUX3);
//                        ADCSRA |= (1<<ADSC); //ADSC: uruchomienie pojedynczej konwersji
//                        cm_res.dlc = 3;
//                        while(ADCSRA & (1<<ADSC)); //czeka na zako�czenie konwersji
//                        tmp = ADCL;
//                        adc = ADCH;
//                        adc <<= 8;
//                        adc |= tmp;
//                        tmp = 16*adc;
//                        cm_res.data[1] = (uint8_t)((tmp >> 8) & 0xff);
//                        cm_res.data[2] = (uint8_t)(tmp & 0xff);
//                        CAN_put_msg(&cm_res);
//                        break;
//                    case 14:
//                        ADMUX = (1 << MUX3) | (1 << MUX0);
//                        ADCSRA |= (1<<ADSC); //ADSC: uruchomienie pojedynczej konwersji
//                        cm_res.dlc = 3;
//                        while(ADCSRA & (1<<ADSC)); //czeka na zako�czenie konwersji
//                        tmp = ADCL;
//                        adc = ADCH;
//                        adc <<= 8;
//                        adc |= tmp;
//                        tmp = 16*adc;
//                        cm_res.data[1] = (uint8_t)((tmp >> 8) & 0xff);
//                        cm_res.data[2] = (uint8_t)(tmp & 0xff);
//                        CAN_put_msg(&cm_res);
//                        break;
                    default:
                        cm_res.type = H9MSG_TYPE_ERROR;
                        cm_res.data[0] = H9MSG_ERROR_INVALID_REGISTER;
                        cm_res.dlc = 1;
                        CAN_put_msg(&cm_res);
				}
			}
			else if (cm.type == H9MSG_TYPE_SET_REG) {
				h9msg_t cm_res;
				CAN_init_response_msg(&cm, &cm_res);
				cm_res.dlc = 0;
				cm_res.data[0] = cm.data[0];
				switch (cm.data[0]) {
					case 11:
						set_output(cm.data[1], output2);
						_NOP();
						_NOP();
						_NOP();
						_NOP();
						cm_res.data[1] = output1;
						cm_res.dlc = 2;
						CAN_put_msg(&cm_res);
						break;
//					case 11:
//						set_output(output1, cm.data[1]);
//						_NOP();
//						_NOP();
//						_NOP();
//						_NOP();
//						cm_res.data[1] = output2;
//						cm_res.dlc = 2;
//						CAN_put_msg(&cm_res);
//						break;
                    default:
                        cm_res.type = H9MSG_TYPE_ERROR;
                        cm_res.data[0] = H9MSG_ERROR_INVALID_REGISTER;
                        cm_res.dlc = 1;
                        CAN_put_msg(&cm_res);
				}
			}
		}
	}
}
