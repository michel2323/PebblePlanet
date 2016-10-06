/**
 * This file was generated with Enamel : http://gregoiresage.github.io/enamel
 */

#ifndef ENAMEL_H
#define ENAMEL_H

#include <pebble.h>

// -----------------------------------------------------
// Getter for 'HourColor'
GColor enamel_get_HourColor();
// -----------------------------------------------------

// -----------------------------------------------------
// Getter for 'MinuteColor'
GColor enamel_get_MinuteColor();
// -----------------------------------------------------

// -----------------------------------------------------
// Getter for 'DateColor'
GColor enamel_get_DateColor();
// -----------------------------------------------------

void enamel_init();

void enamel_deinit();

typedef void* EventHandle;
typedef void(EnamelSettingsReceivedHandler)(void* context);

EventHandle enamel_settings_received_subscribe(EnamelSettingsReceivedHandler *handler, void *context);
void enamel_settings_received_unsubscribe(EventHandle handle);

#endif