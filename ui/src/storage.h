/* ui/src/storage.h */
#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

/* Call once at boot, after PIN is verified and key is derived */
int  storage_open(const uint8_t *key32);  /* 32-byte AES key */
void storage_close(void);

/* Contacts */
typedef struct {
    int   id;
    char  name[64];
    char  number[32];
} Contact;

int storage_contact_add(const char *name, const char *number);
int storage_contact_list(Contact *out, int max_count);
int storage_contact_delete(int id);

/* Messages */
typedef struct {
    int   id;
    char  number[32];
    char  body[256];
    int   direction;   /* 0 = incoming, 1 = outgoing */
    long  timestamp;
} Message;

int storage_message_add(const char *number, const char *body, int direction);
int storage_message_list(const char *number, Message *out, int max_count);
int storage_thread_list(Message *out, int max_count);
int storage_message_delete_all(const char *number);

#endif
