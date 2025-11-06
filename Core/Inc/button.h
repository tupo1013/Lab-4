/*
 * button.h
 *
 *  Created on: Nov 5, 2025
 *      Author: USER
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include <stdint.h>   // <-- bổ sung
#include "main.h"     // để có HAL & pin define nếu cần

extern uint16_t button_count[16];

void button_init();
void button_Scan();


#endif /* INC_BUTTON_H_ */
