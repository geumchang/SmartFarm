/*
 * DHT11_sensor.c
 *
 * Created: 2024-08-01 오후 4:48:41
 * Author : GeumChang
 */
#define F_CPU 14745600UL  // CPU 클럭 주파수 정의
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "servo.h"
#include "init.h"
#include "DS1302.h"

#define DHT11_PIN 7  
#define DHT11_DDR DDRD    
#define DHT11_PORT PORTD  
#define DHT11_PIN_IN PIND 

#define ID 0x02
#define FUNCTION 0x03
#define CHECK_THE_ADDRESS 0xF0

	////***VARIABLE***////
uint8_t I_RH, D_RH, I_Temp, D_Temp, CheckSum;
uint16_t light_value;

unsigned char BufferPacket[9] = {0,};
volatile uint8_t packetIndex = 0;
volatile uint8_t transmissionComplete = 0;

	////***FUNCTION***////
uint8_t Receive_data();
void Request();
void Response();
void controlLEDs(light_value);
void shift_variable();

unsigned short CRC16_modbus(unsigned char *addr, int num);
void makePacket(unsigned char id, unsigned char function, uint8_t humidity_H,uint8_t humidity_L, uint8_t temperature_H, uint8_t temperature_L);

void SPI_SlaveInit(void);
void startTransmission(void);


ISR(SPI_STC_vect)
{
	if (packetIndex < sizeof(BufferPacket))
	{
		SPDR = BufferPacket[packetIndex++];
	}
	
	if (packetIndex >= sizeof(BufferPacket))
	{
		transmissionComplete = 1;
		packetIndex = 0;
	}
}

int main(void)
{
	SPI_SlaveInit();
	USART_Init();  
	ADC_Init();
	initServoPWM();  
	Relay_init();
	//DS1302_Init();  // DS1302 초기화
	sei();  // 전역 인터럽트 활성화
	 
	 
/*
	Time current_time;
	Time initial_time = {0, 0, 12};  // {초, 분, 시}
	uint8_t previous_minute = 0xFF;
	DS1302_SetTime(initial_time);*/
	
	while(1)
	{
		Request();  // DHT11 센서에 시작 신호 전송
		Response();  // DHT11 센서의 응답 대기
		shift_variable();
		
		// LED 제어 함수 호출
		controlLEDs(light_value);
		// 서보모터 제어 함수 호출
		controlServo(I_Temp, I_RH);
		
/*
		startTransmission();
		
		// 전송 완료 대기
		while (!transmissionComplete);
		transmissionComplete = 0;*/

		_delay_ms(100);  // 다음 측정 전 대기
		
		//current_time = DS1302_ReadTime();  // DS1302에서 현재 시간을 읽음
		
/*
		if (current_time.minute != previous_minute)  // 1분 경과 확인
		{
			previous_minute = current_time.minute;  // 이전 분 값 업데이트
			
			
		}*/
		
		
		_delay_ms(1500);  // 2초 대기 후 다음 측정
	}
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

void makePacket(unsigned char id, unsigned char function, uint8_t humidity_H,uint8_t humidity_L, uint8_t temperature_H, uint8_t temperature_L)
{
	BufferPacket[0] = id;
	BufferPacket[1] = function;
	
	BufferPacket[2] = humidity_H;
	BufferPacket[3] = humidity_L;
	
	BufferPacket[4] = temperature_H;
	BufferPacket[5] = temperature_L;

	unsigned short CRC = CRC16_modbus(BufferPacket, 6);
	
	BufferPacket[6] = (CRC & 0xFF);
	BufferPacket[7] = (CRC >> 8);
	
	BufferPacket[8] = CHECK_THE_ADDRESS;
	
	_delay_ms(50);
}

// DHT11 센서에 시작 신호 보내기
void Request()
{
   DHT11_DDR |= (1<<DHT11_PIN);    // PORTD의 DHT11_PIN을 출력으로 설정
   DHT11_PORT &= ~(1<<DHT11_PIN);  // LOW 신호 보내기
   _delay_ms(20);                  // 최소 18ms 대기
   DHT11_PORT |= (1<<DHT11_PIN);   // HIGH 신호로 변경
}

void Response()
{
   DHT11_DDR &= ~(1<<DHT11_PIN);   // PORTD의 DHT11_PIN을 입력으로 설정
   while(DHT11_PIN_IN & (1<<DHT11_PIN));    // LOW 신호 대기
   while((DHT11_PIN_IN & (1<<DHT11_PIN))==0);  // HIGH 신호 대기
   while(DHT11_PIN_IN & (1<<DHT11_PIN));    // 다시 LOW 신호 대기
}

uint8_t Receive_data()
{
   uint8_t data = 0;
   for (int q=0; q<8; q++)
   {
      while((DHT11_PIN_IN & (1<<DHT11_PIN)) == 0);  // LOW 신호 대기
      _delay_us(30);
      if(DHT11_PIN_IN & (1<<DHT11_PIN))  // 30us 후에도 HIGH면 1, 아니면 0
      data = (data<<1)|(0x01);
      else
      data = (data<<1);
      while(DHT11_PIN_IN & (1<<DHT11_PIN));  // 다음 비트 대기
   }
   return data;
}

void shift_variable()
{
	I_RH = Receive_data();  // 습도 정수부 수신
	D_RH = Receive_data();  // 습도 소수부 수신
	
	I_Temp = Receive_data();  // 온도 정수부 수신
	D_Temp = Receive_data();  // 온도 소수부 수신
	
	CheckSum = Receive_data();  // 체크섬 수신
	
	light_value = ADC_Read(7);  // ADC7 (PF7) 핀에서 조도값 읽기
	light_value = 1023 - light_value;  // ADC 값 반전 (10비트 ADC 기준)
	
	// 체크섬 확인
	if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum)
	{
		USART_PrintString("Error\r\n");  // 오류 메시지 전송
	}
	else
	{
		makePacket(ID, FUNCTION, I_RH, D_RH, I_Temp, D_Temp);
		//아래 주석은 소수부 담긴 패킷. esp에서 0.1을 곱하는 후처리를 금창이가 해야 함.
		sendBuff_USART1(BufferPacket, sizeof(BufferPacket));
		
		
/*
		//소수부 까지 확인용
		float humidity = I_RH + D_RH * 0.1;
		float temperature = I_Temp + D_Temp * 0.1;
		 
		sprintf(data, "Humidity = %.1f %% Temperature = %.1f C Light value = %d\r\n", humidity, temperature, light_value);
		USART_PrintString(data);
		
		//습도가 잘 안떨어져서 아래 웅이코드와 검증
		sprintf(data, "I Humidity = %d %% Temperature = %d C Light value = %d\r\n", I_RH, I_Temp, light_value);
		USART_PrintString(data);
		*/
	}
}


void SPI_SlaveInit(void)
{
	DDRB = (1 << PB3);  // MISO를 출력으로 설정
	DDRB &= ~((1 << PB0) | (1 << PB2) | (1 << PB1));  // SS, MOSI, SCK를 입력으로 설정
	SPCR = (1 << SPIE) | (1 << SPE);  // SPI 인터럽트 활성화, SPI 활성화
}

void startTransmission(void)
{
	packetIndex = 0;
	transmissionComplete = 0;
	SPDR = BufferPacket[packetIndex++];  // 첫 바이트 로드
}
