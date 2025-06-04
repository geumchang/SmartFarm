
#define F_CPU 14745600UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <math.h>

#include "uart.h"
#include "DS1302.h"
#include "ControlPump.h"

#define BAUD0 9600
#define BAUD1 115200
#define MYUBRR0 F_CPU/16/BAUD0-1
#define MYUBRR1 F_CPU/16/BAUD1-1

// pH 임계값 설정
#define PH_LOW_THRESHOLD 600  // pH 6.0
#define PH_HIGH_THRESHOLD 700 // pH 7.0

// pH 임계값 설정
#define HUM_LOW_THRESHOLD 300  // 습도 3.0
#define HUM_HIGH_THRESHOLD 500 // 습도 5.0
void ControlsPUMP(uint8_t enable);

#define RESPONSE_LENGTH 13 // => 매직 넘버보단 이게 나음 

   /*Packet*/
#define ID 0x01
#define FUNCTION 0x02
#define CHECK_THE_ADDRESS 0xF0


/* SPI*/
#define SLAVE_PACKET_SIZE 9
#define SS_LOW PORTB &= ~(1 << PB0) //select
#define SS_HIGH PORTB |= (1 << PB0) // deselect

volatile uint8_t receivedPacket[SLAVE_PACKET_SIZE];
volatile uint8_t packetIndex = 0;
volatile uint8_t packetReceived = 0;

   /*Variable*/
volatile uint8_t rx_buffer[20];
volatile uint8_t rx_count = 0;
volatile uint8_t rx_complete = 0;

unsigned char BufferPacket[13] = {0,};
unsigned char AllSenSorBufferPacket[17] = {0,};



uint8_t inquiry_frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09};
   
   /*Function*/
   
void USART1_Init(unsigned int ubrr);
void USART1_Transmit(unsigned char data);
void USART0_Init(unsigned int ubrr);
void USART0_Transmit(unsigned char data);
void sendBuff_USART1(char *str,int length);
void send_packet_USART0(unsigned char* packet, int length);
void send_string1(char *str);
void send_string0(char *str);

void send_inquiry_frame();
void process_response();

unsigned short CRC16_modbus(unsigned char *addr, int num);
void GetArray(uint16_t data, unsigned char start_index, unsigned char* buffer);
void makePacket(unsigned char id, unsigned char function, uint16_t humidity, uint16_t temperature,uint16_t conductivity, uint16_t ph);

// SPI FUNCTION
void SPI_MasterInit(void);
void startReceive(void);

ISR(USART0_RX_vect)
{
   rx_buffer[rx_count++] = UDR0;
   
   if(rx_count >= RESPONSE_LENGTH)  // Expected response length
   {
      
	  process_response(); // 토양 센서 값 저장 및 패킷 생성
	  rx_count = 0;
	  
   }
}


ISR(SPI_STC_vect)
{
	uint8_t received = SPDR;
	
	if (packetIndex == 0 && received != 0x02) {
		// 첫 바이트가 예상한 ID가 아니면 무시
		return;
	}
	
	receivedPacket[packetIndex++] = received;
	
	if (packetIndex >= SLAVE_PACKET_SIZE)
	{
		packetReceived = 1;
		packetIndex = 0;
		SS_HIGH;  // 패킷 수신 완료 후 슬레이브 선택 해제
		
		checkCRC_and_ProcessPacket();
	}
	else {
		SPDR = 0xFF;  // 다음 바이트 요청
	}
}

int main(void)
{
   USART0_Init(MYUBRR0);  // 센서 통신용 9600 baud
   USART1_Init(MYUBRR1);  // ESP 통신용 115200 baud
   
   //SPI
   SPI_MasterInit();
   
   sei();
   
   DS1302_Init();  // DS1302 초기화
   
   Time current_time;
   Time initial_time = {0, 0, 12};  // {초, 분, 시}
   uint8_t previous_minute = 0xFF;
   DS1302_SetTime(initial_time);
   
   int minutecount = 0;
   while(1)
   {
      current_time = DS1302_ReadTime();  // DS1302에서 현재 시간을 읽음
      
      if (current_time.minute != previous_minute)
      {
	      previous_minute = current_time.minute;  // 이전 분 값 업데이트
		  send_inquiry_frame();
		  _delay_ms(100);
		  
		  if(minutecount == 1)
		  {
			  SS_LOW;
			  SPDR = 0xff;
			  minutecount = 0;
		  }else;
		  minutecount += 1;
	  }
	}
   return 0;
}

unsigned short CRC16_modbus(unsigned char *addr, int num)
{
   unsigned short CRC = 0xFFFF;
   int i;
   while (num--)
   {
      CRC ^= *addr++;
      for (i = 0; i < 8; i++)
      {
         if (CRC & 1)
         {
            CRC >>= 1;
            CRC ^= 0xA001;
         }
         else
         {
            CRC >>= 1;
         }
      }
   }
   return CRC;
}

void GetArray(uint16_t data, unsigned char start_index, unsigned char* buffer)
{
   buffer[start_index++] = (unsigned char)(data >> 8);
   buffer[start_index++] = (unsigned char)data;
}

void makePacket(unsigned char id, unsigned char function, uint16_t humidity, uint16_t temperature,uint16_t conductivity, uint16_t ph)
{
   BufferPacket[0] = id;
   
   BufferPacket[1] = function;
   
   GetArray(humidity, 2, BufferPacket); // 2 3
   //if(data< 0){ --BufferPacket[4]; }
   GetArray(temperature, 4, BufferPacket);// 4 5
   GetArray(conductivity, 6, BufferPacket);// 6 7
   GetArray(ph, 8, BufferPacket);// 8 9
   //if(ang< 0){ --BufferPacket[10]; }
   
   unsigned short CRC = CRC16_modbus(BufferPacket, 10);
   
   BufferPacket[10] = (CRC & 0xFF);
   BufferPacket[11] = (CRC >> 8);
   
   BufferPacket[12] = CHECK_THE_ADDRESS;
   _delay_ms(50);
}

void send_inquiry_frame()
{

   for(int i = 0; i < sizeof(inquiry_frame); i++)
   {
      USART0_Transmit(inquiry_frame[i]);
   }
   
}

// [Address][Function][valid][Humidity][Temperature][Conductivity][pH][CRC][CRC]
// [8bit][8bit][8bit][hum_L][hum_H][Tem_L][Tem_H][con_L][con_H][pH_L][pH_H][CRC_L][CRC_H]
void process_response()
{
   
   if(rx_buffer[0] != 0x01 || rx_buffer[1] != 0x03 || rx_buffer[2] != 0x08)
   {
      //send_string1("Invalid response received\r\n");
	  memset((unsigned char*)rx_buffer, 0, sizeof(rx_buffer));  // 패킷 버퍼를 0으로 초기화
	  
      return;
   }

   uint16_t humidity = (rx_buffer[3] << 8) | rx_buffer[4];
   int16_t temperature = (rx_buffer[5] << 8) | rx_buffer[6];
   uint16_t conductivity = (rx_buffer[7] << 8) | rx_buffer[8];
   uint16_t ph = (rx_buffer[9] << 8) | rx_buffer[10];
   
   if( humidity < HUM_LOW_THRESHOLD)
   {
	   ControlsPUMP_water(1);
   }
   else
   {
	   ControlsPUMP_water(0);
   }
   
/*
   if (ph < PH_LOW_THRESHOLD || ph > PH_HIGH_THRESHOLD)
   {
	   
	   ControlsPUMP_antel(1);  // 펌프 켜기
   }
   else
   {
	   ControlsPUMP_antel(0);  // 펌프 끄기
   }*/
   
   makePacket(ID,FUNCTION,humidity,temperature,conductivity,ph);
   
   rx_count = 0;
   
   
   
   
   // pH에 따른 펌프 제어(ph 임계값을 넘지 못하면 펌프가 계속 작동하는 문제 해결 필요)
   // watchdok으로 timeout 설정이 필요함
   
   
      
   _delay_ms(10);
}
	


/*SPI*/


void SPI_MasterInit(void)
{
	DDRB = (1 << PB1) | (1 << PB2) | (1 << PB0);  // Set SCK, MOSI, SS as output, MISO as input
	DDRB &= ~(1 << PB3);
	

	SPCR = (1 << SPIE) |(1 << SPE) | (1 << MSTR)| (1 << SPR0);  // Enable SPI, set as Master, set clock rate fck/16
	
	//SPSR = 0x00;
	SS_HIGH;//초기에 슬레이브 선택 해제
}



// 마스터 쪽에서 패킷 수신 완료 후 CRC 검증
void checkCRC_and_ProcessPacket(void)
{
	// 수신된 데이터에서 CRC 부분을 제외한 6바이트에 대해 CRC 계산
	unsigned short receivedCRC = (receivedPacket[7] << 8) | receivedPacket[6]; // 수신된 CRC (상위 바이트 << 8 | 하위 바이트)
	unsigned short calculatedCRC = CRC16_modbus(receivedPacket, 6);  // ID부터 데이터까지 6바이트에 대해 CRC 계산

	if (receivedCRC == calculatedCRC)
	{
		// CRC가 일치하면 패킷 유효함

		// BufferPacket의 [0]~[9]을 AllSenSorBufferPacket의 [0]~[9]으로 복사
		memcpy(&AllSenSorBufferPacket[0], &BufferPacket[0], 10);

		// 슬레이브로부터 받은 습도와 온도 값을 AllSenSorBufferPacket의 [10]~[13]에 복사
		AllSenSorBufferPacket[10] = receivedPacket[2];  // 습도 High Byte
		AllSenSorBufferPacket[11] = receivedPacket[3];  // 습도 Low Byte
		AllSenSorBufferPacket[12] = receivedPacket[4];  // 온도 High Byte
		AllSenSorBufferPacket[13] = receivedPacket[5];  // 온도 Low Byte

		// BufferPacket의 [10]~[12]를 AllSenSorBufferPacket의 [14]~[16]으로 복사
		memcpy(&AllSenSorBufferPacket[14], &BufferPacket[10], 3);

		// UART1으로 AllSenSorBufferPacket 전송
		sendBuff_USART1((unsigned char*)AllSenSorBufferPacket, 17); // AllSenSorBufferPacket 크기 17바이트 전송

		packetReceived = 0;  // 패킷 처리 완료 후 플래그 초기화
	}
	else
	{
		//sendBuff_USART1(BufferPacket,sizeof(BufferPacket));
		// CRC가 불일치하면 패킷 무효화 처리
		memset((unsigned char*)receivedPacket, 0, SLAVE_PACKET_SIZE);  // 패킷 버퍼를 0으로 초기화
		packetReceived = 0;  // 플래그 초기화
		
	}
}

