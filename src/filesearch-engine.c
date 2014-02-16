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

#include "filesearch-engine.h"
#include "filesearch-dialog.h"
#include "filesearch-index.h"

static void file_search_engine_class_init  (FileSearchEngineClass *klass);
static void file_search_engine_init        (FileSearchEngine      *engine);
static void file_search_engine_finalize    (FileSearchEngine      *engine);

static void execute                        (CodeSlayer            *codeslayer);
static GList* get_indexes                  (CodeSlayer            *codeslayer);
static void get_project_indexes            (CodeSlayerProject     *project, 
                                            GFile                 *file, 
                                            GList                 **indexes, 
                                            GList                 *exclude_types,
                                            GList                 *exclude_dirs);
static void write_indexes                  (CodeSlayer            *codeslayer, 
                                            GIOChannel            *channel,
                                            GList                 *indexes);                                        
                            
#define FILE_SEARCH_ENGINE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), FILE_SEARCH_ENGINE_TYPE, FileSearchEnginePrivate))

typedef struct _FileSearchEnginePrivate FileSearchEnginePrivate;

struct _FileSearchEnginePrivate
{
  CodeSlayer *codeslayer;
  FileSearchDialog *dialog;
  gulong projects_changed_id;
};

G_DEFINE_TYPE (FileSearchEngine, file_search_engine, G_TYPE_OBJECT)

static void
file_search_engine_class_init (FileSearchEngineClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = (GObjectFinalizeFunc) file_search_engine_finalize;
  g_type_class_add_private (klass, sizeof (FileSearchEnginePrivate));
}

static void
file_search_engine_init (FileSearchEngine *engine) 
{
}

static void
file_search_engine_finalize (FileSearchEngine *engine)
{
  FileSearchEnginePrivate *priv;
  priv = FILE_SEARCH_ENGINE_GET_PRIVATE (engine);
  g_object_unref (priv->dialog);
  g_signal_handler_disconnect (priv->codeslayer, priv->projects_changed_id);
  G_OBJECT_CLASS (file_search_engine_parent_class)->finalize (G_OBJECT(engine));
}

FileSearchEngine*
file_search_engine_new (CodeSlayer *codeslayer, 
                        GtkWidget  *menu)
{
  FileSearchEnginePrivate *priv;
  FileSearchEngine *engine;

  engine = FILE_SEARCH_ENGINE (g_object_new (file_search_engine_get_type (), NULL));
  priv = FILE_SEARCH_ENGINE_GET_PRIVATE (engine);

  priv->codeslayer = codeslayer;
  
  priv->dialog = file_search_dialog_new (codeslayer, menu);
  
  priv->projects_changed_id = g_signal_connect_swapped (G_OBJECT (codeslayer), "projects-changed",
                                                        G_CALLBACK (file_search_engine_index_files), engine);

  return engine;
}

void
file_search_engine_index_files (FileSearchEngine *engine)
{
  FileSearchEnginePrivate *priv;
  priv = FILE_SEARCH_ENGINE_GET_PRIVATE (engine);
  g_thread_new ("index files", (GThreadFunc) execute, priv->codeslayer); 
}

static void
execute (CodeSlayer *codeslayer)
{
  GList *indexes;
  gchar *profile_folder_path;
  gchar *profile_indexes_file;
  GIOChannel *channel;
  GError *error = NULL;

  profile_folder_path = codeslayer_get_profile_config_folder_path (codeslayer);
  profile_indexes_file = g_strconcat (profile_folder_path, G_DIR_SEPARATOR_S, "filesearch", NULL);
  
  channel = g_io_channel_new_file (profile_indexes_file, "w", &error);
  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
    {
      g_warning ("Error creating file search file: %s\n", error->message);
      g_error_free (error);
    }

  indexes = get_indexes (codeslayer);
  if (indexes != NULL)
    {
      write_indexes (codeslayer, channel, indexes);
      g_io_channel_shutdown (channel, TRUE, NULL);
      g_list_foreach (indexes, (GFunc) g_object_unref, NULL);
      g_list_free (indexes);
      g_io_channel_unref (channel);
    }
    
  g_free (profile_folder_path);
  g_free (profile_indexes_file);
}

static GList*
get_indexes (CodeSlayer *codeslayer)
{
  GList *results = NULL;
  GList *projects;
  
  CodeSlayerRegistry *registry;
  gchar *exclude_types_str;
  gchar *exclude_dirs_str;
  GList *exclude_types = NULL;
  GList *exclude_dirs = NULL;
  
  registry = codeslayer_get_registry (codeslayer);
  
  exclude_types_str = codeslayer_registry_get_string (registry,
                                                      CODESLAYER_REGISTRY_PROJECTS_EXCLUDE_TYPES);
  exclude_dirs_str = codeslayer_registry_get_string (registry,
                                                     CODESLAYER_REGISTRY_PROJECTS_EXCLUDE_DIRS);
  exclude_types = codeslayer_utils_string_to_list (exclude_types_str);
  exclude_dirs = codeslayer_utils_string_to_list (exclude_dirs_str);
  
  projects = codeslayer_get_projects (codeslayer);
  while (projects != NULL)
    {
      CodeSlayerProject *project = projects->data;
      GList *indexes = NULL;
      GFile *file;
      const gchar *folder_path;
      
      folder_path = codeslayer_project_get_folder_path (project);
      file = g_file_new_for_path (folder_path);
      
      get_project_indexes (project, file, &indexes, exclude_types, exclude_dirs);
      if (indexes != NULL)
        results = g_list_concat (results, indexes);
        
      g_object_unref (file);

      projects = g_list_next (projects);
    }
    
  g_free (exclude_types_str);
  g_free (exclude_dirs_str);
  if (exclude_types)
    {
      g_list_foreach (exclude_types, (GFunc) g_free, NULL);
      g_list_free (exclude_types);
    }
  if (exclude_dirs)
    {
      g_list_foreach (exclude_dirs, (GFunc) g_free, NULL);
      g_list_free (exclude_dirs);
    }    
    
  return results;    
}

static void
get_project_indexes (CodeSlayerProject *project, 
                     GFile             *file,
                     GList             **indexes, 
                     GList             *exclude_types,
                     GList             *exclude_dirs)
{
  GFileEnumerator *enumerator;
  
  enumerator = g_file_enumerate_children (file, "standard::*",
                                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, 
                                          NULL, NULL);
                                                                  
  if (enumerator != NULL)
    {
      GFileInfo *file_info;
      while ((file_info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL)
        {
          GFile *child;
        
          const char *file_name = g_file_info_get_name (file_info);

          child = g_file_get_child (file, file_name);

          if (g_file_info_get_file_type (file_info) == G_FILE_TYPE_DIRECTORY)
            {
              if (!codeslayer_utils_contains_element (exclude_dirs, file_name))
                get_project_indexes (project, child, indexes, exclude_types, exclude_dirs);            
            }
          else
            {
              if (!codeslayer_utils_contains_element_with_suffix (exclude_types, file_name))
                {
                  FileSearchIndex *index;
                  gchar *file_path;
                  file_path = g_file_get_path (child);
                  
                  index = file_search_index_new ();
                  file_search_index_set_file_name (index, file_name);
                  file_search_index_set_file_path (index, file_path);
                  
                  g_free (file_path);
                  
                  *indexes = g_list_prepend (*indexes, index);
                }
            }

          g_object_unref(child);
          g_object_unref (file_info);
        }
      g_object_unref (enumerator);
    }
}

static void
write_indexes (CodeSlayer *codeslayer,
               GIOChannel *channel, 
               GList      *indexes)
{
  while (indexes != NULL)
    {
      FileSearchIndex *index = indexes->data;
      GIOStatus status;
      gchar *line;
      
      line = g_strdup_printf ("%s\t%s\t%s\n", 
                              file_search_index_get_file_name (index), 
                              file_search_index_get_file_path (index), 
                              file_search_index_get_project_key (index));

      status = g_io_channel_write_chars (channel, line, -1, NULL, NULL);
      
      g_free (line);
      
      if (status != G_IO_STATUS_NORMAL)
        g_warning ("Error writing to file search file.");

      indexes = g_list_next (indexes);
    }
    
  g_io_channel_flush (channel, NULL);
}
