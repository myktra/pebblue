#include <pebble.h>
#include "pebblue.h"

// redefine APP_LOG to release bluetooth load
// (comment these lines out to reveal further detail on the Pebble side)
#undef APP_LOG
#define APP_LOG(...)

static Window *window;
static TextLayer *text_layer;

static AppTimer *timer;

// user selectable speed params
#define SPEED_MAX 5000
#define SPEED_MIN 25
#define DEFAULT_SPEED 500
#define SPEED_INCREMENT 25

// start at 2 updates per second (2 Hz)
static unsigned int speed = DEFAULT_SPEED;

// incrementing counter to send to the device
static uint32_t key = 0;
static int counter = 0;

// custom vibe pattern
static const uint32_t const segments[] = { 25 };
VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};

// error message helper
char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void set_timer() {
  timer = app_timer_register(speed, timer_callback, NULL);
}

static void timer_callback() {

  // "feel the speed"
  //vibes_enqueue_custom_pattern(pat);
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // send the counter to the host device
  Tuplet value_test = TupletInteger(key, counter);
  dict_write_tuplet(iter, &value_test);
  dict_write_end(iter);
  AppMessageResult app_message_error = app_message_outbox_send();
  
  if (app_message_error == APP_MSG_OK) {
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %s", translate_error(app_message_error));
  }

  // fire it up again
  set_timer();
  
}

// lay out the current speed
static void print_speed() {
  static char buf[16];
  snprintf(buf, 16, "%u", speed);
  text_layer_set_text(text_layer, buf);
}

// reset counter
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Go");
  counter = 0;
  print_speed();
  set_timer();
}

// increase frequency of updates and reset counter
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (speed > SPEED_MIN) {
    speed -= SPEED_INCREMENT;
  }
  counter = 0;
  print_speed();  
}

// decrease frequency of updates and reset counter
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (speed < SPEED_MAX) {
    speed += SPEED_INCREMENT;
  }
  counter = 0;
  print_speed();  
}

// click configuration
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "pebblue");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  app_timer_cancel(timer);
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sent! %i", counter);
  counter++;
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
  APP_LOG(APP_LOG_LEVEL_DEBUG, "out failed!");
}

void in_received_handler(DictionaryIterator *received, void *context) {
  // incoming message received
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in received!");   
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in dropped!");
}
 
static void init(void) {
  
  // create and load the window
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  
  // set up handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  
  // set up app message queue
  const uint32_t inbound_size = 16;
  const uint32_t outbound_size = 16;
  app_message_open(inbound_size, outbound_size);
  
  // show the window
  window_stack_push(window, animated);

}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  // sniff more - better performance at the expense of
  // some additional power drain
  app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);

  app_event_loop();
  deinit();
}
