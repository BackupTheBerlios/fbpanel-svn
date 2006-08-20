#include <stdio.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <string.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

int
main(int argc, char *argv[], char *env[])
{
    GtkWidget *topgwin;
    
    setlocale(LC_CTYPE, "");
    gtk_set_locale();
    gtk_init(&argc, &argv);
    XSetLocaleModifiers("");

    topgwin =  gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(topgwin), 0);
    //gtk_window_set_resizable(GTK_WINDOW(topgwin), FALSE);
    gtk_window_set_wmclass(GTK_WINDOW(topgwin), "panel", "fbpanel");
    gtk_window_set_title(GTK_WINDOW(topgwin), "panel");
    //gtk_window_set_position(GTK_WINDOW(topgwin), GTK_WIN_POS_NONE);
    //gtk_window_set_decorated(GTK_WINDOW(topgwin), FALSE);
    gtk_widget_realize(topgwin);
    //gdk_window_set_decorations(p->topgwin->window, 0);
    gtk_widget_set_app_paintable(topgwin, TRUE);
    
    /* window mapping point */
    gtk_widget_show_all(topgwin);
    gtk_window_resize(GTK_WINDOW(topgwin), 30, 30);
    gtk_main();  
    return 0;
}
