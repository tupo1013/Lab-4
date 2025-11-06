/*
 * ds3231.h
 *
 *  Created on: Nov 5, 2025
 *      Author: USER
 */

#ifndef INC_DS3231_H_
#define INC_DS3231_H_

#include "utils.h"

#define ADDRESS_SEC			0x00
#define ADDRESS_MIN			0x01
#define ADDRESS_HOUR		0x02
#define ADDRESS_DAY			0x03
#define ADDRESS_DATE		0x04
#define ADDRESS_MONTH		0x05
#define ADDRESS_YEAR		0x06

extern uint8_t ds3231_hours;
extern uint8_t ds3231_min;
extern uint8_t ds3231_sec;
extern uint8_t ds3231_date;
extern uint8_t ds3231_day;
extern uint8_t ds3231_month;
extern uint8_t ds3231_year;

void ds3231_init();

void ds3231_Write(uint8_t address, uint8_t value);

void ds3231_ReadTime();

#endif /* INC_DS3231_H_ */
