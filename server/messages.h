#ifndef _MESSGES_H
#define _MESSAGES_H

#include "proto.h"


void message_send_status(int status, int sock);
char *message_login(struct proto_message *m, int sock);
void message_do_logout(char *login);
void message_logout(char *login, int sock);
void message_receive(char *login, struct proto_message *m, int sock);
void message_send(char kind, struct timeval time, const char *login, const char *body, int sock);
void message_history(char *login, struct proto_message *m, int sock);
void message_list(char *login, int sock);
void message_kick(char *login, struct proto_message *m, int sock);

#endif
