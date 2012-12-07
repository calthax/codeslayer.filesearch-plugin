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

static void add_menu_items          (FileSearchMenu      *menu,
                                     GtkWidget        *submenu,
                                     GtkAccelGroup    *accel_group);
static void index_files_action      (FileSearchMenu      *menu);
static void search_files_action     (FileSearchMenu      *menu);
                                        
enum
{
  INDEX_FILES,
  SEARCH_FILES,
  LAST_SIGNAL
};

static guint file_search_menu_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (FileSearchMenu, file_search_menu, GTK_TYPE_MENU_ITEM)

static void
file_search_menu_class_init (FileSearchMenuClass *klass)
{
  file_search_menu_signals[INDEX_FILES] =
    g_signal_new ("index-files", 
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  G_STRUCT_OFFSET (FileSearchMenuClass, index_files),
                  NULL, NULL, 
                  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

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
  gtk_menu_item_set_label (GTK_MENU_ITEM (menu), "File Search");
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
  GtkWidget *submenu;
  
  menu = g_object_new (file_search_menu_get_type (), NULL);
  
  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu), submenu);

  add_menu_items (FILE_SEARCH_MENU (menu), submenu, accel_group);

  return menu;
}

static void
add_menu_items (FileSearchMenu   *menu,
                GtkWidget     *submenu,
                GtkAccelGroup *accel_group)
{
  GtkWidget *index_files_item;
  GtkWidget *search_files_item;

  index_files_item = codeslayer_menu_item_new_with_label ("Index Files");
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), index_files_item);

  search_files_item = codeslayer_menu_item_new_with_label ("Search Files");
  gtk_widget_add_accelerator (search_files_item, "activate", 
                              accel_group, GDK_KEY_I, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);  
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), search_files_item);

  g_signal_connect_swapped (G_OBJECT (index_files_item), "activate", 
                            G_CALLBACK (index_files_action), menu);
   
  g_signal_connect_swapped (G_OBJECT (search_files_item), "activate", 
                            G_CALLBACK (search_files_action), menu);
}

static void 
index_files_action (FileSearchMenu *menu) 
{
  g_signal_emit_by_name ((gpointer) menu, "index-files");
}

static void 
search_files_action (FileSearchMenu *menu) 
{
  g_signal_emit_by_name ((gpointer) menu, "search-files");
}
