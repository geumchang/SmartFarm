/*
 * init.h
 *
 * Created: 2024-09-21 오후 2:51:17
 *  Author: ChangJun LEE
 */ 

#ifndef INIT_H_
#define INIT_H_

// 릴레이 핀 정의를 PE4, PE5, PE6, PE7로 변경
#define RELAY_PIN1 PE4  // 릴레이 1 제어 핀
#define RELAY_PIN2 PE5  // 릴레이 2 제어 핀
#define RELAY_PIN3 PE6  // 릴레이 3 제어 핀
#define RELAY_PIN4 PE7  // 릴레이 4 제어 핀

//조도센서 헤더
void ADC_Init()
{
	ADMUX = (1<<REFS0);  // AVCC를 기준 전압으로 사용
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);  // ADC 활성화 및 분주비 128
}

uint16_t ADC_Read(uint8_t channel)
{
	ADMUX = (ADMUX & 0xF8) | (channel & 0x07);  // 채널 선택
	ADCSRA |= (1<<ADSC);  // 변환 시작
	while (ADCSRA & (1<<ADSC));  // 변환 완료 대기
	return ADC;
}

void Relay_init()
{
	// 4개의 릴레이 핀을 출력 모드로 설정 (DDRE 사용)
	DDRE |= (1 << RELAY_PIN1) | (1 << RELAY_PIN2) | (1 << RELAY_PIN3) | (1 << RELAY_PIN4);
	
}

// LED 제어 함수
void controlLEDs(light_value) {
	if (light_value > 700) {
		// 조도값이 700 이상이면 릴레이 1개 켜짐
		PORTE |= (1 << RELAY_PIN2) | (1 << RELAY_PIN3) | (1 << RELAY_PIN4);
		PORTE &= ~(1 << RELAY_PIN1);
	}
	else if (light_value > 500) {
		// 조도값이 500-700이면 릴레이 2개 켜짐
		PORTE |= (1 << RELAY_PIN3) | (1 << RELAY_PIN4);
		PORTE &= ~((1 << RELAY_PIN1) | (1 << RELAY_PIN2));
	}
	else if (light_value > 300) {
		// 조도값이 300-500이면 릴레이 3개 켜짐
		PORTE |= (1 << RELAY_PIN4);
		PORTE &= ~((1 << RELAY_PIN1) | (1 << RELAY_PIN2) | (1 << RELAY_PIN3));
	}
	else {
		// 조도값이 300 이하이면 릴레이 4개 켜짐
		PORTE &= ~((1 << RELAY_PIN1) | (1 << RELAY_PIN2) | (1 << RELAY_PIN3) | (1 << RELAY_PIN4));
	}
}

#endif /* INIT_H_ */
