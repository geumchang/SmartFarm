/*
 * DS1302.h
 *
 * Created: 2024-09-21 오후 11:53:34
 *  Author: jiung
 */ 


#ifndef DS1302_H_
#define DS1302_H_

// DS1302 핀 정의를 PINA로 변경
#define DS1302_CLK PA0
#define DS1302_DAT PA1
#define DS1302_RST PA2

#define DS1302_CLK_DDR DDRA
#define DS1302_DAT_DDR DDRA
#define DS1302_RST_DDR DDRA

#define DS1302_CLK_PORT PORTA
#define DS1302_DAT_PORT PORTA
#define DS1302_RST_PORT PORTA

#define DS1302_DAT_PIN PINA

// DS1302 레지스터 주소
#define DS1302_SEC_REG     0x80
#define DS1302_MIN_REG     0x82
#define DS1302_HOUR_REG    0x84
#define DS1302_CONTROL_REG 0x8E
#define DS1302_CHARGER_REG 0x90

typedef struct {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
} Time;

// DS1302 초기화
void DS1302_Init() {
	DS1302_RST_DDR |= (1 << DS1302_RST);
	DS1302_CLK_DDR |= (1 << DS1302_CLK);
	DS1302_DAT_DDR |= (1 << DS1302_DAT);
	
	DS1302_RST_PORT &= ~(1 << DS1302_RST);
	DS1302_CLK_PORT &= ~(1 << DS1302_CLK);
	
	// Write Protect 해제
	DS1302_WriteByte(DS1302_CONTROL_REG, 0x00);
	
	// 충전 기능 비활성화
	DS1302_WriteByte(DS1302_CHARGER_REG, 0x00);
}

// DS1302에 1바이트 쓰기
void DS1302_WriteByte(uint8_t addr, uint8_t data) {
	DS1302_RST_PORT |= (1 << DS1302_RST);
	
	// 주소 전송
	for (int i = 0; i < 8; i++) {
		DS1302_DAT_PORT = (addr & (1 << i)) ? (DS1302_DAT_PORT | (1 << DS1302_DAT)) : (DS1302_DAT_PORT & ~(1 << DS1302_DAT));
		DS1302_CLK_PORT |= (1 << DS1302_CLK);
		_delay_us(1);
		DS1302_CLK_PORT &= ~(1 << DS1302_CLK);
	}
	
	// 데이터 전송
	for (int i = 0; i < 8; i++) {
		DS1302_DAT_PORT = (data & (1 << i)) ? (DS1302_DAT_PORT | (1 << DS1302_DAT)) : (DS1302_DAT_PORT & ~(1 << DS1302_DAT));
		DS1302_CLK_PORT |= (1 << DS1302_CLK);
		_delay_us(1);
		DS1302_CLK_PORT &= ~(1 << DS1302_CLK);
	}
	
	DS1302_RST_PORT &= ~(1 << DS1302_RST);
}

// DS1302에서 1바이트 읽기
uint8_t DS1302_ReadByte(uint8_t addr) {
	uint8_t data = 0;
	
	DS1302_RST_PORT |= (1 << DS1302_RST);
	
	// 주소 전송 (읽기 위해 최하위 비트를 1로 설정)
	addr |= 0x01;
	for (int i = 0; i < 8; i++) {
		DS1302_DAT_PORT = (addr & (1 << i)) ? (DS1302_DAT_PORT | (1 << DS1302_DAT)) : (DS1302_DAT_PORT & ~(1 << DS1302_DAT));
		DS1302_CLK_PORT |= (1 << DS1302_CLK);
		_delay_us(1);
		DS1302_CLK_PORT &= ~(1 << DS1302_CLK);
	}
	
	// 데이터 핀을 입력으로 전환
	DS1302_DAT_DDR &= ~(1 << DS1302_DAT);
	
	// 데이터 수신
	for (int i = 0; i < 8; i++) {
		DS1302_CLK_PORT |= (1 << DS1302_CLK);
		_delay_us(1);
		if (DS1302_DAT_PIN & (1 << DS1302_DAT)) {
			data |= (1 << i);
		}
		DS1302_CLK_PORT &= ~(1 << DS1302_CLK);
	}
	
	DS1302_RST_PORT &= ~(1 << DS1302_RST);
	
	// 데이터 핀을 다시 출력으로 전환
	DS1302_DAT_DDR |= (1 << DS1302_DAT);
	
	return data;
}

// BCD를 Decimal로 변환
uint8_t BCD_to_DEC(uint8_t bcd) {
	return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Decimal을 BCD로 변환
uint8_t DEC_to_BCD(uint8_t dec) {
	return ((dec / 10) << 4) | (dec % 10);
}

// 전체 시간 읽기
Time DS1302_ReadTime() {
	Time t;
	t.second = BCD_to_DEC(DS1302_ReadByte(DS1302_SEC_REG) & 0x7F);
	t.minute = BCD_to_DEC(DS1302_ReadByte(DS1302_MIN_REG));
	t.hour = BCD_to_DEC(DS1302_ReadByte(DS1302_HOUR_REG) & 0x3F);
	return t;
}

// 시간 설정
void DS1302_SetTime(Time t) {
	DS1302_WriteByte(DS1302_CONTROL_REG, 0x00);  // Write Protect 해제
	DS1302_WriteByte(DS1302_SEC_REG, DEC_to_BCD(t.second));
	DS1302_WriteByte(DS1302_MIN_REG, DEC_to_BCD(t.minute));
	DS1302_WriteByte(DS1302_HOUR_REG, DEC_to_BCD(t.hour));
}

#endif /* DS1302_H_ */
