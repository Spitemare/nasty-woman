#include <pebble.h>
#include <pebble-layout/pebble-layout.h>
#include <pebble-events/pebble-events.h>
#include <pebble-hourly-vibes/hourly-vibes.h>
#include <pebble-connection-vibes/connection-vibes.h>
#include "enamel.h"
#include "logging.h"

static Window *s_window;
static Layout *s_layout;
static EventHandle s_tick_timer_event_handle;

static EventHandle s_settings_received_event_handle;

static inline GColor prv_color_background(void) {
    logf();
#ifdef PBL_COLOR
    return enamel_get_COLOR_BACKGROUND();
#else
    return enamel_get_COLOR_INVERT() ? GColorWhite : GColorBlack;
#endif
}

static inline GColor prv_color_foreground(void) {
    logf();
    return gcolor_legible_over(prv_color_background());
}

static void prv_settings_received_handler(void *context) {
    logf();
    hourly_vibes_set_enabled(enamel_get_HOURLY_VIBE());
    connection_vibes_set_state(atoi(enamel_get_CONNECTION_VIBE()));
#ifdef PBL_HEALTH
    connection_vibes_enable_health(enamel_get_ENABLE_HEALTH());
    hourly_vibes_enable_health(enamel_get_ENABLE_HEALTH());
#endif

    GColor background = prv_color_background();
    GColor foreground = prv_color_foreground();

    window_set_background_color(s_window, background);
    text_layer_set_text_color((TextLayer *) layout_find_by_id(s_layout, "time"), foreground);
    text_layer_set_text_color((TextLayer *) layout_find_by_id(s_layout, "text"), foreground);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed, void *context) {
    logf();
    TextLayer *text_layer = (TextLayer *) context;
    char *buf = (char *) text_layer_get_text(text_layer);

    char s[8];
#ifdef DEMO
    snprintf(s, sizeof(s), "12:34");
#else
    strftime(s, sizeof(s), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
#endif
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
    s_tick_timer_event_handle = events_tick_timer_service_subscribe_context(MINUTE_UNIT, prv_tick_handler, time_layer);

    prv_settings_received_handler(NULL);
    s_settings_received_event_handle = enamel_settings_received_subscribe(prv_settings_received_handler, NULL);
}

static void prv_window_unload(Window *window) {
    logf();
    enamel_settings_received_unsubscribe(s_settings_received_event_handle);
    events_tick_timer_service_unsubscribe(s_tick_timer_event_handle);
    layout_destroy(s_layout);
}

static void prv_init(void) {
    logf();
    enamel_init();
    connection_vibes_init();
    hourly_vibes_init();
    uint32_t const pattern[] = { 100 };
    hourly_vibes_set_pattern((VibePattern) {
        .durations = pattern,
        .num_segments = 1
    });
    events_app_message_open();

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

    connection_vibes_deinit();
    hourly_vibes_deinit();
    enamel_deinit();
}

int main(void) {
    logf();
    prv_init();
    app_event_loop();
    prv_deinit();
}
