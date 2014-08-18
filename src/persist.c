#include "network.h"
#include "debug_layer.h"
#include "battery_layer.h"
#include "config.h"
#include "persist.h"

/**
 * Must happen after layers are created! 
 */ 
void load_persisted_values(WeatherData *weather_data) 
{
  // Debug
  weather_data->debug = persist_exists(KEY_DEBUG_MODE) ? persist_read_bool(KEY_DEBUG_MODE) : DEFAULT_DEBUG_MODE; 

  if (weather_data->debug) {
    debug_enable_display();
    debug_update_message("Initializing...");
  } else {
    debug_disable_display();
  }

  // Battery
  weather_data->battery = persist_exists(KEY_DISPLAY_BATTERY) ? persist_read_bool(KEY_DISPLAY_BATTERY) : DEFAULT_DISPLAY_BATTERY; 

  if (weather_data->battery) {
    battery_enable_display();
  } else {
    battery_disable_display();
  }
  
  
  // Feels like
   weather_data->feels_like = persist_exists(KEY_FEELS_LIKE) ? persist_read_bool(KEY_FEELS_LIKE) : DEFAULT_USE_FEELSLIKE;
 
   // Hourly Offsets 1
   weather_data->h1_offset = persist_exists(KEY_H1_OFFSET) ? persist_read_int(KEY_H1_OFFSET) : DEFAULT_H1_OFFSET;
 
   // Hourly Offsets 2
   weather_data->h2_offset = persist_exists(KEY_H2_OFFSET) ? persist_read_int(KEY_H2_OFFSET) : DEFAULT_H2_OFFSET;
 
  
  
  // Weather Service
  if (persist_exists(KEY_WEATHER_SERVICE)) {
    persist_read_string(KEY_WEATHER_SERVICE, weather_data->service, sizeof(weather_data->service));
  } else {
    strcpy(weather_data->service, DEFAULT_WEATHER_SERVICE);
  }

  // Weather Scale
  if (persist_exists(KEY_WEATHER_SCALE)) {
    persist_read_string(KEY_WEATHER_SCALE, weather_data->scale, sizeof(weather_data->scale));
  } else {
    strcpy(weather_data->scale, DEFAULT_WEATHER_SCALE);
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "PersistLoad:  d:%d b:%d s:%s u:%s f:%d h1:2-%d:%d", 
      weather_data->debug, weather_data->battery, weather_data->service, weather_data->scale, weather_data->feels_like,
         (int)weather_data->h1_offset, (int) weather_data->h2_offset);
}

void store_persisted_values(WeatherData *weather_data) 
{
  persist_write_bool(KEY_DEBUG_MODE, weather_data->debug);
  persist_write_bool(KEY_DISPLAY_BATTERY, weather_data->battery);
  persist_write_bool(KEY_USE_FEELSLIKE, weather_data->feels_like);
  persist_write_string(KEY_WEATHER_SERVICE, weather_data->service);
  persist_write_string(KEY_WEATHER_SCALE, weather_data->scale);
  persist_write_int(KEY_H1_OFFSET, weather_data->h1_offset);
  persist_write_int(KEY_H2_OFFSET, weather_data->h2_offset);


  APP_LOG(APP_LOG_LEVEL_DEBUG, "PersistStore:  d:%d b:%d s:%s u:%s f:%d h1:2-%d:%d", 
      weather_data->debug, weather_data->battery, weather_data->service, weather_data->scale, weather_data->feels_like,
         (int) weather_data->h1_offset, (int)weather_data->h2_offset);
}