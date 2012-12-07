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

#ifndef __FILE_SEARCH_INDEX_H__
#define	__FILE_SEARCH_INDEX_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FILE_SEARCH_INDEX_TYPE            (file_search_index_get_type ())
#define FILE_SEARCH_INDEX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILE_SEARCH_INDEX_TYPE, FileSearchIndex))
#define FILE_SEARCH_INDEX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FILE_SEARCH_INDEX_TYPE, FileSearchIndexClass))
#define IS_FILE_SEARCH_INDEX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILE_SEARCH_INDEX_TYPE))
#define IS_FILE_SEARCH_INDEX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FILE_SEARCH_INDEX_TYPE))

typedef struct _FileSearchIndex FileSearchIndex;
typedef struct _FileSearchIndexClass FileSearchIndexClass;

struct _FileSearchIndex
{
  GObject parent_instance;
};

struct _FileSearchIndexClass
{
  GObjectClass parent_class;
};

GType file_search_index_get_type (void) G_GNUC_CONST;

FileSearchIndex*  file_search_index_new              (void);

const gchar*      file_search_index_get_project_key  (FileSearchIndex *index);
void              file_search_index_set_project_key  (FileSearchIndex *index,
                                                      const gchar     *project_key);
const gchar*      file_search_index_get_file_name    (FileSearchIndex *index);
void              file_search_index_set_file_name    (FileSearchIndex *index,
                                                      const gchar     *file_name);
const gchar*      file_search_index_get_file_path    (FileSearchIndex *index);
void              file_search_index_set_file_path    (FileSearchIndex *index,
                                                      const gchar     *file_path);

G_END_DECLS

#endif /* __FILE_SEARCH_INDEX_H__ */
