/*
 * servo.h
 *
 * Created: 2024-09-21 오후 2:46:43
 *  Author: ChangJun LEE
 */ 


#ifndef SERVO_H_
#define SERVO_H_

// 서보 PWM 초기화 함수
void initServoPWM()
{
	DDRE |= (1<<3);  // PE3를 출력으로 설정
	// 타이머/카운터 3 설정
	TCCR3B |= (1<<CS31) | (1<<CS30);  // 프리스케일러 64
	TCCR3A |= (1<<COM3A1);  // 비반전 모드
	TCCR3B |= (1<<WGM33) | (1<<WGM32);  // Fast PWM 모드
	TCCR3A |= (1<<WGM31);
	ICR3 = 4608 - 1;  // TOP 값 설정 (50Hz)
}

// 서보 모터 멈춤
void servoStop()
{
	TCCR3A &= ~((1<<COM3A1));
}

// 서보 모터 제어 (각도에 따라 PWM 신호 생성)
void servoRun(uint8_t degree)
{
	uint16_t degValue;
	TCCR3A |= (1<<COM3A1);
	degValue = (uint16_t)((degree/180.00) * 500 + 125);
	OCR3A = degValue;
}

// 서보모터 제어 함수
void controlServo(uint8_t temp, uint8_t humidity) {
	if (temp >= 30 || humidity >= 80) {
		servoRun(300);  // 팬 최대 속도
		} else if (temp >= 25 || humidity >= 70) {
		servoRun(240);  // 팬 중간 속도
		} else {
		servoStop();  // 팬 정지
	}
}

#endif /* SERVO_H_ */
