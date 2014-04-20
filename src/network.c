#include <pebble.h>
#include "network.h"

const  int MAX_RETRY = 3;
static int retry_count = 0;

static void appmsg_in_received(DictionaryIterator *received, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In received.");

  WeatherData *weather = (WeatherData*) context;

  Tuple *temperature_tuple  = dict_find(received, KEY_TEMPERATURE);
  Tuple *condition_tuple    = dict_find(received, KEY_CONDITION);
  Tuple *sunrise_tuple      = dict_find(received, KEY_SUNRISE);
  Tuple *sunset_tuple       = dict_find(received, KEY_SUNSET);
  Tuple *pub_date_tuple     = dict_find(received, KEY_PUB_DATE);
  Tuple *neighborhood_tuple = dict_find(received, KEY_NEIGHBORHOOD);
  Tuple *error_tuple        = dict_find(received, KEY_ERROR);
  Tuple *service_tuple      = dict_find(received, KEY_SERVICE);
  Tuple *js_ready_tuple     = dict_find(received, KEY_JS_READY);
  Tuple *debug_tuple        = dict_find(received, KEY_DEBUG);
  Tuple *scale_tuple        = dict_find(received, KEY_SCALE);


  if (temperature_tuple && condition_tuple) {
    weather->temperature  = temperature_tuple->value->int32;
    weather->condition    = condition_tuple->value->int32;
    weather->sunrise      = sunrise_tuple->value->int32;
    weather->sunset       = sunset_tuple->value->int32;
    weather->pub_date     = pub_date_tuple->value->cstring;
    weather->error        = WEATHER_E_OK;
    weather->service      = service_tuple->value->cstring;
    weather->scale        = scale_tuple->value->cstring;
    weather->neighborhood = neighborhood_tuple->value->cstring;
    weather->debug        = (bool)debug_tuple->value->int32;
    weather->updated      = time(NULL);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got temperature %i and condition %i via %s for %s debug %i", 
      weather->temperature, weather->condition, weather->service, weather->neighborhood, weather->debug);
  }
  else if (js_ready_tuple) {
    weather->js_ready = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Javascript reports that it is ready");
  }
  else if (error_tuple) {
    weather->error = WEATHER_E_NETWORK;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got error %s", error_tuple->value->cstring);
  }
  else {
    weather->error = WEATHER_E_PHONE;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message with unknown keys... temperature=%p condition=%p error=%p",
      temperature_tuple, condition_tuple, error_tuple);
  }

  // Succes! reset the retry count...
  retry_count = 0;
}

static void appmsg_in_dropped(AppMessageResult reason, void *context) {
  WeatherData *weather_data = (WeatherData*) context;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i", reason);

  // Request a new update...
  retry_count++;
  request_weather(weather_data);
}

static void appmsg_out_sent(DictionaryIterator *sent, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Out sent.");
}

static void appmsg_out_failed(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  WeatherData *weather_data = (WeatherData*) context;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Out failed: %i", reason);

  retry_count++;
  
  switch (reason) {
    case APP_MSG_NOT_CONNECTED:
      weather_data->error = WEATHER_E_DISCONNECTED;
      request_weather(weather_data);
      break;
    case APP_MSG_SEND_REJECTED:
    case APP_MSG_SEND_TIMEOUT:
    default:
      weather_data->error = WEATHER_E_PHONE;
      request_weather(weather_data);
      break;
  }
}

void init_network(WeatherData *weather_data)
{
  app_message_register_inbox_received(appmsg_in_received);
  app_message_register_inbox_dropped(appmsg_in_dropped);
  app_message_register_outbox_sent(appmsg_out_sent);
  app_message_register_outbox_failed(appmsg_out_failed);
  app_message_set_context(weather_data);
  app_message_open(124, 256);

  weather_data->error = WEATHER_E_OK;
  weather_data->updated = 0;
  weather_data->js_ready = false;

  retry_count = 0;
}

void close_network()
{
  app_message_deregister_callbacks();
}

void request_weather(WeatherData *weather_data)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Request weather called, retry count: %i", retry_count);

  if (retry_count > MAX_RETRY) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Too many retries, let's wait for the next main loop request to try again.");
    retry_count = 0;
    return;
  }

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter");
    return;
  }

  dict_write_cstring(iter, KEY_SERVICE, weather_data->service);
  dict_write_cstring(iter, KEY_SCALE, weather_data->scale);
  dict_write_uint8(iter, KEY_DEBUG, (uint8_t)weather_data->debug);

  dict_write_end(iter);

  app_message_outbox_send();
}
