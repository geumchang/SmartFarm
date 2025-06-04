/*
 * ControlPump.h
 *
 * Created: 2024-09-22 오전 12:51:41
 *  Author: jiung
 */ 


#ifndef CONTROLPUMP_H_
#define CONTROLPUMP_H_

void ControlsPUMP_antel(uint8_t enable)
{
	// PC6을 출력으로 설정
	DDRC |= (1 << PC4);

	if (enable)
	{
		PORTC |= (1 << PC4);  // 모터 켜기 (HIGH)
	}
	else
	{
		PORTC &= ~(1 << PC4);  // 모터 끄기 (LOW)
	}
}

void ControlsPUMP_water(uint8_t enable)
{
	// PC6을 출력으로 설정
	DDRC |= (1 << PC6);

	if (enable)
	{
		PORTC |= (1 << PC6);  // 모터 켜기 (HIGH)
	}
	else
	{
		PORTC &= ~(1 << PC6);  // 모터 끄기 (LOW)
	}
}

#endif /* CONTROLPUMP_H_ */
