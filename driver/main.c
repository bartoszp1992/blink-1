/*
 * main.c
 *
 *  Created on: 12 lut 2018
 *      Author: Bartosz Pracz
 *     	blink-1 DRIVER

 *      v0.2 - working, first test
 *      v0.3- beta amperometer also in back
 *      v0.4- custom characters, notify
 *      v0.5- kers support
 *     	v0.6- better security
 *     	v0.7- throttle steps as array
 *     	v0.8- upgrade to 1.5kW :)
 *
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "hd44780.h"
#include <avr/interrupt.h>

#define one PORTD |= (1<<PD0)
#define two PORTD |= (1<<PD1)
#define three PORTD |= (1<<PD2)
#define four PORTB |= (1<<PB6)

#define oneOff PORTD &= ~(1<<PD0)
#define twoOff PORTD &= ~(1<<PD1)
#define threeOff PORTD &= ~(1<<PD2)
#define fourOff PORTB &= ~(1<<PB6)

//variables
//main
volatile int throttle = 0;
volatile int duty = 0;
volatile int throStep[] = {210, 250, 300, 350, 400, 450, 500, 550, 600, 650, 675, 690, 720, 750};

//modes
int mode = 3;
int kers = 0;

//voltages
volatile float voltage;
volatile float difference;
volatile float minVoltage = 39;

//amperages
volatile int amperage = 0;
int maxAmperage = 28 ;

//lcd
int modeDisplay = 0;
int voltageDisplay = 11;
int throttleDisplay = 67;
int kersDisplay = 65;
int ampDisplay = 6;
int speedDisplay = 70;
int notifyDisplay = 76;
char buffer[10];
int counter = 1;

//speedometer
volatile unsigned int count = 0;
volatile unsigned int rps;
volatile unsigned int speed;

int itoa();
int sprintf(char *str, const char *format, ...);

void batteryCheck(void) {
	ADMUX |= (1 << MUX1);		//ADC3(battery)
	ADMUX |= (1 << MUX0);
	ADCSRA |= (1 << ADSC); //start
	while (ADCSRA & (1 << ADSC))
		;
	voltage = ADC / 14.4;
}

void throttleCheck() {
	ADMUX |= (1 << MUX1);		//ADC2(throttle)
	ADMUX &= ~(1 << MUX0);
	ADCSRA |= (1 << ADSC); //start
	while (ADCSRA & (1 << ADSC))
		;
	throttle = ADC;
}

void ampCheck() {
	ADMUX &= ~(1 << MUX1);		//ADC0(amp) 205/V, 925/4,5V
	ADMUX &= ~(1 << MUX0);
	ADCSRA |= (1 << ADSC); //start
	while (ADCSRA & (1 << ADSC))
		;

	amperage = ((ADC* 100)/137)/10 -37 ;
}

void power(int power) {

	if (power == 0) {
		one;
		two;
		three;
		four;
	}
	if (power == 1) {
		one;
		two;
		three;
		fourOff;
	}
	if (power == 2) {
		one;
		two;
		threeOff;
		four;
	}
	if (power == 3) {
		one;
		two;
		threeOff;
		fourOff;
	}
	if (power == 4) {
		one;
		twoOff;
		three;
		four;
	}
	if (power == 5) {
		one;
		twoOff;
		three;
		fourOff;
	}
	if (power == 6) {
		one;
		twoOff;
		threeOff;
		four;
	}
	if (power == 7) {
		one;
		twoOff;
		threeOff;
		fourOff;
	}
	if (power == 8) {
		oneOff;
		two;
		three;
		four;
	}
	if (power == 9) {
		oneOff;
		two;
		three;
		fourOff;
	}
	if (power == 10) {
		oneOff;
		two;
		threeOff;
		four;
	}
	if (power == 11) {
		oneOff;
		two;
		threeOff;
		fourOff;
	}
	if (power == 12) {
		oneOff;
		twoOff;
		three;
		four;
	}
	if (power == 13) {
		oneOff;
		twoOff;
		three;
		fourOff;
	}
	if (power == 14) {
		oneOff;
		twoOff;
		threeOff;
		four;
	}
	if (power == 15) {
		oneOff;
		twoOff;
		threeOff;
		fourOff;
	}
}

void dNotify() {
	if (voltage < 42) {
		lcd_goto(notifyDisplay);
		lcd_puts("weak");
	}

	if ((counter % 500) == 0) {
		lcd_goto(notifyDisplay);
		lcd_puts("    ");
	}
}

void dAmpers() {
	//display amperage
	if (amperage >= 10) {
		itoa(amperage, buffer, 10);
		lcd_goto(ampDisplay);
		lcd_puts("     ");
		lcd_goto(ampDisplay);
		lcd_puts(buffer);
		lcd_puts("A");
	}

	if (amperage < 10 && amperage >= 0) {
		itoa(amperage, buffer, 10);
		lcd_goto(ampDisplay);
		lcd_puts("     ");
		lcd_goto(ampDisplay);
		lcd_puts(buffer);
		lcd_puts("A");
	}
	if (amperage < 0 && amperage > -10) {
		itoa(amperage, buffer, 10);
		lcd_goto(ampDisplay);
		lcd_puts("     ");
		lcd_goto(ampDisplay);
		lcd_puts(buffer);
		lcd_puts("A");
	}
	if (amperage <= -10) {
		itoa(amperage, buffer, 10);
		lcd_goto(ampDisplay);
		lcd_puts("     ");
		lcd_goto(ampDisplay);
		lcd_puts(buffer);
		lcd_puts("A");
	}
}

void dVolts() {
	//display voltage
	if (voltage >= 10) {
		lcd_goto(voltageDisplay);
		sprintf(buffer, "%2.1fV", voltage);
		lcd_puts(buffer);
		lcd_puts("   ");
	}

	if (voltage < 10) {
		lcd_goto(voltageDisplay);
		lcd_puts(" ");
		sprintf(buffer, "%1.1fV", voltage);
		lcd_puts(buffer);
		lcd_puts("   ");
	}
}

void dThrottle() {
	//display throttle

	lcd_goto(throttleDisplay);
	if (throttle < throStep[0])
		lcd_puts(" ");
	if (throttle >= throStep[0])
		lcd_putc(0);
}

void dKers() {

	lcd_goto(kersDisplay);
	if (amperage < 0) {
		lcd_putc(1);
	} else {
		lcd_puts(" ");
	}
}

void dSpeed() {
	itoa(speed, buffer, 10);
	lcd_goto(speedDisplay);
	;
	lcd_puts(buffer);
	if (speed < 10)
		lcd_puts("kmh ");
	if (speed >= 10)
		lcd_puts("kmh");
}

void modes() {
//mode select
	if (!(PINB & (1 << PB4))) {
		_delay_ms(800);

		if (!(PINB & (1 << PB4))) {
			if (kers == 1) {
				kers = 0;
				lcd_goto(64);
				lcd_puts(" ");
				_delay_ms(500);
			} else if (kers == 0) {
				kers = 1;
				lcd_goto(64);
				lcd_puts("K");
				_delay_ms(500);
			}
		} else {

			if (mode <= 2) {
				mode++;
				_delay_ms(500);
			} else {
				mode = 1;
				_delay_ms(500);
			}
		}
	}

//mode
	if (mode == 1) {
		lcd_goto(modeDisplay);
		lcd_puts("eco  ");
	}

	if (mode == 2) {
		lcd_goto(modeDisplay);
		lcd_puts("smart");
	}

	if (mode == 3) {
		lcd_goto(modeDisplay);
		lcd_puts("sport");
	}
	if (kers == 1) {
		lcd_goto(64);
		lcd_puts("K");
	}
	if (kers == 0) {
		lcd_goto(64);
		lcd_puts(" ");
	}

}

int main(void) {

//debug led(kers ON)
//DDRC |= (1 << PC5);
//DDRC |= (1 << PC4);

//button
	DDRB &= ~(1 << PB4);
	PORTB |= (1 << PB4);

//ADC define
	ADMUX |= (1<<REFS0);
	ADCSRA |= (1 << ADPS1); //prescaler
	ADCSRA |= (1 << ADPS2);
	ADCSRA |= (1 << ADEN); //ON
//ADMUX |= (1 << REFS0); //reference voltage
//pin select
	ADMUX |= (1 << MUX1);	//ADC3(battery)
	ADMUX |= (1 << MUX0);

	ADMUX |= (1 << MUX1);	//ADC2(throttle)
	ADMUX &= ~(1 << MUX0);

	ADMUX |= (1 << MUX0);	//ADC1(kers)
	ADMUX &= ~(1 << MUX1);

//data bus for BLDC driver
	DDRD |= (1 << PD0);
	DDRD |= (1 << PD1);
	DDRD |= (1 << PD2);
	DDRB |= (1 << PB6);

	PORTD |= (1 << PD0);
	PORTD |= (1 << PD1);
	PORTD |= (1 << PD2);
	PORTB |= (1 << PB6);

	lcd_init();

	//right
	lcd_command(_BV(LCD_CGRAM) + 0 * 8);
	lcd_putc(0b00000);
	lcd_putc(0b10000);
	lcd_putc(0b11100);
	lcd_putc(0b11111);
	lcd_putc(0b11100);
	lcd_putc(0b10000);
	lcd_putc(0b00000);
	lcd_putc(0b00000);
	lcd_goto(0);

	//left
	lcd_command(_BV(LCD_CGRAM) + 1 * 8);
	lcd_putc(0b00000);
	lcd_putc(0b00001);
	lcd_putc(0b00111);
	lcd_putc(0b11111);
	lcd_putc(0b00111);
	lcd_putc(0b00001);
	lcd_putc(0b00000);
	lcd_putc(0b00000);
	lcd_goto(0);

	//sep
	lcd_command(_BV(LCD_CGRAM) + 2 * 8);
	lcd_putc(0b01110);
	lcd_putc(0b01110);
	lcd_putc(0b01110);
	lcd_putc(0b01110);
	lcd_putc(0b01110);
	lcd_putc(0b01110);
	lcd_putc(0b01110);
	lcd_putc(0b01110);
	lcd_goto(0);

	lcd_clrscr();
	lcd_puts("Bart's");
	lcd_goto(64);
	lcd_puts("blink-1");
	_delay_ms(1000);
	lcd_clrscr();
	lcd_goto(66);
	lcd_putc(2);
	lcd_goto(68);
	lcd_puts("D");

//speedometer

	EICRA |= (1 << ISC11); // falling edge on INT1
	EIMSK |= (1 << INT1); //enable interrupts on INT1
	TCCR1B |= (1 << WGM12); //CTC, OCR1A
	TCCR1B |= (1 << CS12); //set prescaler for 1024
	TCCR1B |= (1 << CS10);

	TIMSK1 |= (1 << OCIE1A);
	sei();

	OCR1A = 7812;
	DDRD &= ~(1 << PD3);
	PORTD |= (1 << PD3);

	while (1) {

		modes();

		batteryCheck();
		ampCheck();
		throttleCheck();

		dThrottle();
		dKers();
		dNotify();

		counter++;
		if (counter > 32000)
			counter = 1;

		if ((counter % 150) == 0) {
			dVolts();
			dAmpers();
			dSpeed();
		}

		//send signal
		if (throttle < throStep[0] && kers == 1) {
			power(0); //when power zero, send 0 enable and 1 kers
		} else if (throttle < throStep[0] && kers == 0) {
			power(1); //wher power one, send 0 enable and 0 kers

		} else {
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[0])
				duty = 2;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[1])
				duty = 3;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[2])
				duty = 4;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[3])
				duty = 5;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[4])
				duty = 6;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[5])
				duty = 7;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[6] && mode > 1) //mid
				duty = 8;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[7] && mode > 1)
				duty = 9;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[8] && mode > 1)
				duty = 10;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[9] && mode > 1)
				duty = 11;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[10] && mode > 2) //pro
				duty = 12;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[11] && mode > 2)
				duty = 13;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[12] && mode > 2)
				duty = 14;
			if (amperage < maxAmperage && voltage > minVoltage && throttle >= throStep[13] && mode > 2)
				duty = 15;
			power(duty);

			batteryCheck();
			ampCheck();

			while (((voltage <= minVoltage) || (amperage > maxAmperage))
					&& throttle >= 200) {
				power(duty - 1);
				batteryCheck();
				ampCheck();
				throttleCheck();
			}

		}
	}
}

ISR(INT1_vect) {
//CPU Jumps here automatically when INT0 pin detect a falling edge
	count++;
}

ISR(TIMER1_COMPA_vect) {
//CPU Jumps here every 1 sec exactly!
	speed = count / 3;
	if(speed > 99) speed = 99;
	count = 0;
}

