#include <pebble.h>
#include "main.h"
#include "network.h"
#include "persist.h"
#include "weather_layer.h"
#include "eweather_layer.h"
#include "debug_layer.h"
#include "battery_layer.h"
#include "datetime_layer.h"
#include "config.h"

#define HOUR_FRAME      (GRect(0, 3, 64, 84))
#define MIN_FRAME       (GRect(80, 3, 144, 84))
#define DATE_FRAME      (GRect(1, 65, 144, 168))
#define WEATHER_FRAME   (GRect(0, 98, 144, 70))
#define DEBUG_FRAME     (GRect(0, 0, 144, 15))
#define BATTERY_FRAME   (GRect(66,22, 78, 40))

/* Keep a pointer to the current weather data as a global variable */
static WeatherData *weather_data;

/* Global variables to keep track of the UI elements */
static Window *window;

/* Need to wait for JS to be ready */
const  int  MAX_JS_READY_WAIT = 5000; // 5s
static bool initial_request = true;
static AppTimer *initial_jsready_timer;
static AppTimer *tap_timer;
static AppTimer *eweather_timer;


#define TAP_TIME 3000
#define EWEATHER_TIME 10*1000
static bool is_tapped_waiting;

static void timer_callback() {
  is_tapped_waiting = false;

  // DEBUG ONLY - VIBE TRIGGERS ANOTHER TAP
  // vibes_short_pulse();
}

// Tap Handlervoid accel_tap_handler(AccelAxisType axis, int32_t direction)
static void handle_tap(AccelAxisType axis,  int32_t direction) {
  if (1) {
    if (!is_tapped_waiting) {
      is_tapped_waiting = true;
      tap_timer = app_timer_register(TAP_TIME, timer_callback, NULL);
    }

    else {
      double_tap();

      app_timer_cancel(tap_timer);
      is_tapped_waiting = false;
    }
  }
}

void double_tap() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Showing extended weather");
	eweather_layer_hide(false);
	eweather_timer = app_timer_register(EWEATHER_TIME, dismiss_ewather, NULL);

}

void dismiss_ewather() {

	eweather_layer_hide(true);
}

// Add below to handle_init
void handle_init(void) {

}



static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
 
  if (units_changed & MINUTE_UNIT) {
    min_layer_update(units_changed);
    adjust_time_layer();
  }

  if (units_changed & HOUR_UNIT) {
    hour_layer_update(units_changed);
    min_layer_update(units_changed);
    adjust_time_layer();
  }

  if (units_changed & DAY_UNIT) {
    date_layer_update(tick_time);
  }

  /*
   * Useful for showing all icons using Yahoo, subscribe to SECOND_UNIT tick service


  if ((units_changed & SECOND_UNIT ) && (tick_time->tm_sec % 5 == 0)) {
  weather_data->temperature = (tick_time->tm_sec + rand()%60) * (rand()%3 ? 1 : -1);
  weather_data->condition = tick_time->tm_sec;
  weather_data->updated = time(NULL);
  weather_data->hourly_enabled = true;
  weather_data->hourly_updated = time(NULL);
  weather_data->h1_cond = tick_time->tm_sec % 30;
  weather_data->h2_cond = tick_time->tm_sec % 30;
  weather_data->h1_time = time(NULL) + (tick_time->tm_sec * (rand()%3600));
  weather_data->h2_time = time(NULL) + (tick_time->tm_sec * (rand()%3600));
  weather_data->h1_temp = (tick_time->tm_sec + rand()%60) * (rand()%3 ? 1 : -1);
  weather_data->h2_temp = (tick_time->tm_sec + rand()%60) * (rand()%3 ? 1 : -1);
  weather_layer_update(weather_data);
  }

*/


  // Refresh the weather info every 15 mins, targeting 18 mins after the hour (Yahoo updates around then)
  if ((units_changed & MINUTE_UNIT) && 
      (tick_time->tm_min % 15 == 3) &&
      !initial_request) {
    request_weather(weather_data);
  }
} 

/**
 * Wait for an official 'ready' from javascript or MAX_JS_READY_WAIT, whichever happens sooner 
 */
void initial_jsready_callback()
{
  initial_request = false;

  if (initial_jsready_timer) {
    app_timer_cancel(initial_jsready_timer);
  }

  request_weather(weather_data); 
}

static void init(void) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init started");

  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  weather_data = malloc(sizeof(WeatherData));
  init_network(weather_data);

  // Setup our layers
  min_layer_create(MIN_FRAME, window);
  hour_layer_create(HOUR_FRAME, window);
  date_layer_create(DATE_FRAME, window);
  weather_layer_create(WEATHER_FRAME, window);
  eweather_layer_create(WEATHER_FRAME, window);
  debug_layer_create(DEBUG_FRAME, window);
  battery_layer_create(BATTERY_FRAME, window);

  load_persisted_values(weather_data);

  // Kickoff our weather loading 'dot' animation
  weather_animate(weather_data);

  // Setup a timer incase we miss or don't receive js_ready to manually try ourselves
  initial_jsready_timer = app_timer_register(MAX_JS_READY_WAIT, initial_jsready_callback, NULL);

  // Update the screen right away
  time_t now = time(NULL);
  handle_tick(localtime(&now), MINUTE_UNIT | DAY_UNIT | HOUR_UNIT );

  // And then every minute
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  accel_tap_service_subscribe(&handle_tap);
}

static void deinit(void) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "deinit started");

  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();

  window_destroy(window);

  time_layer_destroy();
  date_layer_destroy();
  weather_layer_destroy();
  eweather_layer_destroy();
  debug_layer_destroy();
  battery_layer_destroy();

  free(weather_data);
  close_network();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
