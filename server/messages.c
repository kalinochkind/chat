#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include "messages.h"
#include "proto.h"
#include "chat.h"


static void _send(struct proto_message *p, int sock)
{
    char *buf;
    unsigned len = proto_encode(p, &buf);
    proto_free(p);
    send(sock, buf, len, 0);
    free(buf);
}

void message_send_status(int status, int sock)
{
    struct proto_message *p = proto_create('s', 1);
    proto_set_int(p, 0, status);
    _send(p, sock);
}

char *message_login(struct proto_message *m, int sock)
{
    if(proto_get_line_count(m) < 2)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return 0;
    }
    char *login = proto_get_str(m, 0);
    char *password = proto_get_str(m, 1);
    if(!chat_validate_login(login))
    {
        message_send_status(STATUS_SIGNUP_ERROR, sock);
        return 0;
    }
    char *db_password = chat_get_user_password(login);
    if(!db_password)
    {
        if(strlen(password) < 2 || strlen(password) > 31)
        {
            message_send_status(STATUS_SIGNUP_ERROR, sock);
            return 0;
        }
        chat_create_user(login, password);
        printf("Created user %s\n", login);
    }
    else if(strcmp(password, db_password))
    {
        free(db_password);
        message_send_status(STATUS_AUTH_ERROR, sock);
        return 0;
    }
    free(db_password);
    message_send_status(STATUS_OK, sock);
    chat_create_session(login);

    char buf[64];
    strcpy(buf, login);
    strcat(buf, " logged in");
    chat_new_message("m", "", buf);
    char *t = malloc(strlen(login) + 1);
    strcpy(t, login);
    return t;
}

void message_do_logout(char *login)
{
    chat_delete_session(login);
    printf("%s logged out\n", login);
    char buf[64];
    strcpy(buf, login);
    strcat(buf, " logged out");
    chat_new_message("m", "", buf);

}

void message_logout(char *login, int sock)
{
    if(login)
    {
        message_do_logout(login);
        message_send_status(STATUS_OK, sock);
    }
    else
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
    }
}

void message_receive(char *login, struct proto_message *m, int sock)
{
    if(proto_get_line_count(m) < 1)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return;
    }
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    char *line = proto_get_str(m, 0);
    printf("Message from %s: %s\n", login, line);
    chat_new_message("r", login, line);
}

void message_send(char kind, struct timeval time, const char *login, const char *body, int sock)
{
    if(kind == 'r' || kind == 'h')
    {
        struct proto_message *p = proto_create(kind, 3);
        proto_set_timeval(p, 0, &time);
        proto_set_str(p, 1, login);
        proto_set_str(p, 2, body);
        _send(p, sock);
    }
    if(kind == 'm')
    {
        struct proto_message *p = proto_create('m', 2);
        proto_set_timeval(p, 0, &time);
        proto_set_str(p, 1, body);
        _send(p, sock);
    }
    if(kind == 'k')
    {
        struct proto_message *p = proto_create('k', 1);
        proto_set_str(p, 0, body);
        _send(p, sock);
        shutdown(sock, 2);
    }
}

void message_history(char *login, struct proto_message *m, int sock)
{
    if(proto_get_len(m, 0) != 4)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return;
    }
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    int cnt = proto_get_int(m, 0);
    chat_send_history(cnt, sock);
    printf("Sent %d old messages to %s\n", cnt, login);
}

void message_list(char *login, int sock)
{
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    struct chat_user_list *s, *t;
    int len = chat_get_user_list(&s);
    t = s;
    struct proto_message *p = proto_create('l', 2 * len);
    for(int i=0;s;++i,s=s->next)
    {
        proto_set_int(p, 2 * i, (int) s->uid);
        proto_set_str(p, 2 * i + 1, s->login);
    }
    chat_free_user_list(t);
    _send(p, sock);
    printf("Sent user list to %s\n", login);
}

void message_kick(char *login, struct proto_message *m, int sock)
{
    if(proto_get_line_count(m) < 2 || proto_get_len(m, 0) != 4)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return;
    }
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    if(strcmp(login, "root"))
    {
        printf("%s tried to kick\n", login);
        message_send_status(STATUS_ACCESS_DENIED, sock);
        return;
    }
    int uid = proto_get_int(m, 0);
    char *reason = proto_get_str(m, 1);
    if(chat_kick_user(uid, reason))
    {
        printf("Kicked user %d: %s\n", uid, reason);
    }
    else
    {
        message_send_status(STATUS_NO_SUCH_USER, sock);
        printf("Tried to kick %d: %s\n", uid, reason);
    }
}
