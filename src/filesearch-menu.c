/*
 * Copyright (C) 2010 - Jeff Johnston
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.remove_group_item
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gdk/gdkkeysyms.h>
#include <codeslayer/codeslayer.h>
#include "filesearch-menu.h"

static void file_search_menu_class_init (FileSearchMenuClass *klass);
static void file_search_menu_init       (FileSearchMenu      *menu);
static void file_search_menu_finalize   (FileSearchMenu      *menu);

static void search_files_action         (FileSearchMenu      *menu);
                                        
enum
{
  SEARCH_FILES,
  LAST_SIGNAL
};

static guint file_search_menu_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (FileSearchMenu, file_search_menu, GTK_TYPE_MENU_ITEM)

static void
file_search_menu_class_init (FileSearchMenuClass *klass)
{
  file_search_menu_signals[SEARCH_FILES] =
    g_signal_new ("search-files", 
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  G_STRUCT_OFFSET (FileSearchMenuClass, search_files),
                  NULL, NULL, 
                  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  G_OBJECT_CLASS (klass)->finalize = (GObjectFinalizeFunc) file_search_menu_finalize;
}

static void
file_search_menu_init (FileSearchMenu *menu)
{
  gtk_menu_item_set_label (GTK_MENU_ITEM (menu), "Search Files");
}

static void
file_search_menu_finalize (FileSearchMenu *menu)
{
  G_OBJECT_CLASS (file_search_menu_parent_class)->finalize (G_OBJECT (menu));
}

GtkWidget*
file_search_menu_new (GtkAccelGroup *accel_group)
{
  GtkWidget *menu;
  
  menu = g_object_new (file_search_menu_get_type (), NULL);
  
  gtk_widget_add_accelerator (menu, "activate", 
                              accel_group, GDK_KEY_I, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);  
  
  g_signal_connect_swapped (G_OBJECT (menu), "activate", 
                            G_CALLBACK (search_files_action), menu);

  return menu;
}

static void 
search_files_action (FileSearchMenu *menu) 
{
  g_signal_emit_by_name ((gpointer) menu, "search-files");
}
