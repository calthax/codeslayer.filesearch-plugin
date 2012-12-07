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

#ifndef __FILE_SEARCH_ENGINE_H__
#define	__FILE_SEARCH_ENGINE_H__

#include <gtk/gtk.h>
#include <codeslayer/codeslayer.h>

G_BEGIN_DECLS

#define FILE_SEARCH_ENGINE_TYPE            (file_search_engine_get_type ())
#define FILE_SEARCH_ENGINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILE_SEARCH_ENGINE_TYPE, FileSearchEngine))
#define FILE_SEARCH_ENGINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FILE_SEARCH_ENGINE_TYPE, FileSearchEngineClass))
#define IS_FILE_SEARCH_ENGINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILE_SEARCH_ENGINE_TYPE))
#define IS_FILE_SEARCH_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FILE_SEARCH_ENGINE_TYPE))

typedef struct _FileSearchEngine FileSearchEngine;
typedef struct _FileSearchEngineClass FileSearchEngineClass;

struct _FileSearchEngine
{
  GObject parent_instance;
};

struct _FileSearchEngineClass
{
  GObjectClass parent_class;
};

GType file_search_engine_get_type (void) G_GNUC_CONST;

FileSearchEngine*  file_search_engine_new  (CodeSlayer *codeslayer, 
                                     GtkWidget  *menu);

G_END_DECLS

#endif /* _FILE_SEARCH_ENGINE_H */
