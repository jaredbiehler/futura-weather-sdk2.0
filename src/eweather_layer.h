typedef struct {
	TextLayer *main_layer_background;

	GBitmap *sunrise_icon;
	BitmapLayer *sunrise_icon_layer;
	TextLayer *sunrise_time_layer;

	GBitmap *sunset_icon;
	BitmapLayer *sunset_icon_layer;
	TextLayer *sunset_time_layer;

	GBitmap *wind_icon;
	BitmapLayer *wind_icon_layer;
	TextLayer *wind_speeddir_layer;

	GBitmap *humidity_icon;
	BitmapLayer *humidity_icon_layer;
	TextLayer *humidity_layer;

	GBitmap *temp_high_icon;
	BitmapLayer *temp_high_icon_layer;
	TextLayer *temp_high_layer;

	GBitmap *temp_low_icon;
	BitmapLayer *temp_low_icon_layer;
	TextLayer *temp_low_layer;
} EWeatherLayerData;

void eweather_layer_create(GRect frame, Window *window);
void eweather_layer_hide(bool hide);
void eweather_layer_update(WeatherData *weather_data);
void eweather_layer_destroy();


