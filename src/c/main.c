#include <pebble.h>
#include <pebble-layout/pebble-layout.h>
#include <pebble-events/pebble-events.h>
#include "logging.h"

static Window *s_window;
static Layout *s_layout;
static EventHandle s_event_handle;

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed, void *context) {
    logf();
    TextLayer *text_layer = (TextLayer *) context;
    char *buf = (char *) text_layer_get_text(text_layer);

    char s[8];
    strftime(s, sizeof(s), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    memcpy(buf, s, sizeof(s));

    text_layer_set_text((TextLayer *) context, buf);
}

static void prv_window_load(Window *window) {
    logf();
    s_layout = layout_create();
    layout_add_standard_type(s_layout, StandardTypeText);
    layout_add_font(s_layout, "ABRIL", RESOURCE_ID_FONT_ABRIL_34);

    layout_parse(s_layout, RESOURCE_ID_LAYOUT);
    layer_add_child(window_get_root_layer(window), layout_get_root_layer(s_layout));

    TextLayer *time_layer = layout_find_by_id(s_layout, "time");
    time_t now = time(NULL);
    prv_tick_handler(localtime(&now), MINUTE_UNIT, time_layer);
    s_event_handle = events_tick_timer_service_subscribe_context(MINUTE_UNIT, prv_tick_handler, time_layer);
}

static void prv_window_unload(Window *window) {
    logf();
    events_tick_timer_service_unsubscribe(s_event_handle);
    layout_destroy(s_layout);
}

static void prv_init(void) {
    logf();
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload
    });
    window_stack_push(s_window, true);
}

static void prv_deinit(void) {
    logf();
    window_destroy(s_window);
}

int main(void) {
    logf();
    prv_init();
    app_event_loop();
    prv_deinit();
}
