#include <pebble.h>
#include "battery_layer.h"

const uint32_t BATTERY_TIMEOUT = 2000; // 2 second animation 
const uint8_t  MAX_DOTS = 10;

static Layer *battery_layer;

static AppTimer *battery_animation_timer;
static bool is_animating = false;
static bool is_enabled   = false;
static int8_t dots = 10; 

static void handle_battery(BatteryChargeState charge_state) 
{
  if (charge_state.is_charging || charge_state.is_plugged) {

    if (!is_animating) {
       is_animating = true;
       battery_animation_timer = app_timer_register(BATTERY_TIMEOUT, battery_timer_callback, NULL);
    }
    return;

  } 
  else {

    is_animating = false;
    if (battery_animation_timer) {
      app_timer_cancel(battery_animation_timer);
    }
    
    uint8_t charge = charge_state.charge_percent;
    if (charge >= 95) {
      dots = MAX_DOTS;
    } else if (charge >= 90 && charge < 95) {
      dots = 9;
    } else if (charge >= 80 && charge < 90) {
      dots = 8;
    } else if (charge >= 70 && charge < 80) {
      dots = 7;
    } else if (charge >= 60 && charge < 70) {
      dots = 6;
    } else if (charge >= 50 && charge < 60) {
      dots = 5;
    } else if (charge >= 40 && charge < 50) {
      dots = 4;
    } else if (charge >= 30 && charge < 40) {
      dots = 3;
    } else if (charge >= 20 && charge < 30) {
      dots = 2;
    } else if (charge >= 10 && charge < 20) {
      dots = 1;
    }else {
      dots = 0;
    }
  }

  layer_mark_dirty(battery_layer);
}


void battery_layer_create(GRect frame, Window *window)
{
  battery_layer = layer_create(frame);
  layer_set_update_proc(battery_layer, battery_layer_update);
  layer_add_child(window_get_root_layer(window), battery_layer);
}

void battery_enable_display() 
{
  if (is_enabled) {
    return;
  }

  is_animating = false;
  is_enabled = true;

  // Kickoff first update
  handle_battery(battery_state_service_peek());

  // Subscribe to the battery monitoring service
  battery_state_service_subscribe(&handle_battery);

  layer_set_hidden(battery_layer, false);
}

void battery_disable_display() 
{
  is_animating = false;
  is_enabled = false;

  layer_set_hidden(battery_layer, true);

  // Unsubscribe to the battery monitoring service
  battery_state_service_unsubscribe();

  // Kill the timer
  if (battery_animation_timer) {
    app_timer_cancel(battery_animation_timer);
  }
}

void battery_timer_callback()
{
  dots++;
  if (dots > MAX_DOTS) {
    dots = 1;
  }
  layer_mark_dirty(battery_layer);
  battery_animation_timer = app_timer_register(BATTERY_TIMEOUT, battery_timer_callback, NULL);
}

void battery_layer_update(Layer *me, GContext *ctx) 
{
  int8_t spacer  = 7; // pixels
  int8_t start_x = spacer * MAX_DOTS;
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (int i=0; i<MAX_DOTS; i++) {
    if (i<dots) {
      graphics_fill_circle(ctx, GPoint(start_x-(i*spacer), 4), 2);
    } else {
      graphics_draw_circle(ctx, GPoint(start_x-(i*spacer), 4), 2);
    }
  } 
}

void battery_layer_destroy() 
{
  battery_disable_display();
  layer_destroy(battery_layer);
}



