#ifndef DATETIME_LAYER_H
#define DATETIME_LAYER_H

void date_layer_create(GRect frame, Window *window);
void min_layer_create(GRect frame, Window *window);
void hour_layer_create(GRect frame, Window *window);
Layer *get_time_layer();

void date_layer_update();
void hour_layer_update();
void min_layer_update();


void date_layer_destroy();
void time_layer_destroy();


#endif
