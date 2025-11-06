#include <stdint.h>
#include <stdbool.h>
#include "app_clock.h"
#include "ds3231.h"
#include "lcd.h"
#include "button.h"
#include "software_timer.h"
#include "utils.h"

/* ============ Cấu hình nút ============ */
/* Sửa 3 index này theo thứ tự button thực tế trên mạch bạn */
#ifndef BTN_MODE_IDX
#define BTN_MODE_IDX   0
#endif
#ifndef BTN_UP_IDX
#define BTN_UP_IDX     1
#endif
#ifndef BTN_OK_IDX
#define BTN_OK_IDX     2
#endif

/* Chu kỳ tick (ms) — khớp setTimer2(...) của bạn, mặc định 50 ms */
#define APP_TICK_MS            50

/* Tham số hành vi theo đề bài */
#define BLINK_PERIOD_MS        500   // 2 Hz
#define HOLD_THRESHOLD_MS      2000  // giữ 2 s
#define REPEAT_STEP_MS         200   // lặp 200 ms

/* ============ button.c dữ liệu ============ */
extern uint16_t button_count[16];

static inline bool btn_pressed_now(int idx)   { return button_count[idx] > 0; }
static inline bool btn_pressed_edge(int idx)  { return button_count[idx] == 1; }

/* ============ Kiểu dữ liệu ============ */
typedef struct {
  uint8_t sec, min, hour;
  uint8_t day, date, month, year;
} datetime_t;

typedef struct {
  uint8_t hour, min, sec;
  bool enabled;
} alarm_t;

/* ============ Trạng thái app ============ */
static app_mode_t mode = MODE_VIEW;
static field_t    editing_field = FIELD_HOUR;

static datetime_t cur, edit;
static alarm_t    alarm1 = { .hour=7, .min=0, .sec=0, .enabled=false };

static uint16_t blink_acc_ms = 0;
static bool     blink_on = true;

static uint16_t up_hold_ms = 0;
static uint16_t up_repeat_acc_ms = 0;
static bool     up_was_pressed_last = false;

static bool     alarm_active = false;
static uint16_t alarm_remain_ms = 0;

/* ============ Helper ============ */
static inline void wrap_inc(uint8_t *v, uint8_t min, uint8_t max){
  uint8_t nv = (uint8_t)(*v + 1);
  if(nv > max) nv = min;
  *v = nv;
}

/* ============ DS3231 glue ============ */
extern uint8_t ds3231_hours;
extern uint8_t ds3231_min;
extern uint8_t ds3231_sec;
extern uint8_t ds3231_date;
extern uint8_t ds3231_day;
extern uint8_t ds3231_month;
extern uint8_t ds3231_year;

static void read_ds3231_into_cur(void){
  ds3231_ReadTime();
  cur.hour  = ds3231_hours;
  cur.min   = ds3231_min;
  cur.sec   = ds3231_sec;
  cur.day   = ds3231_day;
  cur.date  = ds3231_date;
  cur.month = ds3231_month;
  cur.year  = ds3231_year;
}

static void snapshot_from_cur(void){ edit = cur; }

static void commit_edit_to_ds3231(void){
  ds3231_Write(ADDRESS_YEAR , edit.year);
  ds3231_Write(ADDRESS_MONTH, edit.month);
  ds3231_Write(ADDRESS_DATE , edit.date);
  ds3231_Write(ADDRESS_DAY  , edit.day);
  ds3231_Write(ADDRESS_HOUR , edit.hour);
  ds3231_Write(ADDRESS_MIN  , edit.min);
  ds3231_Write(ADDRESS_SEC  , edit.sec);
}

/* ============ Tăng trường ============ */
static void increment_field(field_t f){
  switch(f){
    case FIELD_SEC:   wrap_inc(&edit.sec  , 0, 59); break;
    case FIELD_MIN:   wrap_inc(&edit.min  , 0, 59); break;
    case FIELD_HOUR:  wrap_inc(&edit.hour , 0, 23); break;
    case FIELD_DAY:   wrap_inc(&edit.day  , 1, 7 ); break;
    case FIELD_DATE:  wrap_inc(&edit.date , 1, 31); break; // đơn giản
    case FIELD_MONTH: wrap_inc(&edit.month, 1, 12); break;
    case FIELD_YEAR:  wrap_inc(&edit.year , 0, 99); break;
    default: break;
  }
}

/* ============ Vẽ LCD (dùng lcd_ShowStr của bạn) ============ */
static void draw2(int x, int y, uint8_t val, bool active){
  if(active && !blink_on){
    lcd_ShowStr(x, y, (uint8_t*)"  ", BLACK, BLACK, 24, 0);
  } else {
    lcd_ShowIntNum(x, y, val, 2, GREEN, BLACK, 24);
  }
}

static void draw_status_bar(void){
  lcd_Fill(0,0,240,20,BLACK);
  if(mode == MODE_VIEW){
    lcd_ShowStr(4,2,(uint8_t*)"MODE: VIEW", WHITE, BLACK, 16, 0);
  } else if(mode == MODE_SET_TIME){
    lcd_ShowStr(4,2,(uint8_t*)"MODE: SET", WHITE, BLACK, 16, 0);
  } else {
    lcd_ShowStr(4,2,(uint8_t*)"MODE: ALARM", WHITE, BLACK, 16, 0);
    lcd_ShowStr(140,2,(uint8_t*)(alarm1.enabled?"ON":"OFF"),
                alarm1.enabled?GREEN:RED, BLACK, 16, 0);
  }
}

static void draw_time_area(const datetime_t *dt){
  draw2(70 ,100, dt->hour,  (mode==MODE_SET_TIME && editing_field==FIELD_HOUR) ||
                           (mode==MODE_ALARM   && editing_field==FIELD_HOUR));
  lcd_ShowStr(100,100,(uint8_t*)":", GREEN, BLACK, 24, 0);
  draw2(110,100, dt->min,   (mode==MODE_SET_TIME && editing_field==FIELD_MIN) ||
                           (mode==MODE_ALARM   && editing_field==FIELD_MIN));
  lcd_ShowStr(140,100,(uint8_t*)":", GREEN, BLACK, 24, 0);
  draw2(150,100, dt->sec,   (mode==MODE_SET_TIME && editing_field==FIELD_SEC) ||
                           (mode==MODE_ALARM   && editing_field==FIELD_SEC));
}

static void draw_date_area(const datetime_t *dt){
  lcd_ShowStr(20,130,(uint8_t*)"D:",  YELLOW, BLACK, 24, 0);
  draw2(50,130, dt->day,   (mode==MODE_SET_TIME && editing_field==FIELD_DAY));

  lcd_ShowStr(70,130,(uint8_t*)"Dt:", YELLOW, BLACK, 24, 0);
  draw2(100,130, dt->date, (mode==MODE_SET_TIME && editing_field==FIELD_DATE));

  lcd_ShowStr(120,130,(uint8_t*)"Mo:", YELLOW, BLACK, 24, 0);
  draw2(150,130, dt->month,(mode==MODE_SET_TIME && editing_field==FIELD_MONTH));

  lcd_ShowStr(180,130,(uint8_t*)"Yr:", YELLOW, BLACK, 24, 0);
  draw2(210,130, dt->year, (mode==MODE_SET_TIME && editing_field==FIELD_YEAR));
}

static void draw_alarm_effect(void){
  if(!alarm_active) return;
  static bool inv = false;
  inv = !inv;
  lcd_Fill(0,180,240,220, inv ? YELLOW : BLACK);
  lcd_ShowStr(70,190,(uint8_t*)"ALARM!", inv?BLACK:YELLOW, inv?YELLOW:BLACK, 24, 0);
}

/* ============ Alarm ============ */
static void maybe_trigger_alarm(void){
  if(!alarm1.enabled) return;
  if(cur.hour==alarm1.hour && cur.min==alarm1.min && cur.sec==alarm1.sec){
    alarm_active = true;
    alarm_remain_ms = 3000; // nháy 3s
  }
}

/* ============ INIT & TICK ============ */
void app_clock_init(void){
  ds3231_init();
  read_ds3231_into_cur();
  snapshot_from_cur();

  blink_acc_ms = 0; blink_on = true;
  up_hold_ms = 0; up_repeat_acc_ms = 0; up_was_pressed_last = false;

  alarm_active = false; alarm_remain_ms = 0;

  lcd_Clear(BLACK);
}

void app_clock_on_tick(void){
  /* 1) Blink & hold/repeat */
  blink_acc_ms += APP_TICK_MS;
  if(blink_acc_ms >= BLINK_PERIOD_MS){
    blink_acc_ms = 0;
    blink_on = !blink_on;
  }

  bool up_now = btn_pressed_now(BTN_UP_IDX);
  if(up_now){
    if(!up_was_pressed_last){
      up_was_pressed_last = true;
      up_hold_ms = 0;
      up_repeat_acc_ms = 0;
    } else {
      if(up_hold_ms < 60000) up_hold_ms += APP_TICK_MS;
      if(up_hold_ms >= HOLD_THRESHOLD_MS){
        up_repeat_acc_ms += APP_TICK_MS;
        if(up_repeat_acc_ms >= REPEAT_STEP_MS){
          up_repeat_acc_ms = 0;
          if(mode == MODE_SET_TIME){
            increment_field(editing_field);
          } else if(mode == MODE_ALARM){
            if(editing_field == FIELD_HOUR) wrap_inc(&alarm1.hour,0,23);
            else if(editing_field == FIELD_MIN) wrap_inc(&alarm1.min,0,59);
            else if(editing_field == FIELD_SEC) wrap_inc(&alarm1.sec,0,59);
          }
        }
      }
    }
  } else {
    up_was_pressed_last = false;
    up_hold_ms = 0;
    up_repeat_acc_ms = 0;
  }

  /* 2) Cập nhật DS3231 nếu không ở SET (SET thì đóng băng thời gian) */
  if(mode != MODE_SET_TIME){
    read_ds3231_into_cur();
  }

  /* 3) Events nút */
  bool ev_mode = btn_pressed_edge(BTN_MODE_IDX);
  bool ev_up   = btn_pressed_edge(BTN_UP_IDX);
  bool ev_ok   = btn_pressed_edge(BTN_OK_IDX);

  if(ev_mode){
    if(mode == MODE_VIEW){
      mode = MODE_SET_TIME;
      editing_field = FIELD_HOUR;
      snapshot_from_cur();
    } else if(mode == MODE_SET_TIME){
      commit_edit_to_ds3231();
      mode = MODE_ALARM;
      editing_field = FIELD_HOUR;
    } else {
      mode = MODE_VIEW;
    }
  }

  switch(mode){
    case MODE_VIEW:
      break;

    case MODE_SET_TIME:
      if(ev_up){
        increment_field(editing_field);
      }
      if(ev_ok){
        editing_field = (field_t)((editing_field + 1) % FIELD_COUNT);
        if(editing_field == FIELD_SEC){
          commit_edit_to_ds3231();
          read_ds3231_into_cur();
          snapshot_from_cur();
        }
      }
      cur = edit; // hiển thị theo bản edit
      break;

    case MODE_ALARM:
      if(ev_up){
        if(editing_field == FIELD_HOUR) wrap_inc(&alarm1.hour,0,23);
        else if(editing_field == FIELD_MIN) wrap_inc(&alarm1.min,0,59);
        else if(editing_field == FIELD_SEC) wrap_inc(&alarm1.sec,0,59);
      }
      if(ev_ok){
        if(editing_field == FIELD_SEC){
          alarm1.enabled = !alarm1.enabled;
          editing_field = FIELD_HOUR;
        } else {
          editing_field = (field_t)(editing_field + 1);
        }
      }
      break;
  }

  /* 4) Alarm effect */
  maybe_trigger_alarm();
  if(alarm_active){
    if(alarm_remain_ms > APP_TICK_MS) alarm_remain_ms -= APP_TICK_MS;
    else { alarm_remain_ms = 0; alarm_active = false; }
  }

  /* 5) Vẽ UI */
  draw_status_bar();
  draw_time_area(&cur);
  draw_date_area(&cur);
  draw_alarm_effect();
}
