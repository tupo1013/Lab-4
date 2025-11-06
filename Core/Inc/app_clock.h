#ifndef APP_CLOCK_H
#define APP_CLOCK_H

#include <stdint.h>   // cần cho uint8_t/uint16_t
#include <stdbool.h>  // cần cho bool

typedef enum {
  MODE_VIEW = 0,     // Xem giờ
  MODE_SET_TIME,     // Chỉnh giờ/ngày
  MODE_ALARM         // Hẹn giờ
} app_mode_t;

typedef enum {
  FIELD_SEC = 0,
  FIELD_MIN,
  FIELD_HOUR,
  FIELD_DAY,   // 1..7
  FIELD_DATE,  // 1..31 (đơn giản)
  FIELD_MONTH, // 1..12
  FIELD_YEAR,  // 0..99 (2000..2099)
  FIELD_COUNT
} field_t;

void app_clock_init(void);
/** Gọi mỗi tick (ví dụ 50 ms) sau khi đã button_Scan() */
void app_clock_on_tick(void);

#endif // APP_CLOCK_H
