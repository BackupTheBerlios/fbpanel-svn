/*
 * fb-background-monitor.c:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ev.h"
#include "misc.h"

#include "fbwin.h"

//#define DEBUG
#include "dbg.h"



static struct {
    int add_win;
    int win_name;
} update;



struct _FbEvClass {
    GObjectClass   parent_class;
    void *dummy;
    void (*current_desktop)(FbEv *ev, gpointer p);
    void (*active_window)(FbEv *ev, gpointer p);
    void (*number_of_desktops)(FbEv *ev, gpointer p);
    void (*client_list)(FbEv *ev, gpointer p);
    void (*client_list_stacking)(FbEv *ev, gpointer p);
};

struct _FbEv {
    GObject    parent_instance;

    int current_desktop;
    int number_of_desktops;
    Window active_window;
    int client_num;
    Window *client_list;
    Window *client_list_stacking;
    GHashTable  *client_ht;
    
    Window   xroot;
    Atom     id;
    GC       gc;
    Display *dpy;
    Pixmap   pixmap;
};

static void fb_ev_class_init (FbEvClass *klass);
static void fb_ev_init (FbEv *monitor);
static void fb_ev_finalize (GObject *object);

static void ev_current_desktop(FbEv *ev, gpointer p);
static void ev_active_window(FbEv *ev, gpointer p);
static void ev_number_of_desktops(FbEv *ev, gpointer p);
static void ev_client_list(FbEv *ev, gpointer p);
static void ev_client_list_stacking(FbEv *ev, gpointer p);

static guint signals [LAST_SIGNAL] = { 0 };


GType
fb_ev_get_type (void)
{
    static GType object_type = 0;

    if (!object_type) {
        static const GTypeInfo object_info = {
            sizeof (FbEvClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) fb_ev_class_init,
            NULL,           /* class_finalize */
            NULL,           /* class_data */
            sizeof (FbEv),
            0,              /* n_preallocs */
            (GInstanceInitFunc) fb_ev_init,
        };

        object_type = g_type_register_static (
            G_TYPE_OBJECT, "FbEv", &object_info, 0);
    }

    return object_type;
}



static void
fb_ev_class_init (FbEvClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    signals [EV_CURRENT_DESKTOP] = 
        g_signal_new ("current_desktop",
              G_OBJECT_CLASS_TYPE (object_class),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (FbEvClass, current_desktop),
              NULL, NULL,
              g_cclosure_marshal_VOID__VOID,
              G_TYPE_NONE, 0);
    signals [EV_NUMBER_OF_DESKTOPS] = 
        g_signal_new ("number_of_desktops",
              G_OBJECT_CLASS_TYPE (object_class),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (FbEvClass, number_of_desktops),
              NULL, NULL,
              g_cclosure_marshal_VOID__VOID,
              G_TYPE_NONE, 0);
    signals [EV_ACTIVE_WINDOW] = 
        g_signal_new ("active_window",
              G_OBJECT_CLASS_TYPE (object_class),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (FbEvClass, active_window),
              NULL, NULL,
              g_cclosure_marshal_VOID__VOID,
              G_TYPE_NONE, 0);
    signals [EV_CLIENT_LIST_STACKING] = 
        g_signal_new ("client_list_stacking",
              G_OBJECT_CLASS_TYPE (object_class),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (FbEvClass, client_list_stacking),
              NULL, NULL,
              g_cclosure_marshal_VOID__VOID,
              G_TYPE_NONE, 0);
    signals [EV_CLIENT_LIST] = 
        g_signal_new ("client_list",
              G_OBJECT_CLASS_TYPE (object_class),
              G_SIGNAL_RUN_FIRST,
              G_STRUCT_OFFSET (FbEvClass, client_list),
              NULL, NULL,
              g_cclosure_marshal_VOID__VOID,
              G_TYPE_NONE, 0);
    signals [EV_CLIENT_LIST_WINDOW_ADDED] = 
        g_signal_new ("client_list_window_added",
              G_OBJECT_CLASS_TYPE (object_class),
              G_SIGNAL_RUN_FIRST,
              0,//G_STRUCT_OFFSET (FbEvClass, client_list_window_added),
              NULL, NULL,
              g_cclosure_marshal_VOID__VOID,
              G_TYPE_NONE, 0);
    object_class->finalize = fb_ev_finalize;

    klass->current_desktop = ev_current_desktop;
    klass->active_window = ev_active_window;
    klass->number_of_desktops = ev_number_of_desktops;
    klass->client_list = ev_client_list;
    klass->client_list_stacking = ev_client_list_stacking;
}

static void
fb_ev_init (FbEv *ev)
{
    ev->number_of_desktops = -1;
    ev->current_desktop = -1;
    ev->active_window = None;
    ev->client_list_stacking = NULL;
    ev->client_list = NULL;
    ev->client_ht = g_hash_table_new(g_int_hash, g_int_equal);
}


FbEv *
fb_ev_new()
{
    return  g_object_new (FB_TYPE_EV, NULL);
}

static void
fb_ev_finalize (GObject *object)
{
    FbEv *ev;

    ev = FB_EV (object);
    g_hash_table_destroy(ev->client_ht);
    //XFreeGC(ev->dpy, ev->gc);
}

void
fb_ev_trigger(FbEv *ev, int signal)
{
    DBG("signal=%d\n", signal);
    g_assert(signal >=0 && signal < LAST_SIGNAL);
    DBG("\n");
    g_signal_emit(ev, signals [signal], 0);
}

static void
ev_current_desktop(FbEv *ev, gpointer p)
{
    ENTER;
    ev->current_desktop = -1;
    RET();
}

static void
ev_active_window(FbEv *ev, gpointer p)
{
    ENTER;
    ev->active_window = None;
    RET();
}

static void
ev_number_of_desktops(FbEv *ev, gpointer p)
{
    ENTER;
    ev->number_of_desktops = -1;
    RET();
}
static void
fb_win_update(FbWin *win)
{
    if (update.win_name)
        fb_win_update_name(win);
}


/* tell to remove element with zero refcount */
static gboolean
ev_remove_stale_fbwins(Window *xwin, FbWin *win)
{
    ENTER;
    g_object_unref(win);
    RET(FALSE);
}

static void
ev_client_list(FbEv *ev, gpointer p)
{
    int i;
    FbWin *win;
    Window xwin;
    
    ENTER2;    
    if (ev->client_list) 
        XFree(ev->client_list);
    ev->client_list =  get_xaproperty (GDK_ROOT_WINDOW(), a_NET_CLIENT_LIST, XA_WINDOW, &ev->client_num);
    if (!ev->client_list)
        RET();    
    if (!update.add_win)
        RET();
    for (i = 0; i < ev->client_num; i++) {
        xwin = ev->client_list[i];
        if (!(win = g_hash_table_lookup(ev->client_ht, &xwin))) {
            win = fb_win_new(xwin);
            fb_win_update(win);
            g_hash_table_insert(ev->client_ht, &xwin, win);
            DBG2("adding %x\n", xwin);
            g_signal_emit(ev, signals [EV_CLIENT_LIST_WINDOW_ADDED], 0, win);
        }
        g_object_ref(win);
    }
    
    /* remove windows that arn't in the NET_CLIENT_LIST anymore */
    g_hash_table_foreach_remove(ev->client_ht, (GHRFunc) ev_remove_stale_fbwins, NULL);
}

static void
ev_client_list_stacking(FbEv *ev, gpointer p)
{
    ENTER;
    if (ev->client_list_stacking) {
        XFree(ev->client_list_stacking);
        ev->client_list_stacking = NULL;
    }
    RET();
}

int
fb_ev_current_desktop(FbEv *ev)
{
    ENTER;
    if (ev->current_desktop == -1) {
        guint32 *data;

        data = get_xaproperty (GDK_ROOT_WINDOW(), a_NET_CURRENT_DESKTOP, XA_CARDINAL, 0);
        if (data) {
            ev->current_desktop = *data;
            XFree (data);
        } else
            ev->current_desktop = 0;              
    }
    RET(ev->current_desktop);
}
        
int fb_ev_number_of_desktops(FbEv *ev)
{
    ENTER;
    RET(1);
}

Window fb_ev_active_window(FbEv *ev);
Window *fb_ev_client_list(FbEv *ev);
Window *fb_ev_client_list_stacking(FbEv *ev);
