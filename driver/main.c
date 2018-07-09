/*
 * main.c
 *
 *  Created on: 12 lut 2018
 *      Author: Bartosz Pracz
 *     	blink-1 driver

 *      v0.2 - working, first test
 *      v0.3- beta amperometer
 *      v0.4- custom characters, notify
 *      v0.5- kers support
 *     	v0.6- better security
 *     	v0.7- throttle steps as array
 *     	v0.8- upgrade to 1.5kW :)
 *     	v0.9-fixed amperometer
 *     	v1.1- more amp measurement options
 *     	v1.2.1- fixed for ACS712 with current divider, display refresh, faster speedo
 *     	v1.3 - 2kW :)
 *     	v1.4 - fixed overdraining battery, faster screen refresh
 *     	v1.5 - safeLoop notify, faster speedo, Overload notify
 *     	Milestone 1- All working?
 *     	v2.0 - added menu
 *     	v2.0.1 - better overload secure
 *     	v2.1 - speed limit, optimalization.
 *
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "hd44780.h"
#include <avr/interrupt.h>

EEMEM unsigned int s; // speedo
EEMEM unsigned int a; // amperage
EEMEM unsigned int v; // voltage
EEMEM unsigned int l; // speed limit
EEMEM unsigned int m; // mode

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
volatile int throStep[] = { 210, 250, 300, 350, 400, 450, 500, 550, 600, 650,
		675, 690, 720, 750 };
int maxDuty = 15;

//modes
int mode;
int kers = 0;
int ecoLimit = 9;
int smartLimit = 12;

//menu
int menu = 0;
int option = 0;
int speedoCal;
int ampCal;
int voltCal;
int speedLimitCal;
float minVoltageDef = 39;
int maxAmperageDef = 40;
int speedoDef = 15;
int speedLimitDef = 99;
int modeDef = 3;

//voltages
volatile float voltage;
volatile float difference;
int minVoltage;

//amperages
volatile int amperage = 0;
int maxAmperage;

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
volatile int count = 0;
volatile int speed;
int speedo;
int speedLimit;

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
	//amperage = ADC / 7 - 73; //for 2xACS712-30A
	//amperage = ADC /14 - 36; //for ACS712-30A
	//amperage = ADC / 20 - 25; //for ACS712-20A
	//amperage =( ADC /14 - 36) * 5; //for ACS712-30A with divider
	amperage = ADC / 4 - 128; //for ACS712-20A with divider
}

void lcdSplash() {
	lcd_init();
	lcd_clrscr();
	lcd_puts("Bart's");
	lcd_goto(64);
	lcd_puts("blink-1");
	_delay_ms(1000);
}

void lcdRef() {
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
	lcd_goto(66);
	lcd_putc(2);
	lcd_goto(68);
	lcd_puts("D");
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

	if ((counter % 500) == 0) {
		lcd_goto(notifyDisplay);
		lcd_puts("    ");
	}

	if (voltage < minVoltage + 2) {
		lcd_goto(notifyDisplay);
		lcd_puts("W");
	}

	if (amperage > maxAmperage) {
		lcd_goto(notifyDisplay + 1);
		lcd_puts("O");
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
		throttleCheck();

		if (throttle <= throStep[0]) {

			if (!(PINB & (1 << PB4))) {
				if (kers == 1) {
					kers = 0;
					lcd_goto(64);
					lcd_puts(" ");
					_delay_ms(800);
				} else if (kers == 0) {
					kers = 1;
					lcd_goto(64);
					lcd_puts("K");
					_delay_ms(800);
				}
			} else {

				if (mode <= 2) {
					mode++;
					_delay_ms(800);
				} else {
					mode = 1;
					_delay_ms(800);
				}
				eeprom_write_byte(&m, mode);
				_delay_ms(20);
			}

		}

		else {
			menu = 1;
			lcd_clrscr();
			lcd_goto(0);
			lcd_puts("blink-1. menu:");
			_delay_ms(800);

			while (menu == 1) {

				power(1);
				throttleCheck();
				if (throttle < throStep[1]) {
					lcd_goto(64);
					lcd_puts("speedo calibrate");
					option = 1;
				}
				if (throttle >= throStep[1] && throttle < throStep[3]) {
					lcd_goto(64);
					lcd_puts("max amperage    ");
					option = 2;
				}

				if (throttle >= throStep[3] && throttle < throStep[5]) {
					lcd_goto(64);
					lcd_puts("minimal voltage  ");
					option = 3;
				}
				if (throttle >= throStep[5] && throttle < throStep[7]) {
					lcd_goto(64);
					lcd_puts("speed limit      ");
					option = 4;

				}
				if (throttle >= throStep[7] && throttle < throStep[9]) {
					lcd_goto(64);
					lcd_puts("check values     ");
					option = 5;
				}
				if (throttle >= throStep[9] && throttle < throStep[11]) {
					lcd_goto(64);
					lcd_puts("reset defaults   ");
					option = 6;
				}
				if (throttle >= throStep[11]) {
					lcd_goto(64);
					lcd_puts("exit             ");
					option = 7;
				}
				if (!(PINB & (1 << PB4))) {

					_delay_ms(800);
					if (option == 7) {
						throttleCheck();
						while (throttle > throStep[0]) {
							lcd_goto(64);
							lcd_puts("release throttle");
							throttleCheck();
						}
						menu = 0;
						lcdRef();
						maxDuty = 15;
					}
					while (option == 1) {
						lcd_goto(64);
						lcd_puts("speedo Cal:   ");
						lcd_goto(78);
						itoa(speedoCal, buffer, 10);
						lcd_puts(buffer);
						throttleCheck();
						speedoCal = throttle / 40;
						if (speedoCal < 10)
							speedoCal = 10;
						if (!(PINB & (1 << PB4))) {
							speedo = speedoCal;
							lcd_goto(64);
							lcd_puts("set: ");
							itoa(speedo, buffer, 10);
							lcd_puts(buffer);
							lcd_puts("         ");
							_delay_ms(800);
							option = 0;
							eeprom_write_byte(&s, speedo);
							_delay_ms(20);

						}
					}
					while (option == 2) {
						lcd_goto(64);
						lcd_puts("max Amp:      ");
						lcd_goto(78);
						itoa(ampCal, buffer, 10);
						lcd_puts(buffer);
						if (ampCal <= 9)
							lcd_puts(" ");
						throttleCheck();
						ampCal = (throttle / 7) - 22;
						if (ampCal > 99)
							ampCal = 99;
						if (!(PINB & (1 << PB4))) {
							maxAmperage = ampCal;
							lcd_goto(64);
							lcd_puts("set: ");
							itoa(maxAmperage, buffer, 10);
							lcd_puts(buffer);
							lcd_puts("         ");
							_delay_ms(800);
							option = 0;
							eeprom_write_byte(&a, maxAmperage);
							_delay_ms(20);

						}
					}

					while (option == 3) {
						lcd_goto(64);
						lcd_puts("min Volt:     ");
						lcd_goto(78);
						itoa(voltCal, buffer, 10);
						lcd_puts(buffer);
						throttleCheck();
						if (throttle < throStep[0])
							voltCal = 30;
						if (throttle >= throStep[1] && throttle < throStep[4])
							voltCal = 39;
						if (throttle >= throStep[4] && throttle < throStep[8])
							voltCal = 60;
						if (!(PINB & (1 << PB4))) {
							minVoltage = voltCal;
							lcd_goto(64);
							lcd_puts("set: ");
							itoa(minVoltage, buffer, 10);
							lcd_puts(buffer);
							lcd_puts("         ");
							_delay_ms(800);
							option = 0;
							eeprom_write_byte(&v, minVoltage);
							_delay_ms(20);

						}
					}

					while (option == 5) {
						lcd_goto(64);
						lcd_puts("speedo:       ");
						lcd_goto(78);
						itoa(speedo, buffer, 10);
						lcd_puts(buffer);
						_delay_ms(800);

						lcd_goto(64);
						lcd_puts("Max amperage: ");
						lcd_goto(78);
						itoa(maxAmperage, buffer, 10);
						lcd_puts(buffer);
						lcd_puts("    ");
						_delay_ms(800);

						lcd_goto(64);
						lcd_puts("min Voltage:  ");
						lcd_goto(78);
						itoa(minVoltage, buffer, 10);
						lcd_puts(buffer);
						_delay_ms(800);
						option = 0;

						lcd_goto(64);
						lcd_puts("speed limit:  ");
						lcd_goto(78);
						itoa(speedLimit, buffer, 10);
						lcd_puts(buffer);
						_delay_ms(800);
						option = 0;

					}

					while (option == 6) {
						lcd_goto(64);
						lcd_puts("restoring...    ");
						speedo = speedoDef;
						maxAmperage = maxAmperageDef;
						minVoltage = minVoltageDef;
						speedLimit = speedLimitDef;
						eeprom_write_byte(&s, speedo);
						_delay_ms(20);
						eeprom_write_byte(&a, maxAmperage);
						_delay_ms(20);
						eeprom_write_byte(&v, minVoltage);
						_delay_ms(20);
						eeprom_write_byte(&l, speedLimit);
						_delay_ms(20);
						_delay_ms(800);
						option = 0;

					}

					while (option == 4) {
						lcd_goto(64);
						lcd_puts("Speed Limit:  ");
						lcd_goto(78);
						itoa(speedLimitCal, buffer, 10);
						lcd_puts(buffer);
						if (speedLimitCal <= 9)
							lcd_puts(" ");
						throttleCheck();
						speedLimitCal = (throttle / 7) - 22;
						if (speedLimitCal > 99)
							speedLimitCal = 99;
						if (!(PINB & (1 << PB4))) {
							speedLimit = speedLimitCal;
							lcd_goto(64);
							lcd_puts("set: ");
							itoa(speedLimit, buffer, 10);
							lcd_puts(buffer);
							lcd_puts("         ");
							_delay_ms(800);
							option = 0;
							eeprom_write_byte(&l, speedLimit);
							_delay_ms(20);

						}
					}

				}
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
	ADMUX |= (1 << REFS0);
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

	lcdSplash();
	lcdRef();

//speedometer

	EICRA |= (1 << ISC11); // falling edge on INT1
	EIMSK |= (1 << INT1); //enable interrupts on INT1
	TCCR1B |= (1 << WGM12); //CTC, OCR1A
	TCCR1B |= (1 << CS12); //set prescaler for 1024
	TCCR1B |= (1 << CS10);

	TIMSK1 |= (1 << OCIE1A);
	sei();

	//OCR1A = 7812;
	OCR1A = 3906;
	DDRD &= ~(1 << PD3);
	PORTD |= (1 << PD3);

	//read eeprom
	minVoltage = eeprom_read_byte(&v);
	if (minVoltage > 60)
		minVoltage = minVoltageDef;
	maxAmperage = eeprom_read_byte(&a);
	if (maxAmperage > 99)
		maxAmperage = maxAmperageDef;
	speedo = eeprom_read_byte(&s);
	if (speedo > 50)
		speedo = speedoDef;
	speedLimit = eeprom_read_byte(&l);
			if (speedLimit > 99)
				speedLimit = speedLimitDef;
	mode = eeprom_read_byte(&m);
		if (mode > 3)
			mode = modeDef;

	while (1) {

		modes();

		batteryCheck();
		ampCheck();
		throttleCheck();

		dThrottle();
		dKers();
		dNotify();

		//Non Continous operations

		counter++;
		if (counter > 32000)
			counter = 1;

		if ((counter % 1000) == 0) {
			lcdRef();
		}

		if ((counter % 50) == 0)
			if (maxDuty < 15)
				maxDuty++; //slow maxDuty reset

		if ((counter % 100) == 0) {
			dVolts();
			dAmpers();
		}

		if ((counter % 50) == 0) {
			dSpeed();
		}

		//end of non continous operation

		//send signal
		if (throttle < throStep[0] && kers == 1) {
			power(0); //when power zero, send 0 enable and 1 kers
		} else if (throttle < throStep[0] && kers == 0) {
			power(1); //wher power one, send 0 enable and 0 kers

		} else {
			if (throttle >= throStep[0])
				duty = 2;
			if (throttle >= throStep[1])
				duty = 3;
			if (throttle >= throStep[2])
				duty = 4;
			if (throttle >= throStep[3])
				duty = 5;
			if (throttle >= throStep[4])
				duty = 6;
			if (throttle >= throStep[5])
				duty = 7;
			if (throttle >= throStep[6])
				duty = 8;
			if (throttle >= throStep[7])
				duty = 9;
			if (throttle >= throStep[8])
				duty = 10;
			if (throttle >= throStep[9])
				duty = 11;
			if (throttle >= throStep[10])
				duty = 12;
			if (throttle >= throStep[11])
				duty = 13;
			if (throttle >= throStep[12])
				duty = 14;
			if (throttle >= throStep[13])
				duty = 15;

			if (mode == 1 && duty > 9) //eco mode
				duty = 9;
			if (mode == 2 && duty > 12) //smart mode
				duty = 12;

			if (duty > maxDuty)
				duty = maxDuty;

			power(duty); //send power pattern

			//overload secure, over discharge secure, speedLimit secure
			if (amperage > maxAmperage || voltage < minVoltage || speed > speedLimit) {
				maxDuty = duty - 1;
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
	speed = (count * 10) / speedo;
	if (speed > 99)
		speed = 99;
	count = 0;
}

