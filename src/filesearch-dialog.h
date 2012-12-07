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

#ifndef __FILE_SEARCH_DIALOG_H__
#define	__FILE_SEARCH_DIALOG_H__

#include <gtk/gtk.h>
#include <codeslayer/codeslayer.h>

G_BEGIN_DECLS

#define FILE_SEARCH_DIALOG_TYPE            (file_search_dialog_get_type ())
#define FILE_SEARCH_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILE_SEARCH_DIALOG_TYPE, FileSearchDialog))
#define FILE_SEARCH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FILE_SEARCH_DIALOG_TYPE, FileSearchDialogClass))
#define IS_FILE_SEARCH_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILE_SEARCH_DIALOG_TYPE))
#define IS_FILE_SEARCH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FILE_SEARCH_DIALOG_TYPE))

typedef struct _FileSearchDialog FileSearchDialog;
typedef struct _FileSearchDialogClass FileSearchDialogClass;

struct _FileSearchDialog
{
  GObject parent_instance;
};

struct _FileSearchDialogClass
{
  GObjectClass parent_class;
};

GType file_search_dialog_get_type (void) G_GNUC_CONST;
     
FileSearchDialog*  file_search_dialog_new  (CodeSlayer *codeslayer,
                                            GtkWidget  *menu);
                                     
G_END_DECLS

#endif /* __FILE_SEARCH_DIALOG_H__ */
