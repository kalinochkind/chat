#include <sys/time.h>

char *message_connect(const char *ip, int port);
char *message_do_login(const char *login, const char *password);
void message_do_logout();
void message_send(const char *msg);
int message_receive(struct timeval *time, char **author, char **body);
void message_request_history(int cnt);
void message_request_list();
void message_kick_user(int uid, const char *reason);
void message_disconnect();
