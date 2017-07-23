#ifndef _PROTO_H
#define _PROTO_H

#include <stdlib.h>
#include <sys/time.h>

struct proto_line
{
    char *data;
    unsigned length;
};

struct proto_message
{
    struct proto_line *lines;
    unsigned line_count;
    char type;
};

int proto_recv_packet(char *buf, int sock);

void proto_free(struct proto_message *s);
struct proto_message *proto_decode(char *buf, size_t length);
unsigned proto_encode(struct proto_message *msg, char **buf);
int proto_get_int(struct proto_message *msg, unsigned line_no);
char *proto_get_str(struct proto_message *msg, unsigned line_no);
unsigned proto_get_len(struct proto_message *msg, unsigned line_no);
unsigned proto_get_line_count(struct proto_message *msg);
char proto_get_type(struct proto_message *msg);
void proto_get_timeval(struct proto_message *msg, unsigned line_no, struct timeval *timestamp);
struct proto_message *proto_create(char type, unsigned line_count);
void proto_set_int(struct proto_message *msg, unsigned line_no, int value);
void proto_set_blob(struct proto_message *msg, unsigned line_no, const char *value, unsigned len);
void proto_set_str(struct proto_message *msg, unsigned line_no, const char *value);
void proto_set_timeval(struct proto_message *msg, unsigned line_no, struct timeval *value);

#define STATUS_OK 0
#define STATUS_UNKNOWN_TYPE 1
#define STATUS_LOGIN_REQUIRED 2
#define STATUS_AUTH_ERROR 3
#define STATUS_SIGNUP_ERROR 4
#define STATUS_ACCESS_DENIED 5
#define STATUS_INVALID_MESSAGE 6
#define STATUS_NO_SUCH_USER 7

#define MESSAGE_BUF_SIZE (1 << 17)

#endif
