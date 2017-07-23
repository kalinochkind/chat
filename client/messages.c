#define _GNU_SOURCE

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "proto.h"

int sock;

char message_buf[MESSAGE_BUF_SIZE];


static void _send(struct proto_message *p)
{
    char *buf;
    unsigned len = proto_encode(p, &buf);
    proto_free(p);
    send(sock, buf, len, 0);
    free(buf);
}

static int resolve_host(const char *host)
{
    int s;
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(host, "http", &hints, &result))
        return 0;
    s = ((struct sockaddr_in *) result->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(result);
    return s;

}

char *message_connect(const char *ip, int port)
{
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        return "Cannot create socket";
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(!(addr.sin_addr.s_addr = resolve_host(ip)))
    {
        return "Invalid host";
    }
    if(connect(sock, (struct sockaddr*)&addr,  sizeof(struct sockaddr)) < 0)
    {
        shutdown(sock, 2);
        return "Unable to connect";
    }
    return 0;
}

char *message_do_login(const char *login, const char *password)
{
    struct proto_message *p = proto_create('i', 2);
    proto_set_str(p, 0, login);
    proto_set_str(p, 1, password);
    _send(p);
    unsigned len = proto_recv_packet(message_buf, sock);
    if(len <= 0)
    {
        return "Connection failed";
    }
    p = proto_decode(message_buf, len);
    if(!p || proto_get_type(p) != 's' || proto_get_line_count(p) < 1 || proto_get_len(p, 0) != 4)
    {
        return "Unknown error";
    }
    int t = proto_get_int(p, 0);
    proto_free(p);
    switch(t)
    {
        case STATUS_OK:
            return 0;
        case STATUS_SIGNUP_ERROR:
            return "Signup error";
        case STATUS_AUTH_ERROR:
            return "Incorrect password";
        default:
            return "Unknown error";
    }

}

void message_do_logout()
{
    struct proto_message *p = proto_create('o', 0);
    _send(p);
    proto_recv_packet(message_buf, sock);
}

void message_send(const char *msg)
{
    struct proto_message *p = proto_create('r', 1);
    proto_set_str(p, 0, msg);
    _send(p);
}

int message_receive(struct timeval *time, char **author, char **body)
{
    int len = proto_recv_packet(message_buf, sock);
    if(len <= 0)
        return -1;
    struct proto_message *p = proto_decode(message_buf, len);
    if(!p)
        return -1;
    int tp = proto_get_type(p);
    if(tp == 'r' || tp == 'h')
    {
        proto_get_timeval(p, 0, time);
        char *s1 = proto_get_str(p, 1);
        char *s2 = proto_get_str(p, 2);
        *author = malloc(strlen(s1) + 1);
        strcpy(*author, s1);
        *body = malloc(strlen(s2) + 1);
        strcpy(*body, s2);
    }
    else if(tp == 'm')
    {
        proto_get_timeval(p, 0, time);
        char *s1 = proto_get_str(p, 1);
        *author = malloc(1);
        **author = 0;
        *body = malloc(strlen(s1) + 1);
        strcpy(*body, s1);
    }
    else if(tp == 'l')
    {
        int cnt = proto_get_line_count(p) / 2;
        int len = 20 + cnt * 50;
        *body = malloc(len);
        strcpy(*body, "User list: ");
        for(int i=0;i<cnt;++i)
        {
            int uid = proto_get_int(p, i * 2);
            char *login = proto_get_str(p, i * 2 + 1);
            if(strlen(login) > 32)
                continue;
            sprintf(message_buf, "%s%s (%d)", (i?", ":""), login, uid);
            strcat(*body, message_buf);
        }
        *author = malloc(1);
        **author = 0;
        time->tv_sec = 0;
    }
    else if(tp == 's' && proto_get_line_count(p) == 1 && proto_get_int(p, 0) == STATUS_ACCESS_DENIED)
    {
        *author = 0;
        *body = "Access denied";
    }
    else if(tp == 's' && proto_get_line_count(p) == 1 && proto_get_int(p, 0) == STATUS_NO_SUCH_USER)
    {
        *author = 0;
        *body = "No such user";
    }
    else if(tp == 'k')
    {
        char *reason = proto_get_str(p, 0);
        *author = malloc(1);
        **author = 0;
        *body = malloc(300);
        strcpy(*body, "You have been kicked (reason: ");
        strncat(*body, reason, 256);
        strcat(*body, ")");
        time->tv_sec = 0;
    }
    else
    {
        tp = 0;
    }
    proto_free(p);
    return tp;
}

void message_request_history(int cnt)
{
    struct proto_message *p = proto_create('h', 1);
    proto_set_int(p, 0, cnt);
    _send(p);
}

void message_request_list()
{
    struct proto_message *p = proto_create('l', 0);
    _send(p);
}

void message_kick_user(int uid, const char *reason)
{
    struct proto_message *p = proto_create('k', 2);
    proto_set_int(p, 0, uid);
    proto_set_str(p, 1, reason);
    _send(p);
}

void message_disconnect()
{
    shutdown(sock, 2);
}
