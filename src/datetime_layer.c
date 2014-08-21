#include <pebble.h>
#include "network.h"
#include "datetime_layer.h"

static Layer *time_layer;
static TextLayer *min_layer;
static TextLayer *hour_layer;
static TextLayer *date_layer;

static char date_text[] = "XXX 00 XXX";

static char min_text[] = "00";
static char hour_text[] = "00";

/* Preload the fonts */
GFont font_date;
GFont font_time;

Layer *get_time_layer() {

	return time_layer;
}

void min_layer_create(GRect frame, Window *window) {
	time_layer = layer_create(GRect(0, 0, 144, 98));
	layer_add_child(window_get_root_layer(window), time_layer);

	font_time = fonts_load_custom_font(
			resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));
	min_layer = text_layer_create(frame);
	text_layer_set_text_color(min_layer, GColorWhite);
	text_layer_set_background_color(min_layer, GColorClear);
	text_layer_set_font(min_layer, font_time);
	text_layer_set_text_alignment(min_layer, GTextAlignmentLeft);

	layer_add_child(time_layer, text_layer_get_layer(min_layer));
}

void hour_layer_create(GRect frame, Window *window) {
	font_time = fonts_load_custom_font(
			resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));

	hour_layer = text_layer_create(frame);
	text_layer_set_text_color(hour_layer, GColorWhite);
	text_layer_set_background_color(hour_layer, GColorClear);
	text_layer_set_font(hour_layer, font_time);
	text_layer_set_text_alignment(hour_layer, GTextAlignmentRight);

	layer_add_child(time_layer, text_layer_get_layer(hour_layer));
}

void date_layer_create(GRect frame, Window *window) {
	font_date = fonts_load_custom_font(
			resource_get_handle(RESOURCE_ID_FUTURA_18));

	date_layer = text_layer_create(frame);
	text_layer_set_text_color(date_layer, GColorWhite);
	text_layer_set_background_color(date_layer, GColorClear);
	text_layer_set_font(date_layer, font_date);
	text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);

	layer_add_child(window_get_root_layer(window),
			text_layer_get_layer(date_layer));
}

void hour_layer_update() {
	time_t currentTime = time(NULL);
	// Update the time - Fix to deal with 12 / 24 centering bug
	struct tm *currentLocalTime = localtime(&currentTime);

	// Manually format the time as 12 / 24 hour, as specified
	strftime(hour_text, sizeof(hour_text), clock_is_24h_style() ? "%H" : "%I",
			currentLocalTime);

	strftime(min_text, sizeof(min_text), "%M", currentLocalTime);
	if (!clock_is_24h_style() && (hour_text[0] == '0'))
		memmove(hour_text, &hour_text[1], sizeof(hour_text) - 1);
	// Drop the first char of hour_text if needed

	text_layer_set_text(hour_layer, hour_text);
}

void adjust_time_layer() {



	GSize min_size = text_layer_get_content_size(min_layer);
	GSize hour_size = text_layer_get_content_size(hour_layer);

	int horizontal_adjust = 0;

	if (min_size.w > hour_size.w) {
		 horizontal_adjust = hour_size.w - min_size.w;
	} else {
		horizontal_adjust = min_size.w - hour_size.w;
	}
	layer_set_frame(time_layer,  GRect(horizontal_adjust/2 , 0, 144, 98));
}

void min_layer_update() {
	time_t currentTime = time(NULL);
	// Update the time - Fix to deal with 12 / 24 centering bug
	struct tm *currentLocalTime = localtime(&currentTime);

	strftime(min_text, sizeof(min_text), "%M", currentLocalTime);
	text_layer_set_text(min_layer, min_text);
}

void date_layer_update(struct tm *tick_time) {
	// Update the date - Without a leading 0 on the day of the month
	char day_text[4];
	char month_text[4];
	strftime(day_text, sizeof(day_text), "%a", tick_time);
	strftime(month_text, sizeof(month_text), "%b", tick_time);
	snprintf(date_text, sizeof(date_text), "%s %s %i", day_text, month_text,
			tick_time->tm_mday);
	// APP_LOG(APP_LOG_LEVEL_DEBUG, 'Date : %s',date_text);
	text_layer_set_text(date_layer, date_text);
}

void date_layer_destroy() {
	text_layer_destroy(date_layer);
	fonts_unload_custom_font(font_date);
}

void time_layer_destroy() {
	text_layer_destroy(min_layer);
	text_layer_destroy(hour_layer);
	layer_destroy(time_layer);

	fonts_unload_custom_font(font_time);
}

