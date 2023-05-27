#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>

#define LCD_Dir  DDRD		   	// Define LCD data port direction
#define LCD_Port PORTD			// Define LCD data port
#define RS PD2			      	// Define Register Select pin
#define EN PD3              // Define Enable pin

int potValue = 0;      // Declare the potValue variable
int temperature = 0;   // Declare the temperature variable

// Function declarations
int analogRead(uint8_t pin);
void adcInit(void);
void setup_TC0(void);
ISR(TIMER0_COMPA_vect);
void tostring(char str[], double num, int precision);
void reverse(char str[], int length);

char strin[15];

void LCD_Command( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0); // Set upper nibble
	LCD_Port &= ~ (1<<RS); // Set the RS pin to 0 (command)

	LCD_Port |= (1<<EN);	 // Set the EN pin to 1.
	_delay_us(1);          // Wait for 1 microsecond
	LCD_Port &= ~ (1<<EN); // Set the EN pin to 0

	_delay_us(200);        // Wait for 200 microseconds

	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);  // Set lower nibble
	LCD_Port |= (1<<EN);   // Set the EN pin to 1
	_delay_us(1);          // Wait for 1 microsecond
	LCD_Port &= ~ (1<<EN); // Set the EN pin to 0

	_delay_ms(2);          // Wait for 2 milliseconds
}

void LCD_Char( unsigned char data )
{

	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0); // Set upper nibble
	LCD_Port |= (1<<RS);		// Set the RS pin to 1 (data mode)
	LCD_Port|= (1<<EN);     // Set the EN pin to 1
	_delay_us(1);           // Wait for 1 microsecond
	LCD_Port &= ~ (1<<EN);  // Set the EN pin to 0

	_delay_us(200);         // Wait for 200 microseconds

	LCD_Port = (LCD_Port & 0x0F) | (data << 4);  // St the lower nibble
	LCD_Port |= (1<<EN);    // Set the EN pin to 1
	_delay_us(1);           // Wait for 1 millisecond
	LCD_Port &= ~ (1<<EN);  // Set the EN pin to 0
	_delay_ms(2);           // Wait for 2 milliseconds
}

void LCD_Init (void)		  // LCD Initialize function
{
	LCD_Dir = 0xFF;			    // Make LCD port direction as o/p
	_delay_ms(20);			    // LCD Power ON delay always >15ms
	LCD_Command(0x02);		  // send for 4 bit initialization of LCD
	LCD_Command(0x28);      // 2 line, 5*7 matrix in 4-bit mode
	LCD_Command(0x0c);      // Display on cursor off
	LCD_Command(0x06);      // Increment cursor (shift cursor to right)
	LCD_Command(0x01);      // Clear display screen
	_delay_ms(2);           // Wait for 2 milliseconds
}

void LCD_String (const char* str)
{
	int i;

	for(i=0;str[i]!=0;i++)		// Iterate through each character
	{
		LCD_Char(str[i]);       // Then display on the LCD screen
	}
}

void LCD_String_xy (char row, char pos,const char* str)
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);	// Command of first row and required position<16
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);	// Command of first row and required position<16
	LCD_String(str);	             	// Call LCD string function
}

void LCD_Clear()
{
	LCD_Command (0x01);		// Clear display
	_delay_ms(2);
	LCD_Command (0x80);		// Cursor at home position
}

void reverse(char str[], int length) {
	int start = 0;
	int end = length - 1;
	while (start < end) {
		char temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		start++;
		end--;
	}
}

void tostring(char str[], double num, int precision) {
	int i = 0;
	bool is_negative = false;

	// Handle negative numbers
	if (num < 0) {
		is_negative = true;
		num = -num;
	}

	// Convert the integer part
	int integerPart = (int)num;
	int integerLength = 0;
	if (integerPart == 0) {
		str[i++] = '0';
		integerLength = 1;
	}
	else {
		while (integerPart != 0) {
			int digit = integerPart % 10;
			str[i++] = digit + '0';
			integerPart /= 10;
			integerLength++;
		}
	}

	// Reverse the integer part
	reverse(str, integerLength);

	// Add the decimal point if precision > 0
	if (precision > 0) {
		str[i++] = '.';
		double fractionalPart = num - (int)num;

		// Convert the fractional part
		while (precision > 0) {
			fractionalPart *= 10;
			int digit = (int)fractionalPart;
			str[i++] = digit + '0';
			fractionalPart -= digit;
			precision--;
		}
	}

	// Null-terminate the string
	str[i] = '\0';

	// Add the sign if necessary
	if (is_negative) {
		int j = i;
		while (j >= 0) {
			str[j + 1] = str[j];
			j--;
		}
		str[0] = '-';
	}
}

void setup_TC0() {
	// Set frequency with OCR0A
	OCR0A = 255 - 1;
	//Set CTC mode
	TCCR0A |= (1 << WGM01);
	// Set prescalar to 1024
	TCCR0B |= (1 << CS02) | (1 << CS00);
	//Enable Compare Interrupt A
	TIMSK0 |= (1 << OCIE0A);
	// Enable global interrupts
	sei();
}

int analogRead(uint8_t pin) {
	ADMUX = (ADMUX & 0xF0) | (pin & 0x07); // Select the channel
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, start conversion, set prescaler
	while (ADCSRA & (1 << ADSC)); // Wait for conversion to complete
	return ADC;
}

void adcInit(void) {
	ADMUX |= (1 << REFS0); // Set the reference voltage to AVCC
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, start conversion, set prescaler
}

ISR(TIMER0_COMPA_vect) {

	// Read temperature from LM35
	int adc_result = analogRead(0); // Assuming the LM35 is connected to ADC channel 0
	// Convert the voltage into the temperature in degree Celsius:
	float voltage = (adc_result * 5.0 * 100)/1024; //the temperature reading is wrong because this equation is for lm35 sensor and not tmp36 and only sensor availble on tinkercad is tmp36
	float temperature = voltage;
	// Read potentiometer value
	potValue = analogRead(1); // Assuming the potentiometer is connected to ADC channel 1

	if (temperature > 35.0) {
		// Turn on buzzer
		PORTB ^= (1 << 0);
	}
	// Print values on the LCD
	LCD_Clear();
	LCD_String_xy(0, 0,"Temp: ");   tostring(strin, temperature, 2);
	LCD_String(strin);
	LCD_String(" C");
	LCD_String_xy(1, 0,"Pot: ");
	tostring(strin,potValue, 0);
	LCD_String(strin);
	
	_delay_ms(500);
}

int main() {
	DDRB |= (1 << 0); // Set PORTD3 as output for the buzzer
	DDRC &= ~(1 << PORTC1); // Set PORTC1 as input for the potentiometer
	// Initialize LCD
	LCD_Init();
	// Initialize ADC
	adcInit();
	// Initialize TC0 timer
	setup_TC0();
	// Initially cleared
	LCD_Clear();
	
	// Main loop
	while (1) {

	}
	return 0;
}