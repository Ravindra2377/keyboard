/* telephony_stub.c — Mock telephony layer for macOS simulator testing */
#include <stdio.h>
#include <unistd.h>
#include "stealth_ui.h"
#include "storage.h"

static call_state_cb g_call_cb = NULL;
static sms_received_cb g_sms_cb = NULL;

int telephony_init(void) {
    printf("[Sim] Telephony Init\n");
    return 0;
}

void telephony_destroy(void) {
    printf("[Sim] Telephony Destroy\n");
}

int telephony_dial(const char *number) {
    printf("[Sim] Dialing %s...\n", number);
    if (g_call_cb) g_call_cb(number, "active"); /* Active */
    return 0;
}

int telephony_hangup(void) {
    printf("[Sim] Hangup\n");
    if (g_call_cb) g_call_cb(NULL, "disconnected"); /* Disconnected */
    return 0;
}

int telephony_answer(void) {
    printf("[Sim] Answered call\n");
    if (g_call_cb) g_call_cb("Unknown", "active");
    return 0;
}

int telephony_send_sms(const char *number, const char *text) {
    printf("[Sim] SMS to %s: %s\n", number, text);
    storage_message_add(number, text, 1); /* 1 for OUT */
    return 0;
}

void telephony_set_call_callback(call_state_cb cb) {
    g_call_cb = cb;
}

void telephony_set_sms_callback(sms_received_cb cb) {
    g_sms_cb = cb;
}

/* Simulate an incoming SMS (can be triggered via UI menu in future) */
void telephony_on_incoming_sms(const char *number, const char *text, uint32_t timestamp) {
    storage_message_add(number, text, 0); /* 0 for IN */
    if (g_sms_cb) g_sms_cb(number, text, timestamp);
}
