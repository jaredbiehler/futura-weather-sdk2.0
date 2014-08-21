#include <pebble.h>
#include "battery_layer.h"
#include "datetime_layer.h"

const uint32_t BATTERY_TIMEOUT = 2000; // 2 second animation 
const uint8_t  MAX_DOTS = 5;

static Layer *battery_layer;

static AppTimer *battery_animation_timer;
static bool is_animating = false;
static bool is_enabled   = false;
static int8_t dots = 4;

static void handle_battery(BatteryChargeState charge_state) 
{
  uint8_t charge = charge_state.charge_percent;
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
    

    if (charge >= 85) {
      dots = MAX_DOTS;
    } else if (charge >= 75 && charge <85) {
      dots = 4;
    } else if (charge >= 50 && charge < 75) {
      dots = 3;
    } else if (charge >= 20 && charge < 50) {
      dots = 2;
   } else {
      dots = 1;
    }
  }

  layer_mark_dirty(battery_layer);
}


void battery_layer_create(GRect frame, Window *window)
{
  battery_layer = layer_create(frame);
  layer_set_update_proc(battery_layer, battery_layer_update);
  layer_add_child(get_time_layer(), battery_layer);
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
    int8_t spacer = 18; // pixels
	int8_t start_y = 45; //only two colon like dots

	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_context_set_stroke_color(ctx, GColorWhite);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "drawing battery %d", dots);
	switch (dots) {

	case 1:
		graphics_draw_circle(ctx, GPoint(6, start_y - (1 * spacer)), 5);
		graphics_draw_circle(ctx, GPoint(6, start_y - (1 * spacer)), 4);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 5);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 4);
		break;
	case 2:
		graphics_draw_circle(ctx, GPoint(6, start_y - (1 * spacer)), 5);
		graphics_draw_circle(ctx, GPoint(6, start_y - (1 * spacer)), 4);
		graphics_fill_circle(ctx, GPoint(6, start_y - (1 * spacer)), 2);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 5);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 4);
    break;
	case 3:
		graphics_fill_circle(ctx, GPoint(6, start_y - (1 * spacer)), 5);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 5);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 4);
	break;
	case 4:
		graphics_fill_circle(ctx, GPoint(6, start_y - (1 * spacer)), 5);
		graphics_fill_circle(ctx, GPoint(6, start_y - (2 * spacer)), 2);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 5);
		graphics_draw_circle(ctx, GPoint(6, start_y - (2 * spacer)), 4);
		break;
	case 5:
		graphics_fill_circle(ctx, GPoint(6, start_y - (1 * spacer)), 5);
		graphics_fill_circle(ctx, GPoint(6, start_y - (2 * spacer)), 5);
		break;


	}

}

void battery_layer_destroy() 
{
  battery_disable_display();
  layer_destroy(battery_layer);
}



