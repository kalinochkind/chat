#define _GNU_SOURCE

#include "chat.h"
#include "messages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

GtkWidget *chatWindow;
GtkWidget *sendEntry, *sendButton;
GtkWidget *statusLabel;
GtkWidget *messagesTreeView;
GtkAdjustment *vAdjust;
GtkScrolledWindow *scrolledWindow;
GtkListStore *messagesListStore;
pthread_t watcher;

#define REQUEST_HISTORY 10

const char *HELP_STR = "\\l: list of active users, \\k ID REASON: kick (root only)";

void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void add_list_entry(const char *t, const char *a, const char *m, int sleep)
{
    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(messagesListStore), &iter);
    gtk_list_store_set(GTK_LIST_STORE(messagesListStore), &iter, 0, t, 1, a, 2, m, -1);
    if(sleep)
        sleep_ms(100);
    gtk_adjustment_set_value(vAdjust, gtk_adjustment_get_upper(vAdjust) - gtk_adjustment_get_page_size(vAdjust));
}

void do_send()
{
    if(!gtk_widget_get_sensitive(sendButton))
        return;
    gtk_label_set_text(GTK_LABEL(statusLabel), "");
    const gchar *message;
    message = gtk_entry_get_text(GTK_ENTRY(sendEntry));
    if(!message || !*message)
        return;
    if(message[0] == '\\' && message[1])
    {
        if(message[1] == 'h' && (!message[2] || message[2] == ' '))
        {
            gtk_label_set_text(GTK_LABEL(statusLabel), HELP_STR);
            gtk_entry_set_text(GTK_ENTRY(sendEntry), "");
            return;
        }
        if(message[1] == 'l' && (!message[2] || message[2] == ' '))
        {
            message_request_list();
            gtk_entry_set_text(GTK_ENTRY(sendEntry), "");
            return;
        }
        if(message[1] == 'k' && message[2] == ' ')
        {
            int uid;
            char num[16];
            int i = 3, j = 0;
            while(message[i] == ' ')
                ++i;
            while(j < 15 && message[i] != ' ' && message[i])
                num[j++] = message[i++];
            if(j == 15)
            {
                gtk_label_set_text(GTK_LABEL(statusLabel), "Usage: \\k ID REASON");
                gtk_entry_set_text(GTK_ENTRY(sendEntry), "");
                return;
            }
            num[j] = 0;
            sscanf(num, "%d", &uid);
            while(message[i] == ' ')
                ++i;
            message_kick_user(uid, &message[i]);
            gtk_entry_set_text(GTK_ENTRY(sendEntry), "");
            return;
        }
        gtk_label_set_text(GTK_LABEL(statusLabel), "Unknown command, type \\h for help");
        gtk_entry_set_text(GTK_ENTRY(sendEntry), "");
        return;
    }
    char *m = malloc(strlen(message) + 1);
    strcpy(m, message);
    gtk_entry_set_text(GTK_ENTRY(sendEntry), "");
    message_send(m);
    free(m);
}

void *watcher_thread(void *param)
{
    (void) param;
    struct timeval tv;
    struct tm *nowtm;
    char *author, *body;
    char timebuf[64];
    message_request_history(REQUEST_HISTORY);
    while(1)
    {
        int k = message_receive(&tv, &author, &body);
        if(k < 0)
        {
            gtk_label_set_text(GTK_LABEL(statusLabel), "Disconnected, please restart the client");
            gtk_widget_set_sensitive(sendButton, 0);
            break;
        }
        if(k == 0)
            continue;
        if(!author)
        {
            gtk_label_set_text(GTK_LABEL(statusLabel), body);
            continue;
        }
        if(tv.tv_sec)
        {
            nowtm = localtime(&tv.tv_sec);
            strftime(timebuf, 64, "[%d.%m.%Y %H:%M:%S]", nowtm);
        }
        else
        {
            *timebuf = 0;
        }
        add_list_entry(timebuf, author, body, k != 'h');
        free(author);
        free(body);
    }
    return param;
}

void init_chat_window(char *login)
{
    GtkBuilder *builder = gtk_builder_new_from_resource("/org/gtk/client/chat.glade");

    chatWindow = GTK_WIDGET(gtk_builder_get_object(builder,"chatWindow"));
    char buf[100] = "AKOS chat client: ";
    strcat(buf, login);
    gtk_window_set_title(GTK_WINDOW(chatWindow), buf);
    g_signal_connect(chatWindow,"destroy", G_CALLBACK(gtk_main_quit),NULL);
    sendEntry = GTK_WIDGET(gtk_builder_get_object(builder,"sendEntry"));
    sendButton = GTK_WIDGET(gtk_builder_get_object(builder,"sendButton"));
    g_signal_connect(G_OBJECT(sendEntry),"activate", G_CALLBACK(do_send),NULL);
    g_signal_connect(G_OBJECT(sendButton),"clicked", G_CALLBACK(do_send),NULL);
    statusLabel = GTK_WIDGET(gtk_builder_get_object(builder,"statusLabel"));
    messagesTreeView = GTK_WIDGET(gtk_builder_get_object(builder,"messagesTreeView"));
    messagesListStore = GTK_LIST_STORE(gtk_builder_get_object(builder,"messagesListStore"));
    scrolledWindow = GTK_SCROLLED_WINDOW(gtk_builder_get_object(builder,"scrolledWindow"));
    vAdjust = gtk_scrolled_window_get_vadjustment(scrolledWindow);
    pthread_create(&watcher, 0, watcher_thread, 0);
}
