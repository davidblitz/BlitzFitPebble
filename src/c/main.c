#include <pebble.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Window *s_window;
static TextLayer *s_text_layer;

static char s_activity[12] = "Running";
static float prog = 0.0;
static int prog_resolution = 12;
static char final_string[100] = "";
static char progress_string[50] = "";
static bool inProgress = false;
static bool s_js_ready;

static int target_time_seconds = 100; //need to get
static int prog_time_seconds = 0; //need to send

bool comm_is_js_ready() {
  return s_js_ready;
}

static void send_done_signal() {
  //Send current message to phone
    // Declare the dictionary's iterator
    DictionaryIterator *out_iter;
    
    // Prepare the outbox buffer for this message
    AppMessageResult result = app_message_outbox_begin(&out_iter);
    if(result == APP_MSG_OK) {
      bool act_done = true;
      dict_write_int(out_iter, MESSAGE_KEY_ActivityDone, &act_done, sizeof(bool), true);
    
      // Send this message
      result = app_message_outbox_send();
      if(result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
      }
    } else {
      // The outbox cannot be used right now
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
    }
}

static void progress_float_to_string(char *new_string, float prog, int prog_resolution)
{
  snprintf(new_string, 2, "[");
  int number_plus = (int) (prog_resolution * prog);
  
  for(int i=0; i< prog_resolution; i++) {
    if(i < number_plus) strcat(new_string, "+");
    else strcat(new_string, "-");
  }

  strcat(new_string, "]");
  
  strcat(new_string, "\n");
  
  char perc_string[4] = "";
  int percentage = (int) (prog*100);
  snprintf(perc_string, 4, "%d", percentage);

  strcat(perc_string, "%");
    
  strcat(new_string, perc_string);

  return;
}

static void update_display() {
  
  snprintf(final_string, 12, "%s", s_activity);
  strcat(final_string, "\n");
  
  progress_float_to_string(progress_string, prog, prog_resolution);
  strcat(final_string, progress_string);
  
  text_layer_set_text(s_text_layer, final_string);
}

static void increment_progress() {
  if(inProgress) {
    if(prog_time_seconds < target_time_seconds) {
      prog_time_seconds += 1;
    }
    else {
      prog_time_seconds = target_time_seconds;
      inProgress = false;
      
      send_done_signal();
    }
    prog = (float) prog_time_seconds / (float) target_time_seconds;
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  increment_progress();
  update_display();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // A single click has just occured
  inProgress = !inProgress;
}

static void click_config_provider(void *context) {
  ButtonId id = BUTTON_ID_SELECT;  // The Select button

  window_single_click_subscribe(id, select_click_handler);
}


static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // A message was received, but had to be dropped
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}


static void outbox_sent_callback(DictionaryIterator *iter, void *context) {
  // The message just sent has been successfully delivered

}

static void outbox_failed_callback(DictionaryIterator *iter,
                                      AppMessageResult reason, void *context) {
  // The message just sent failed to be delivered
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int)reason);
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
    Tuple *ready_tuple = dict_find(iter, MESSAGE_KEY_JSReady);
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox Received");
  if(ready_tuple) {
    // PebbleKit JS is ready! Safe to send messages
    s_js_ready = true;
    
    APP_LOG(APP_LOG_LEVEL_INFO, "JSReady Received");
  }
  
  Tuple *target_tuple = dict_find(iter, MESSAGE_KEY_Target);
  if(target_tuple) {
    int32_t target_time = target_tuple->value->int32;
    
    APP_LOG(APP_LOG_LEVEL_INFO, "Target Time Received: %d", (int)target_time);
    
    target_time_seconds = (int) target_time;
  }
}

static void init(void) {
  // Create a window and get information about the window
  s_window = window_create();
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  
  
  //ACTIVITY
  // Create a text layer and set the text
  s_text_layer = text_layer_create(bounds);

  update_display();
    
  // Set the font and text alignment
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);

  // Add the text layer to the window
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_text_layer));
  
  // Enable text flow and paging on the text layer, with a slight inset of 10, for round screens
  text_layer_enable_screen_text_flow_and_paging(s_text_layer, 10);
  
  // Push the window, setting the window animation to 'true'
  window_stack_push(s_window, true);
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  window_set_click_config_provider(s_window, click_config_provider);
 
  // Register to be notified about inbox received events
app_message_register_inbox_received(inbox_received_callback);
// Register to be notified about inbox dropped events
app_message_register_inbox_dropped(inbox_dropped_callback);
 
  // Register to be notified about outbox sent events
app_message_register_outbox_sent(outbox_sent_callback);
  
  // Register to be notified about outbox failed events
app_message_register_outbox_failed(outbox_failed_callback);

    // Open AppMessage
const int inbox_size = 128;
const int outbox_size = 128;
app_message_open(inbox_size, outbox_size);
  
  // App Logging!
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");
}

static void deinit(void) {
  // Destroy the text layer
  text_layer_destroy(s_text_layer);
  
  // Destroy the window
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}