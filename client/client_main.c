#include <stdio.h>
#include <gtk/gtk.h>

#include "login.h"

int main(int argc,char *argv[]){

    gtk_init(&argc, &argv);
    init_login_window();
    gtk_widget_show_all(loginWindow);
    gtk_main();
    return 0;
}
