#ifndef _CHAT_H
#define _CHAT_H

#include <sys/time.h>

struct chat_user_list
{
    long long uid;
    char login[32];
    struct chat_user_list *next;
};

void chat_init(char *root_password);
char *chat_get_user_password(char *login);
int chat_validate_login(char *login);
int chat_create_user(char *login, char *password);
void chat_create_session(char *login);
void chat_delete_session(const char *login);

void chat_new_message(char *kind, const char *login, const char *msg);
long long chat_last_message();
void chat_send_all(char *my_login, long long from, long long to, int sock);
void chat_send_history(int cnt, int sock);
int chat_get_user_list(struct chat_user_list **st);
void chat_free_user_list(struct chat_user_list *s);
int chat_kick_user(long long uid, const char *reason);
void chat_clean();
#endif
