#include <Arduino.h>
#include <driver/pcnt.h>

#include "RPM.h"

#define PIN_PWN 27


// So we get pulses; and want to gauge speed based on either the number of pulses
// in some time period or the time between pulses. And the speed by which we want
// to do is in the order of (parts of) seconds.
//
// Our time can be had in microSeconds. The pulses come in at rates of a few per second
// up to about 500 per second. We're mostly at the 10's to low 100/second.
//
// So counting is not that good a strategy; as we we have relaively granual counts
// per seconds (e.g. if the speed is 300 RPM; we get 50/second pulses; so a pulse
// just being early or late when we sample 4 times a second gives us a ~54% 'dance'.
//
// Measuring pulse length gives us more granualrity - microseconds on pulses of about
// 20 milliseconds is a fair number of 'extra' digits.
//
//
static unsigned long dt = 0;

#define PRM_AGG_LEN (2) // log(2) of exponential aggregation len 0=1(off), 1=2, 2=4, 3=8, etc..

static void IRAM_ATTR pwm_int_handle() {
  static unsigned long last = 0;
  static int cnt = 0;

  unsigned long now = micros();
  unsigned long diff = now - last;  
  last = now;
  
  dt =  ((dt <<PRM_AGG_LEN) - dt +  diff) >> PRM_AGG_LEN;
}

double rpm_loop() {
  double rpm = 60. * 1000. * 1000. / ((double)dt);
  return rpm;
}

void rpm_setup() {
  pinMode(PIN_PWN, INPUT);
  attachInterrupt(PIN_PWN, pwm_int_handle, FALLING /* or CHANGE */);
  Serial.println("RPM counter started");
}

#ifdef FAST_MOVING
#define PCNT_UNIT_PWM PCNT_UNIT_0

// ignore glitches shorter than this.
//
#define MIN_LEN 100 // in APB_CLK (80 Mhz) cycles

// Simple puls counting. And see how many ticks we get in some period
// of time. Downside here is that we do not know the exact time of
// the changes; so in effect we have a nyquest frequency in the order
// of our polling rare. So at 3000 RPM == 50 Hz and; twice a second update,
// we are easily +- 20%.
//
pcnt_config_t pcnt_config = {
  .pulse_gpio_num = PIN_PWN,       // set gpio for pulse input gpio
  .ctrl_gpio_num = -1,            // no gpio for control
  .lctrl_mode = PCNT_MODE_KEEP,   // when control signal is low, keep the primary counter mode
  .hctrl_mode = PCNT_MODE_KEEP,   // when control signal is high, keep the primary counter mode
  .pos_mode = PCNT_COUNT_INC,     // increment the counter on positive edge
  .neg_mode = PCNT_COUNT_INC,     //
  .counter_h_lim = -1,            // simply max. Expect it to wrap around.
  .counter_l_lim = -1,
  .unit = PCNT_UNIT_PWM,
  .channel = PCNT_CHANNEL_0
};

// We get an interrupt once in a while. Process this & ignore it.
//
static void IRAM_ATTR pwm_int_handle(void *arg) {
  uint32_t intr_status = PCNT.int_st.val;
  if (intr_status && (1 << PCNT_UNIT_PWM))
    PCNT.int_clr.val = BIT(PCNT_UNIT_PWM);
}

void rpm_setup() {
  if (ESP_OK != pcnt_unit_config(&pcnt_config) ||
      ESP_OK != pcnt_set_filter_value(PCNT_UNIT_PWM, MIN_LEN) ||
      ESP_OK != pcnt_filter_enable(PCNT_UNIT_PWM) ||
      //      ESP_OK != pcnt_isr_register(pwm_int_handle, NULL, 0, NULL) ||
      //      ESP_OK != pcnt_intr_enable(PCNT_UNIT_PWM) ||
      ESP_OK != pcnt_counter_clear(PCNT_UNIT_PWM) ||
      ESP_OK != pcnt_counter_resume(PCNT_UNIT_PWM)
     ) {
    Serial.println("PWM config failed**********");
    while (1) {};
  }
  pinMode(PIN_PWN, INPUT);
  Serial.println("RPM counter started");
}

#ifndef MAX
#define MAX(x,y) ((x)>(y) ? (x):(y))
#endif

double rpm_loop() {
  // sample every 500 milliseconds
  const uint32_t sample_time_ms = 500;
  // Exponential moving average window size/length in seconds.
  const uint32_t ROLLING_AVG_LEN_SECS = 1;
  const uint32_t ROLLING_AVG_LEN = MAX(1, ROLLING_AVG_LEN_SECS * 1000 / sample_time_ms);
  static uint16_t roll_cnt = 0;

  static double avg = 0.;

  // return 3000. + 75 * sin(millis() / 1000.);

  static unsigned long last = micros();
  unsigned long now = micros();
  unsigned long mu_diff = now - last;
  int16_t cnt = 0;
  pcnt_get_counter_value(PCNT_UNIT_PWM, &cnt);

  if (mu_diff < 1000 * sample_time_ms)
    return avg;
  last = now;

  static uint16_t last_cnt = 0;
  uint16_t cnt_diff = cnt - last_cnt;
  last_cnt = cnt;

  if (roll_cnt < ROLLING_AVG_LEN)
    roll_cnt++;

  double rpm =  60. * 1000 * 1000 * cnt_diff  / mu_diff / 2. /* for risze/fall edge */;
  avg += (rpm - avg) / roll_cnt; // rolling exponential average.

  return avg;
}
#endif
