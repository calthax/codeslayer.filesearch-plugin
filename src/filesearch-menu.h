/*
 * Copyright (C) 2010 - Jeff Johnston
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __FILE_SEARCH_MENU_H__
#define	__FILE_SEARCH_MENU_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FILE_SEARCH_MENU_TYPE            (file_search_menu_get_type ())
#define FILE_SEARCH_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILE_SEARCH_MENU_TYPE, FileSearchMenu))
#define FILE_SEARCH_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FILE_SEARCH_MENU_TYPE, FileSearchMenuClass))
#define IS_FILE_SEARCH_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILE_SEARCH_MENU_TYPE))
#define IS_FILE_SEARCH_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FILE_SEARCH_MENU_TYPE))

typedef struct _FileSearchMenu FileSearchMenu;
typedef struct _FileSearchMenuClass FileSearchMenuClass;

struct _FileSearchMenu
{
  GtkMenuItem parent_instance;
};

struct _FileSearchMenuClass
{
  GtkMenuItemClass parent_class;

  void (*search_files) (FileSearchMenu *menu);
};

GType file_search_menu_get_type (void) G_GNUC_CONST;
  
GtkWidget*  file_search_menu_new  (GtkAccelGroup *accel_group);
                                            
G_END_DECLS

#endif /* __FILE_SEARCH_MENU_H__ */
