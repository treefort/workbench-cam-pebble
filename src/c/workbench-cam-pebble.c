#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;
static Layer *s_canvas_layer;

typedef struct {
  int hours;
  int minutes;
} Time;

static Time s_last_time;
static char time_str[6];

static GPoint rec_center;
static GRect stop_rect;
static uint16_t rec_radius = 20;
static uint16_t stop_radius = 4;

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *action_result_tuple = dict_find(iter, MESSAGE_KEY_CamActionResult);

  // if the message tuple is the MESSAGE_KEY_CamActionResult
  if (action_result_tuple) {

    // get the resulting value
    bool actionSuccessful = action_result_tuple->value->uint8 == 1;
    if (actionSuccessful) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Cam Action Success!");
      // short pulse, everything's fine
      vibes_short_pulse();
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Cam Action Fail!");
      // double pulse, everything's on fire, yo
      vibes_double_pulse();
    }
  }
}

static void prv_cam_action(int action) {
  DictionaryIterator *out_iter;

  // Prepare the outbox buffer for this message
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if(result == APP_MSG_OK) {
    // Construct the message
    int value = action;
    // Add an item to ask for weather data
    dict_write_int(out_iter, MESSAGE_KEY_CamAction, &value, sizeof(int), true);
    result = app_message_outbox_send();

    // Check the result
    if(result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Message not sent! %d", (int)result);
    }
  } else {
    // The outbox cannot be used right now
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
  }
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // send 'start' action
  prv_cam_action(1);
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // send 'stop' action
  prv_cam_action(0);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // update time
  text_layer_set_text(s_text_layer, time_str);

  // draw red 'rec' circle
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, rec_center, rec_radius);

  // draw black 'stop' rect
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, stop_rect, stop_radius, GCornersAll);
}

static void prv_window_load(Window *window) {
  // setup interface elements
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 55, bounds.size.w, 70));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));

  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  layer_destroy(s_canvas_layer);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits changed) {
  // this runs once every minute
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.minutes = tick_time->tm_min;

  snprintf(time_str, sizeof(time_str), "%02u:%02u", s_last_time.hours, s_last_time.minutes);

  if (s_canvas_layer) {
    // mark the layer as 'dirty' so it redraws
    layer_mark_dirty(s_canvas_layer);
  }
}

static void prv_init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);

  // load up the current time on init
  srand(time(NULL));
  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  prv_tick_handler(time_now, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  // define some geometry for our rec and stop icons
  rec_center = GPoint(118, 27);
  stop_rect = GRect(99, 118, 38, 38);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(64, 64);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
