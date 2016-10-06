#include <pebble.h>
#include <math.h>
#include <stdio.h>
#include "enamel.h"
#include <pebble-events/pebble-events.h>

static Layer *s_canvas_layer; 
static GBitmap *s_bitmap_earth;
static GBitmap *s_bitmap_stars;
static GBitmap *s_bitmap_moon;
static Window *s_main_window;
static TextLayer *s_time_layer_earth;
static TextLayer *s_time_layer_moon;
static TextLayer *s_time_layer_date_top;
static TextLayer *s_time_layer_date_bottom;
static TextLayer *s_time_layer_date_right;
static BitmapLayer *s_bitmap_layer_earth;
static BitmapLayer *s_bitmap_layer_stars;
static EventHandle s_text_layer_event_handle_hcolor;
static EventHandle s_text_layer_event_handle_mcolor;
static EventHandle s_text_layer_event_handle_dcolor;

GColor hcolor;
GColor mcolor;
GColor dcolor;

static void update_time();

static void enamel_settings_received_text_layer_handler_hcolor(void *context){
  TextLayer *text_layer = (TextLayer *) context;
  text_layer_set_text_color(text_layer, enamel_get_HourColor());
  hcolor=enamel_get_HourColor();
}
static void enamel_settings_received_text_layer_handler_mcolor(void *context){
  mcolor=enamel_get_MinuteColor();
  layer_mark_dirty(s_canvas_layer);
}
static void enamel_settings_received_text_layer_handler_dcolor(void *context){
  TextLayer *text_layer = (TextLayer *) context;
  text_layer_set_text_color(text_layer, enamel_get_DateColor());
  dcolor=enamel_get_DateColor();
  text_layer_set_text_color(s_time_layer_date_top, dcolor);
  text_layer_set_text_color(s_time_layer_date_bottom, dcolor);
   #ifdef PBL_PLATFORM_BASALT
  text_layer_set_text_color(s_time_layer_date_right, dcolor);
  #endif
}




static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer_h[8];
  static char s_buffer_date_top[8];
  static char s_buffer_date_bottom[8];
  static char s_buffer_date_right[8];
  strftime(s_buffer_h, sizeof(s_buffer_h), clock_is_24h_style() ?
                                          "%H" : "%I", tick_time);
  #ifdef PBL_PLATFORM_BASALT
  strftime(s_buffer_date_top, sizeof(s_buffer_date_top), "%a", tick_time);
  strftime(s_buffer_date_bottom, sizeof(s_buffer_date_bottom), "%b", tick_time);
  strftime(s_buffer_date_right, sizeof(s_buffer_date_right), "%d", tick_time);
  #endif
  #ifdef PBL_PLATFORM_CHALK
  strftime(s_buffer_date_top, sizeof(s_buffer_date_top), "%a", tick_time);
  strftime(s_buffer_date_bottom, sizeof(s_buffer_date_bottom), "%b %d", tick_time);
  strftime(s_buffer_date_right, sizeof(s_buffer_date_right), " ", tick_time);
  #endif
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer_earth, s_buffer_h);
  text_layer_set_text(s_time_layer_date_top, s_buffer_date_top);
  text_layer_set_text(s_time_layer_date_bottom, s_buffer_date_bottom);
  text_layer_set_text(s_time_layer_date_right, s_buffer_date_right);
  
  // Redraw as soon as possible
  layer_mark_dirty(s_canvas_layer);
  
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_buffer_m[8];
  strftime(s_buffer_m, sizeof(s_buffer_m), "%M", tick_time);
  //GPoint center = GPoint(58, 78);
  int32_t distance=56;
 
  #ifdef PBL_PLATFORM_BASALT
  GPoint center = GPoint(72, 90);
  #endif
  #ifdef PBL_PLATFORM_CHALK
  GPoint center = GPoint(90, 90);
  #endif
  int32_t angle=TRIG_MAX_ANGLE * tick_time->tm_min / 60;
  GPoint offset = GPoint((int32_t)(sin_lookup(angle) * distance / TRIG_MAX_RATIO)+center.x,(int32_t)(-cos_lookup(angle) * distance / TRIG_MAX_RATIO)+center.y );
  
  uint32_t radius = 15;
  
  GRect bounds_moon = GRect(offset.x-radius-1, offset.y-radius-2,
                     35, 35);
  // Draw the image
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_bitmap_moon, bounds_moon);
  
  // Load the font
  GFont font = fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);
  // Set the color
  graphics_context_set_text_color(ctx, mcolor);

  // Determine a reduced bounding box
  GRect bounds = GRect(offset.x-radius+2, offset.y-radius+2,
                     radius*2, radius*2);

  // Calculate the size of the text to be drawn, with restricted space
  // Draw the text
  graphics_draw_text(ctx, s_buffer_m, font, bounds, GTextOverflowModeWordWrap, 
                                            GTextAlignmentCenter, NULL);
  
  BatteryChargeState charge=battery_state_service_peek();
  uint8_t charge_percent=charge.charge_percent;
  #ifdef PBL_PLATFORM_BASALT
  uint32_t sun_radius = 10+(15*(uint32_t) charge_percent)/100;
  #endif
  #ifdef PBL_PLATFORM_CHALK
  uint32_t sun_radius = 20+(11*(uint32_t) charge_percent)/100;
  #endif
  GPoint sun_center=GPoint(144,0);
  if(charge_percent<40) {
    graphics_context_set_fill_color(ctx, GColorChromeYellow);
    if(charge_percent<20) {
      graphics_context_set_fill_color(ctx, GColorOrange);
    }
  }
  else {
    graphics_context_set_fill_color(ctx, GColorYellow);
  }
  graphics_fill_circle(ctx, sun_center, sun_radius);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
   window_set_background_color(window, GColorBlack);
  // Get information about the Window
  Layer *window_layer=window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_bitmap_moon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON);
  // insert moon
  s_bitmap_stars = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STARS);
  #ifdef PBL_PLATFORM_BASALT
  s_bitmap_layer_stars = bitmap_layer_create(GRect(0, 0, 144, 180));
  #endif
  #ifdef PBL_PLATFORM_CHALK
  s_bitmap_layer_stars = bitmap_layer_create(GRect(0, 0, 180, 180));
  #endif
  bitmap_layer_set_compositing_mode(s_bitmap_layer_stars, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_layer_stars, s_bitmap_stars);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer_stars));
  // insert earth
  s_bitmap_earth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EARTH);
  #ifdef PBL_PLATFORM_BASALT
  s_bitmap_layer_earth = bitmap_layer_create(GRect(39, 59, 64, 64));
  #endif
  #ifdef PBL_PLATFORM_CHALK
  s_bitmap_layer_earth = bitmap_layer_create(GRect(57, 60, 64, 64));
  #endif
  bitmap_layer_set_compositing_mode(s_bitmap_layer_earth, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_layer_earth, s_bitmap_earth);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer_earth));
   #ifdef PBL_PLATFORM_BASALT
  // Set text for date
  s_time_layer_date_top = text_layer_create(
     // GRect(0, 58, bounds.size.w, 50));
     GRect(0, 0, 100, 30));
  s_time_layer_date_bottom = text_layer_create(
     // GRect(0, 58, bounds.size.w, 50));
     GRect(0, 140, 100, 30));
  s_time_layer_date_right = text_layer_create(
     // GRect(0, 58, bounds.size.w, 50));
     GRect(44, 140, 100, 30));
  #endif
  #ifdef PBL_PLATFORM_CHALK
  // Set text for date
  s_time_layer_date_top = text_layer_create(
     // GRect(0, 58, bounds.size.w, 50));
     GRect(40, 0, 100, 30));
  s_time_layer_date_bottom = text_layer_create(
     // GRect(0, 58, bounds.size.w, 50));
     GRect(40, 160, 100, 30));
  s_time_layer_date_right = text_layer_create(
     // GRect(0, 58, bounds.size.w, 50));
     GRect(44, 140, 100, 30));
  #endif
  dcolor=enamel_get_DateColor();
  text_layer_set_background_color(s_time_layer_date_top, GColorClear);
  text_layer_set_background_color(s_time_layer_date_bottom, GColorClear);
  text_layer_set_background_color(s_time_layer_date_right, GColorClear);
  text_layer_set_text_color(s_time_layer_date_top, dcolor);
  text_layer_set_text_color(s_time_layer_date_bottom, dcolor);
  #ifdef PBL_PLATFORM_BASALT
  text_layer_set_text_color(s_time_layer_date_right, dcolor);
  #endif
  #ifdef PBL_PLATFORM_BASALT
  text_layer_set_font(s_time_layer_date_top, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_font(s_time_layer_date_bottom, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_font(s_time_layer_date_right, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  #endif
  #ifdef PBL_PLATFORM_CHALK
  text_layer_set_font(s_time_layer_date_top, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_font(s_time_layer_date_bottom, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_font(s_time_layer_date_right, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  #endif
  #ifdef PBL_PLATFORM_BASALT
  text_layer_set_text_alignment(s_time_layer_date_top, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_time_layer_date_bottom, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_time_layer_date_right, GTextAlignmentRight);
  #endif
  #ifdef PBL_PLATFORM_CHALK
  text_layer_set_text_alignment(s_time_layer_date_top, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_time_layer_date_bottom, GTextAlignmentCenter);
  #endif
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer_date_top));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer_date_bottom));
  #ifdef PBL_PLATFORM_BASALT
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer_date_right));
  #endif
  // Set text for earth
  // Create the TextLayer with specific bounds
  s_time_layer_earth = text_layer_create(
  #ifdef PBL_PLATFORM_BASALT
     GRect(36, 60, 70, 55));
  #endif
  #ifdef PBL_PLATFORM_CHALK
     GRect(54, 60, 70, 55));
  #endif

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer_earth, GColorClear);
  hcolor=enamel_get_HourColor();
  text_layer_set_text_color(s_time_layer_earth, hcolor);
  text_layer_set_font(s_time_layer_earth, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_time_layer_earth, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer_earth));
  
  // Set text for moon
  // Create the TextLayer with specific bounds
  s_time_layer_moon = text_layer_create(
      GRect(64+40, 82, 30, 20));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer_moon, GColorClear);
  mcolor=enamel_get_MinuteColor();
  text_layer_set_text_color(s_time_layer_moon, mcolor);
  text_layer_set_font(s_time_layer_moon, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer_moon, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer_moon));
  
  // Layer for circle moon

  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
  s_text_layer_event_handle_hcolor = enamel_settings_received_subscribe(enamel_settings_received_text_layer_handler_hcolor, s_time_layer_earth);
  s_text_layer_event_handle_mcolor = enamel_settings_received_subscribe(enamel_settings_received_text_layer_handler_mcolor, s_time_layer_moon);
  s_text_layer_event_handle_dcolor = enamel_settings_received_subscribe(enamel_settings_received_text_layer_handler_dcolor, s_time_layer_date_top);
  
}

static void main_window_unload(Window *window) {
  enamel_settings_received_unsubscribe(s_text_layer_event_handle_hcolor);
  enamel_settings_received_unsubscribe(s_text_layer_event_handle_mcolor);
  enamel_settings_received_unsubscribe(s_text_layer_event_handle_dcolor);
  // Destroy TextLayer
  text_layer_destroy(s_time_layer_earth);
  text_layer_destroy(s_time_layer_moon);
  text_layer_destroy(s_time_layer_date_top);
  text_layer_destroy(s_time_layer_date_bottom);
  text_layer_destroy(s_time_layer_date_right);
  layer_destroy(s_canvas_layer);
  gbitmap_destroy(s_bitmap_earth);
  bitmap_layer_destroy(s_bitmap_layer_earth);
  gbitmap_destroy(s_bitmap_stars);
  bitmap_layer_destroy(s_bitmap_layer_stars);
  gbitmap_destroy(s_bitmap_moon);
  
  
}


static void init() {
  // Initialize Enamel to register App Message handlers and restores settings
  enamel_init();
  // Subscribe a handler for a text layer
  
  // call pebble-events app_message_open function
   events_app_message_open(); 
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  

}

static void deinit() {
  // Unsubscribe from Enamel events
    
  
  // Deinit Enamel to unregister App Message handlers and save settings
  enamel_deinit();
  // Destroy Window
  window_destroy(s_main_window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

