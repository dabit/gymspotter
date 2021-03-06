#include <pebble.h>

#define GRACE_PERIOD 5

#if defined(PBL_RECT)
  #define DEVICE_WIDTH 144
  #define DEVICE_HEIGHT 168
  #define MAX_LAYER_POSITION 100
  #define TIMER_FONT fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK)
  #define TIMER_HEIGHT 38
  #define REST_FONT fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
  #define REST_HEIGHT 24
  #define REST_BACKGROUND_COLOR GColorBlack
  #define MAX_FONT fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
  #define TOD_FONT fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
  #define TOD_HEIGHT 20
  #define TOD_POSITION 45
#elif defined(PBL_ROUND)
  #define DEVICE_WIDTH 180
  #define DEVICE_HEIGHT 180
  #define MAX_LAYER_POSITION 120
  #define TIMER_FONT fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49)
  #define TIMER_HEIGHT 50
  #define REST_FONT fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)
  #define REST_HEIGHT 32
  #define REST_BACKGROUND_COLOR GColorRed
  #define MAX_FONT fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
  #define TOD_FONT fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
  #define TOD_HEIGHT 20
  #define TOD_POSITION 50
#endif

static Window *window;
static TextLayer *s_textlayer_timer;
static TextLayer *s_textlayer_rest;
static TextLayer *s_textlayer_max;
static TextLayer *s_textlayer_tod;

static int s_timer = 0;
static int s_max_timer = 0;
static int s_grace_timer = 0;
static bool s_timer_running = false;
static const uint32_t const s_vibe_segments[] = { 1000, 300, 1000 };
static const int const s_max_timer_settings[] = { 15, 30, 45, 60, 75, 90, 105, 120 };

static void set_rest_layer_visible(bool visibility) {
  layer_set_hidden(text_layer_get_layer(s_textlayer_rest), !visibility);
}

static void timer_start() {
  s_timer = 0;
  s_timer_running = true;
  set_rest_layer_visible(true);
}

static void timer_stop() {
  s_timer = 0;
  s_timer_running = false;
  set_rest_layer_visible(false);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if(!s_timer_running && s_grace_timer == 0) {
    timer_start();
    vibes_long_pulse();
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  timer_start();
}

static void cycle_timer(int direction) {
  s_max_timer = s_max_timer + direction;

  if(s_max_timer > (int) ARRAY_LENGTH(s_max_timer_settings) - 1){
    s_max_timer = 0;
  }

  if(s_max_timer < 0) {
    s_max_timer = ARRAY_LENGTH(s_max_timer_settings) - 1;
  }

  static char s_max_buffer[20];
  snprintf(s_max_buffer, sizeof(s_max_buffer), "Stop at:\n%02ds", s_max_timer_settings[s_max_timer]);
  text_layer_set_text(s_textlayer_max, s_max_buffer);
}
static void cycle_timer_up() {
  cycle_timer(1);
}

static void cycle_timer_down() {
  cycle_timer(-1);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_timer_running) {
    timer_stop();
  } else {
    cycle_timer_up();
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_timer_running) {
    timer_stop();
  } else {
    cycle_timer_down();
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void update_time_of_day() {
  static char s_tod_buffer[6];
  clock_copy_time_string(s_tod_buffer, sizeof(s_tod_buffer));
  text_layer_set_text(s_textlayer_tod, s_tod_buffer);
}

static void init_time_of_day(Layer *window_layer) {
  s_textlayer_tod = text_layer_create(GRect(0, TOD_POSITION, DEVICE_WIDTH - 30, TOD_HEIGHT));
  text_layer_set_text_alignment(s_textlayer_tod, GTextAlignmentRight);
  text_layer_set_font(s_textlayer_tod, TOD_FONT);
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_tod));
  update_time_of_day();
}

static void init_rest_label(Layer *window_layer) {
  s_textlayer_rest = text_layer_create(GRect(0, 15, DEVICE_WIDTH, REST_HEIGHT));
  text_layer_set_background_color(s_textlayer_rest, REST_BACKGROUND_COLOR);
  text_layer_set_text_color(s_textlayer_rest, GColorWhite);
  text_layer_set_text(s_textlayer_rest, "REST");
  text_layer_set_text_alignment(s_textlayer_rest, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_rest, REST_FONT);
  set_rest_layer_visible(false);
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_rest));
}

static void init_timer_label(Layer *window_layer) {
  s_textlayer_timer = text_layer_create(GRect(0, (DEVICE_HEIGHT/2 - TIMER_HEIGHT/2), DEVICE_WIDTH, TIMER_HEIGHT));
  text_layer_set_text(s_textlayer_timer, "00:00");
  text_layer_set_text_alignment(s_textlayer_timer, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_timer, TIMER_FONT);
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_timer));
}

static void init_max_label(Layer *window_layer) {
  s_textlayer_max = text_layer_create(GRect(0, MAX_LAYER_POSITION, DEVICE_WIDTH, 48));
  text_layer_set_background_color(s_textlayer_max, GColorWhite);
  text_layer_set_text_color(s_textlayer_max, GColorBlack);

  static char s_max_buffer[16];
  snprintf(s_max_buffer, sizeof(s_max_buffer), "Stop at:\n%02ds", s_max_timer_settings[s_max_timer]);

  text_layer_set_text(s_textlayer_max, s_max_buffer);
  text_layer_set_text_alignment(s_textlayer_max, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_max, MAX_FONT);

  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_max));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  if(persist_exists(0)) {
    s_max_timer = persist_read_int(0);
  }

  init_timer_label(window_layer);
  init_rest_label(window_layer);
  init_max_label(window_layer);
  init_time_of_day(window_layer);

  // Enable for screenshots
  light_enable(false);
}

static void window_unload(Window *window) {
  text_layer_destroy(s_textlayer_timer);
  text_layer_destroy(s_textlayer_rest);
  text_layer_destroy(s_textlayer_max);
  text_layer_destroy(s_textlayer_tod);
}

static void long_vibration() {
  VibePattern pat = {
    .durations = s_vibe_segments,
    .num_segments = ARRAY_LENGTH(s_vibe_segments),
  };
  vibes_enqueue_custom_pattern(pat);
}

static void timer_is_done() {
  s_grace_timer = GRACE_PERIOD;
  s_timer_running = false;
  long_vibration();
  light_enable_interaction();
  set_rest_layer_visible(false);
}

static void update_timer() {
  static char s_timer_buffer[6];
  int seconds = s_timer % 60;
  int minutes = (s_timer % 3600) / 60;

  snprintf(s_timer_buffer, sizeof(s_timer_buffer), "%02d:%02d", minutes, seconds);
  text_layer_set_text(s_textlayer_timer, s_timer_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time_of_day();
  update_timer();

  if(s_grace_timer > 0) {
    s_grace_timer--;
  }

  if(s_timer_running) {
    if(s_timer == s_max_timer_settings[s_max_timer]){
      timer_is_done();
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
