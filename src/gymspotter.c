#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static int s_timer = 0;
static int s_max_timer = 0;
static bool timer_running = false;
static GFont s_res_bitham_30_black;
static const uint32_t const vibe_segments[] = { 1000, 300, 1000 };
static const int const max_timer_settings[] = { 45, 60, 75, 90, 105, 120 };
static GFont s_res_gothic_18_bold;
static TextLayer *s_textlayer_rest;
static TextLayer *s_textlayer_max;

static void tap_handler(AccelAxisType axis, int32_t direction) {
  s_timer = 0;
  timer_running = true;
  vibes_long_pulse();
  layer_set_hidden(text_layer_get_layer(s_textlayer_rest), false);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_timer = 0;
  timer_running = true;
  layer_set_hidden(text_layer_get_layer(s_textlayer_rest), false);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_timer = 0;
  timer_running = false;
  layer_set_hidden(text_layer_get_layer(s_textlayer_rest), true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_timer = 0;
  timer_running = false;
  layer_set_hidden(text_layer_get_layer(s_textlayer_rest), true);
  
  if(s_max_timer < (int) ARRAY_LENGTH(max_timer_settings) - 1){
    s_max_timer++;
  } else {
    s_max_timer = 0;
  }
  static char s_max_buffer[4];
  snprintf(s_max_buffer, sizeof(s_max_buffer), "%d", max_timer_settings[s_max_timer]);
  text_layer_set_text(s_textlayer_max, s_max_buffer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  if(persist_exists(0)) {
    s_max_timer = persist_read_int(0);
  }
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  // text_layer
  text_layer = text_layer_create(GRect(20, 61, 100, 38));
  text_layer_set_text(text_layer, "00:00");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, s_res_bitham_30_black);
  
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  // s_textlayer_rest
  s_textlayer_rest = text_layer_create(GRect(34, 20, 75, 24));
  text_layer_set_background_color(s_textlayer_rest, GColorBlack);
  text_layer_set_text_color(s_textlayer_rest, GColorWhite);
  text_layer_set_text(s_textlayer_rest, "REST");
  text_layer_set_text_alignment(s_textlayer_rest, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_rest, s_res_gothic_18_bold);
  
  s_textlayer_max = text_layer_create(GRect(34, 120, 75, 24));
  text_layer_set_background_color(s_textlayer_max, GColorWhite);
  text_layer_set_text_color(s_textlayer_max, GColorBlack);
  
  static char s_max_buffer[4];
  snprintf(s_max_buffer, sizeof(s_max_buffer), "%d", max_timer_settings[s_max_timer]);

  text_layer_set_text(s_textlayer_max, s_max_buffer);
  text_layer_set_text_alignment(s_textlayer_max, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_max, s_res_gothic_18_bold);  
  
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_rest));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_max));
  layer_set_hidden(text_layer_get_layer(s_textlayer_rest), true);
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  static char s_timer_buffer[6];

  int seconds = s_timer % 60;
  int minutes = (s_timer % 3600) / 60;

  snprintf(s_timer_buffer, sizeof(s_timer_buffer), "%02d:%02d", minutes, seconds);
  text_layer_set_text(text_layer, s_timer_buffer);

  if(timer_running) {
    if(s_timer == max_timer_settings[s_max_timer]){
      timer_running = false;
      VibePattern pat = {
        .durations = vibe_segments,
        .num_segments = ARRAY_LENGTH(vibe_segments),
      };
      vibes_enqueue_custom_pattern(pat);
    } else {
      s_timer++;
    }
  }
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
  .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  accel_tap_service_subscribe(tap_handler);
}

static void deinit(void) {
  persist_write_int(0, s_max_timer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
