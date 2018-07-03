/*
 * main.c
 *
 *  Created on: 12 lut 2018
 *      Author: Bartosz Pracz
 *     	blink-1 ESC
 *
 *      v0.3 - correct throttle scale
 *      v0.4 - only high side on PWM
 *      v0.5 - beta faster sample rate
 *      v0.7 - kers support
 *      v0.8 - better measurement
 *
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

//U//drivers
# define U_H1 TCCR1A |= (1<<COM1A1); //pwm off
# define U_H2 PORTD |= (1<<PD1); //high
#define U_H3 PORTB |= (1 << PB1); //sd on

#define U_H U_H2; U_H1; U_H3;

# define U_L1 TCCR1A &= ~(1<<COM1A1); //pwm on
# define U_L2 PORTD &= ~(1<<PD1); // low
#define U_L3 PORTB |= (1 << PB1); //sd on

#define U_L U_L2; U_L1; U_L3;

# define U_OFF1 TCCR1A &= ~(1<<COM1A1); //pwm off
# define U_OFF2 PORTD &= ~(1<<PD1); // low
#define U_OFF3 PORTB &= ~(1 << PB1); //sd off

#define U_OFF U_OFF2; U_OFF1; U_OFF3;

//V
# define V_H1 TCCR0A |= (1<<COM0A1); //pwm off
# define V_H2 PORTD |= (1<<PD2); //high
#define V_H3 PORTD |= (1<<PD6);//sd on

#define V_H V_H2; V_H1; V_H3;

# define V_L1 TCCR0A &= ~(1<<COM0A1);//pwm on
# define V_L2 PORTD &= ~(1<<PD2);//low
#define V_L3 PORTD |= (1<<PD6);//sd on

#define V_L V_L2; V_L1; V_L3;

# define V_OFF1 TCCR0A &= ~(1<<COM0A1);//pwm off
# define V_OFF2 PORTD &= ~(1<<PD2);//low
#define V_OFF3 PORTD &= ~(1<<PD6);//sd off

#define V_OFF V_OFF2; V_OFF1; V_OFF3;

//W
# define W_H1 TCCR2A |= (1<<COM2A1); //pwm off
# define W_H2 PORTD |= (1<<PD3); //high
#define W_H3 PORTB |= (1<<PB3);//sd on

#define W_H W_H2; W_H1; W_H3;

# define W_L1 TCCR2A &= ~(1<<COM2A1); //pwm on
# define W_L2 PORTD &= ~(1<<PD3); //low
#define W_L3 PORTB |= (1<<PB3);//sd on

#define W_L W_L2; W_L1; W_L3;

# define W_OFF1 TCCR2A &= ~(1<<COM2A1);//pwm off
# define W_OFF2 PORTD &= ~(1<<PD3); //low
#define W_OFF3 PORTB &= ~(1<<PB3);//sd off

#define W_OFF W_OFF2; W_OFF1; W_OFF3;

#define ALL_OFF U_OFF; V_OFF; W_OFF;

#define HALL1 ADMUX |= (1<<MUX0); ADMUX &= ~(1<<MUX1);
#define HALL2 ADMUX &= ~(1<<MUX0); ADMUX |= (1<<MUX1);
#define HALL3 ADMUX |= (1<<MUX0); ADMUX |= (1<<MUX1);

#define one (PINB & (1<<PB7))
#define two (PIND & (1<<PD5))
#define three (PIND & (1<<PD7))
#define four (PINB & (1<<PB0))

#define oneOff !(PINB & (1<<PB7))
#define twoOff !(PIND & (1<<PD5))
#define threeOff !(PIND & (1<<PD7))
#define fourOff !(PINB & (1<<PB0))

int treshold = 400;

volatile int enable;
volatile int phase;
volatile int toggle = 0;
volatile int pulse;
volatile int kers;
int kersPower = 150;

volatile int reading1;
volatile int reading2;
volatile int reading3;

volatile int sensor1;
volatile int sensor2;
volatile int sensor3;

int duty[] = { 0, 30, 60, 90, 120, 140, 160, 180, 200, 210, 220, 235, 245, 250,
		255 };

void measurement() {
	HALL1
	ADCSRA |= (1 << ADSC); //start
	while (ADCSRA & (1 << ADSC))
		;
	reading1 = ADC;

	if (reading1 > treshold) {
		sensor1 = 1;
		//PORTC |= (1 << PC5);
	} else {
		sensor1 = 0;
		//PORTC &= ~(1 << PC5);
	}

	HALL2
	ADCSRA |= (1 << ADSC); //start
	while (ADCSRA & (1 << ADSC))
		;
	reading2 = ADC;

	if (reading2 > treshold) {
		sensor2 = 1;
		PORTC |= (1 << PC5);
	} else {
		sensor2 = 0;
		PORTC &= ~(1 << PC5);
	}

	HALL3
	ADCSRA |= (1 << ADSC); //start
	while (ADCSRA & (1 << ADSC))
		;
	reading3 = ADC;

	if (reading3 > treshold) {
		sensor3 = 1;
		//PORTC |= (1 << PC5);
	} else {
		sensor3 = 0;
		//PORTC &= ~(1 << PC5);
	}

}

void rotation(volatile int step) {

	//if(step == 1) PORTC |= (1<<PC5);
	//else PORTC &= ~(1<<PC5);

	if (step == 0) {
		W_OFF
		U_L
		V_H
	}
	if (step == 1) {
		V_OFF
		U_L
		W_H
	}
	if (step == 2) {
		U_OFF
		V_L
		W_H
	}
	if (step == 3) {
		W_OFF
		U_H
		V_L
	}
	if (step == 4) {
		V_OFF
		U_H
		W_L
	}
	if (step == 5) {
		U_OFF
		V_H
		W_L
	}

}

void power(void) {
	if (one && two && three && four) {
		pulse = duty[0];
		enable = 0;
		kers = 1;
	}
	if (one && two && three && fourOff) {
		pulse = duty[0];
		enable = 0;
		kers = 0;
	}
	if (one && two && threeOff && four) {
		pulse = duty[1];
		enable = 1;
		kers = 0;
	}
	if (one && two && threeOff && fourOff) {
		pulse = duty[2];
		enable = 1;
		kers = 0;
	}
	if (one && twoOff && three && four) {
		pulse = duty[3];
		enable = 1;
		kers = 0;
	}
	if (one && twoOff && three && fourOff) {
		pulse = duty[4];
		enable = 1;
		kers = 0;
	}
	if (one && twoOff && threeOff && four) {
		pulse = duty[5];
		enable = 1;
		kers = 0;
	}
	if (one && twoOff && threeOff && fourOff) {
		pulse = duty[6];
		enable = 1;
		kers = 0;
	}
	if (oneOff && two && three && four) {
		pulse = duty[7];
		enable = 1;
		kers = 0;
	}
	if (oneOff && two && three && fourOff) {
		pulse = duty[8];
		enable = 1;
		kers = 0;
	}
	if (oneOff && two && threeOff && four) {
		pulse = duty[9];
		enable = 1;
		kers = 0;
	}
	if (oneOff && two && threeOff && fourOff) {
		pulse = duty[10];
		enable = 1;
		kers = 0;
	}
	if (oneOff && twoOff && three && four) {
		pulse = duty[11];
		enable = 1;
		kers = 0;
	}
	if (oneOff && twoOff && three && fourOff) {
		pulse = duty[12];
		enable = 1;
		kers = 0;
	}
	if (oneOff && twoOff && threeOff && four) {
		pulse = duty[13];
		enable = 1;
		kers = 0;
	}
	if (oneOff && twoOff && threeOff && fourOff) {
		pulse = duty[14];
		enable = 1;
		kers = 0;
	}
	OCR0A = pulse;
	OCR1A = pulse;
	OCR2A = pulse;
}

int main(void) {

	//debug
	DDRB |= (1<<PB4);
	//speedometer
	DDRB |= (1 << PB6);

	DDRB |= (1 << PB1);	//SD PWM outs
	DDRD |= (1 << PD6);
	DDRB |= (1 << PB3);

	DDRD |= (1 << PD1);	//IN step outs
	DDRD |= (1 << PD2);
	DDRD |= (1 << PD3);

	DDRC &= ~(1 << PC1);	//hall ins
	DDRC &= ~(1 << PC2);
	DDRC &= ~(1 << PC3);

	//PORTC |= (1 << PC1);	//pull-up
	//PORTC |= (1 << PC2);
	//PORTC |= (1 << PC3);

	DDRB &= ~(1 << PB7);	//bus ins
	DDRD &= ~(1 << PD5);
	DDRD &= ~(1 << PD7);
	DDRB &= ~(1 << PB0);

	PORTB |= (1 << PB7);	//pull-up
	PORTD |= (1 << PD5);
	PORTD |= (1 << PD7);
	PORTB |= (1 << PB0);

	DDRC |= (1 << PC5);

	//ADC define
	ADMUX |= (1<<REFS0);
	//prescaler
	//ADCSRA |= (1 << ADPS1);//125kHz
	//ADCSRA |= (1 << ADPS2);

	ADCSRA |= (1 << ADPS0);		//1MHz
	ADCSRA |= (1 << ADPS1);

	ADCSRA |= (1 << ADEN); //ON

	//PWM define
	//timer 0
	OCR0A = 0;
	TCCR0A &= ~(1 << COM0A1);	//pwm off

	TCCR0A |= (1 << WGM01);	//mode
	TCCR0A |= (1 << WGM00);

	TCCR0B |= (1 << CS00);	//prescaler
	TCCR0B |= (1 << CS01);

	//timer1
	OCR1A = 0; //pwm out LOW
	TCCR1A &= ~(1 << COM1A1); //pwm off

	TCCR1A |= (1 << WGM10);	//mode
	TCCR1B |= (1 << WGM12);

	TCCR1B |= (1 << CS10); //prescaler-freq= clock/prescaler/256
	TCCR1B |= (1 << CS11);

	//timer2
	OCR2A = 0;
	TCCR2A &= ~(1 << COM2A1); //pwm off

	TCCR2A |= (1 << WGM20); //mode
	TCCR2A |= (1 << WGM21);

	TCCR2B |= (1 << CS22);
	TCCR2B &= ~(1 << CS21);

	while (1) {
		measurement();
		power();

		if ((sensor1 == 1) && (sensor2 == 0) && (sensor3 == 1)) {
			PORTB |= (1 << PB6); ///speedo pulse
			PORTB |= (1<<PB4);
		} else {
			PORTB &= ~(1 << PB6);
			PORTB &= ~(1<<PB4);
		}

		if (enable == 1 && kers == 0) {



			TCCR0B |= (1 << CS00);	//prescalers to 400Hz
			TCCR0B |= (1 << CS01);
			TCCR1B |= (1 << CS10);
			TCCR1B |= (1 << CS11);
			TCCR2B |= (1 << CS22);
			TCCR2B &= ~(1 << CS21);

			if ((sensor1 == 1) && (sensor2 == 0) && (sensor3 == 1)) {
				rotation(1);
				phase = 1;
			}

			if ((sensor1 == 1) && (sensor2 == 0) && (sensor3 == 0)) {
				rotation(2);
				phase = 2;
			}

			if ((sensor1 == 1) && (sensor2 == 1) && (sensor3 == 0)) {
				rotation(3);
				phase = 3;
			}

			if ((sensor1 == 0) && (sensor2 == 1) && (sensor3 == 0)) {
				rotation(4);
				phase = 4;
			}

			if ((sensor1 == 0) && (sensor2 == 1) && (sensor3 == 1)) {
				rotation(5);
				phase = 5;
			}

			if ((sensor1 == 0) && (sensor2 == 0) && (sensor3 == 1)) {
				rotation(0);
				phase = 0;
			}
		} else if (enable == 0 && kers == 1) {



			TCCR0B &= ~(1 << CS00);	//prescalers to 400Hz
			TCCR0B |= (1 << CS01);
			TCCR1B &= ~(1 << CS10);
			TCCR1B |= (1 << CS11);
			TCCR2B &= ~(1 << CS22);
			TCCR2B |= (1 << CS21);

			OCR1A = kersPower;
			TCCR1A |= (1 << COM1A1);
			PORTD &= ~(1 << PD1);

			OCR0A = kersPower;
			TCCR0A |= (1 << COM0A1);
			PORTD &= ~(1 << PD2);

			OCR2A = kersPower;
			TCCR2A |= (1 << COM2A1);
			PORTD &= ~(1 << PD3);

		} else {
			ALL_OFF
		}

	}
}
