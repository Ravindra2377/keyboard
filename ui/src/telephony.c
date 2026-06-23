/*
 * StealthOS — Telephony Bridge
 * Talks to oFono via D-Bus for making calls and sending SMS.
 */

#include "stealth_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
// Uncomment when libdbus-1-dev is added to buildroot
// #include <dbus/dbus.h>

// static DBusConnection *g_dbus = NULL;

int telephony_init(void) {
    fprintf(stderr, "[tel] Initializing oFono D-Bus bridge (stubbed for now)\n");
    /*
    DBusError err;
    dbus_error_init(&err);
    g_dbus = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "[tel] D-Bus connection error: %s\n", err.message);
        dbus_error_free(&err);
        return -1;
    }
    */
    return 0;
}

void telephony_destroy(void) {
    /*
    if (g_dbus) {
        dbus_connection_unref(g_dbus);
    }
    */
}

int telephony_dial(const char *number) {
    fprintf(stderr, "[tel] Dialing: %s\n", number);
    /*
    if (!g_dbus) return -1;
    DBusMessage *msg = dbus_message_new_method_call(
        "org.ofono",           // destination
        "/sim800l_0",          // object path
        "org.ofono.VoiceCallManager",
        "Dial"
    );
    if (!msg) return -1;

    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &number);
    const char *hide_id = "";
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &hide_id);

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        g_dbus, msg, 5000, NULL);
    dbus_message_unref(msg);

    if (!reply) {
        fprintf(stderr, "[tel] Dial failed via D-Bus\n");
        return -1;
    }
    dbus_message_unref(reply);
    */
    return 0;
}

int telephony_hangup(void) {
    fprintf(stderr, "[tel] Hangup\n");
    /*
    if (!g_dbus) return -1;
    DBusMessage *msg = dbus_message_new_method_call(
        "org.ofono",
        "/sim800l_0",
        "org.ofono.VoiceCallManager",
        "HangupAll"
    );
    if (!msg) return -1;
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(g_dbus, msg, 5000, NULL);
    dbus_message_unref(msg);
    if (reply) dbus_message_unref(reply);
    */
    return 0;
}

int telephony_answer(void) {
    fprintf(stderr, "[tel] Answer\n");
    /* Needs similar implementation to Hangup but targeting incoming call */
    return 0;
}

int telephony_send_sms(const char *number, const char *text) {
    fprintf(stderr, "[tel] SMS to %s: %s\n", number, text);
    /*
    if (!g_dbus) return -1;
    DBusMessage *msg = dbus_message_new_method_call(
        "org.ofono",
        "/sim800l_0",
        "org.ofono.MessageManager",
        "SendMessage"
    );
    if (!msg) return -1;

    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &number);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text);

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        g_dbus, msg, 5000, NULL);
    dbus_message_unref(msg);

    if (!reply) {
        fprintf(stderr, "[tel] SMS failed via D-Bus\n");
        return -1;
    }
    dbus_message_unref(reply);
    */
    return 0;
}

void telephony_set_call_callback(call_state_cb cb) { (void)cb; }
void telephony_set_sms_callback(sms_received_cb cb) { (void)cb; }

/* Internal handler called by D-Bus when an SMS is received */
void telephony_on_incoming_sms(const char *number, const char *text) {
    fprintf(stderr, "[tel] Incoming SMS from %s: %s\n", number, text);
    storage_message_add(number, text, 0); /* 0 = incoming */
    /* TODO: notify UI or invoke sms_received_cb */
}
