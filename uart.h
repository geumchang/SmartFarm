/*
 * uart.h
 *
 * Created: 2024-09-21 오후 11:17:13
 *  Author: jiung
 */ 


#ifndef UART_H_
#define UART_H_

void USART1_Init(unsigned int ubrr)
{
   // Set baud rate
   UBRR1H = (unsigned char)(ubrr>>8);
   UBRR1L = (unsigned char)ubrr;
   // Enable receiver and transmitter
   UCSR1B = (1<<RXEN1)|(1<<TXEN1)|(1<<RXCIE1);
   // Set frame format: 8data, 1stop bit
   UCSR1C = (3<<UCSZ10);
}

void USART1_Transmit(unsigned char data)
{
   // Wait for empty transmit buffer
   while (!(UCSR1A & (1<<UDRE1)));
   // Put data into buffer, sends the data
   UDR1 = data;
}
void USART0_Init(unsigned int ubrr)
{
   // Set baud rate
   UBRR0H = (unsigned char)(ubrr>>8);
   UBRR0L = (unsigned char)ubrr;
   // Enable receiver and transmitter
   UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
   // Set frame format: 8data, 1stop bit
   UCSR0C = (3<<UCSZ00);
}

void USART0_Transmit(unsigned char data)
{
   // Wait for empty transmit buffer
   while (!(UCSR0A & (1<<UDRE0)));
   // Put data into buffer, sends the data
   UDR0 = data;
}

void send_string1(char *str)
{
   while(*str)
   {
      USART1_Transmit(*str);
      str++;
   }
}
void send_string0(char *str)
{
   while(*str)
   {
      USART0_Transmit(*str);
      str++;
   }
}


void sendBuff_USART1(char *str,int length)
{
   while (length--)
   {
      USART1_Transmit(*str++);
   }
}

void send_packet_USART0(unsigned char* packet, int length) {
   for(int i = 0; i < length; i++) {
      USART0_Transmit(packet[i]);
   }
}


#endif /* UART_H_ */
