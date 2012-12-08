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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesearch-dialog.h"
#include "filesearch-index.h"

static void file_search_dialog_class_init  (FileSearchDialogClass *klass);
static void file_search_dialog_init        (FileSearchDialog      *dialog);
static void file_search_dialog_finalize    (FileSearchDialog      *dialog);

static void search_action                  (FileSearchDialog      *dialog);
static void run_dialog                     (FileSearchDialog      *dialog);
static gboolean key_release_action         (FileSearchDialog      *dialog,
                                            GdkEventKey           *event);
static GList* get_indexes                  (FileSearchDialog      *dialog, 
                                            const gchar           *input);
static FileSearchIndex* get_index          (gchar                 *line);
static void render_indexes                 (FileSearchDialog      *dialog, 
                                            GList                 *indexes);
static void row_activated_action           (FileSearchDialog      *dialog,
                                            GtkTreePath           *path,
                                            GtkTreeViewColumn     *column);
static gboolean filter_callback            (GtkTreeModel          *model,
                                            GtkTreeIter           *iter,
                                            FileSearchDialog      *dialog);
static gboolean contains_text              (const gchar           *value, 
                                            const gchar           *text);

#define FILE_SEARCH_DIALOG_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), FILE_SEARCH_DIALOG_TYPE, FileSearchDialogPrivate))

typedef struct _FileSearchDialogPrivate FileSearchDialogPrivate;

struct _FileSearchDialogPrivate
{
  CodeSlayer   *codeslayer;
  GtkWidget    *dialog;
  GtkWidget    *entry;
  GtkWidget    *tree;
  GtkListStore *store;
  GtkTreeModel *filter;  
};

enum
{
  FILE_NAME = 0,
  FILE_PATH,
  PROJECT_KEY,
  COLUMNS
};

G_DEFINE_TYPE (FileSearchDialog, file_search_dialog, G_TYPE_OBJECT)

static void 
file_search_dialog_class_init (FileSearchDialogClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = (GObjectFinalizeFunc) file_search_dialog_finalize;
  g_type_class_add_private (klass, sizeof (FileSearchDialogPrivate));
}

static void
file_search_dialog_init (FileSearchDialog *dialog)
{
  FileSearchDialogPrivate *priv;
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  priv->dialog = NULL;
  priv->filter = NULL;
}

static void
file_search_dialog_finalize (FileSearchDialog *dialog)
{
  FileSearchDialogPrivate *priv;
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  if (priv->dialog != NULL)
    gtk_widget_destroy (priv->dialog);

  G_OBJECT_CLASS (file_search_dialog_parent_class)-> finalize (G_OBJECT (dialog));
}

FileSearchDialog*
file_search_dialog_new (CodeSlayer *codeslayer,
                 GtkWidget  *menu)
{
  FileSearchDialogPrivate *priv;
  FileSearchDialog *dialog;

  dialog = FILE_SEARCH_DIALOG (g_object_new (file_search_dialog_get_type (), NULL));
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  priv->codeslayer = codeslayer;
  
  g_signal_connect_swapped (G_OBJECT (menu), "search-files",
                            G_CALLBACK (search_action), dialog);

  return dialog;
}

static void
search_action (FileSearchDialog *dialog)
{
  run_dialog (dialog);
}

static void
run_dialog (FileSearchDialog *dialog)
{
  FileSearchDialogPrivate *priv;
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  if (priv->dialog == NULL)
    {
      GtkWidget *content_area;
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *label;
      GtkWidget *scrolled_window;
      GtkTreeViewColumn *column;
      GtkCellRenderer *renderer;

      priv->dialog = gtk_dialog_new_with_buttons ("File Search", 
                                                  codeslayer_get_toplevel_window (priv->codeslayer),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                  NULL);
      gtk_window_set_skip_taskbar_hint (GTK_WINDOW (priv->dialog), TRUE);
      gtk_window_set_skip_pager_hint (GTK_WINDOW (priv->dialog), TRUE);

      content_area = gtk_dialog_get_content_area (GTK_DIALOG (priv->dialog));
      
      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
      gtk_box_set_homogeneous (GTK_BOX (vbox), FALSE);
      
      /* the completion box */
      
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);
      
      label = gtk_label_new ("File: ");
      priv->entry = gtk_entry_new ();
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
      gtk_box_pack_start (GTK_BOX (hbox), priv->entry, TRUE, TRUE, 2);
      
      /* the tree view */   
         
      priv->store = gtk_list_store_new (COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
      priv->tree =  gtk_tree_view_new ();
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->tree), FALSE);
      gtk_tree_view_set_enable_search (GTK_TREE_VIEW (priv->tree), FALSE);
      
      priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store), NULL);
      gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter),
                                              (GtkTreeModelFilterVisibleFunc) filter_callback,
                                               dialog, NULL);
      
      gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree), GTK_TREE_MODEL (priv->filter));
      g_object_unref (priv->store);      

      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      renderer = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (column, renderer, FALSE);
      gtk_tree_view_column_add_attribute (column, renderer, "text", FILE_NAME);
      gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree), column);
      
      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      renderer = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (column, renderer, FALSE);
      gtk_tree_view_column_add_attribute (column, renderer, "text", FILE_PATH);
      gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree), column);
      
      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->tree));
      
      /* hook up the signals */
      
      g_signal_connect_swapped (G_OBJECT (priv->entry), "key-release-event",
                                G_CALLBACK (key_release_action), dialog);
                                
      g_signal_connect_swapped (G_OBJECT (priv->tree), "row-activated",
                                G_CALLBACK (row_activated_action), dialog);                                
      
      /* render everything */
      
      gtk_widget_set_size_request (content_area, 600, 400);
      
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);

      gtk_widget_show_all (content_area);
    }
    
  gtk_widget_grab_focus (priv->entry);
  gtk_dialog_run (GTK_DIALOG (priv->dialog));
  gtk_widget_hide (priv->dialog);
}

static gboolean
key_release_action (FileSearchDialog *dialog,
                    GdkEventKey      *event)
{
  FileSearchDialogPrivate *priv;
  gint text_length;
  
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  text_length = gtk_entry_get_text_length (GTK_ENTRY (priv->entry));
  
  if (text_length == 0)
    {
      gtk_list_store_clear (priv->store);
    }
  else if (text_length >= 2) 
    {
      if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->filter), NULL) > 0)
        {
          gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
        }
      else
        {
          gchar *input;
          GList *indexes;
          const gchar *text;
          
          gtk_list_store_clear (priv->store);

          text = gtk_entry_get_text (GTK_ENTRY (priv->entry));

          indexes = get_indexes (dialog, text);
          
          if (indexes != NULL)
            {        
              render_indexes (dialog, indexes);
              g_list_foreach (indexes, (GFunc) g_object_unref, NULL);
              g_list_free (indexes);
            }      
        }
    }

  return FALSE;
}

static GList*
get_indexes (FileSearchDialog *dialog, 
             const gchar      *text)
{
  FileSearchDialogPrivate *priv;
  GList *results = NULL;

  GIOChannel *channel;
  GError *error = NULL;
  gchar *line;
  gsize len;

  CodeSlayerGroup *group;
  gchar *group_folder_path;
  gchar *group_indexes_file;
  
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);

  group = codeslayer_get_active_group (priv->codeslayer);

  group_folder_path = codeslayer_get_active_group_folder_path (priv->codeslayer);
  group_indexes_file = g_strconcat (group_folder_path, G_DIR_SEPARATOR_S, "filesearch", NULL);
  
  channel = g_io_channel_new_file (group_indexes_file, "r", &error);
  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
    {
      g_warning ("Error reading file search file: %s\n", error->message);
      g_error_free (error);
    }
  
  while (g_io_channel_read_line (channel, &line, &len, NULL, &error) != G_IO_STATUS_EOF)
    {
      if (contains_text (line, text))
        {
          FileSearchIndex *index = get_index (line);
          results = g_list_prepend (results, index);                  
        }
      g_free (line);
    }
    
  g_free (group_folder_path);
  g_free (group_indexes_file);
  
  return results;
}

static FileSearchIndex*
get_index (gchar *line)
{
  FileSearchIndex *index = NULL;
  gchar **split;
  gchar **tmp;
  
  if (!codeslayer_utils_has_text (line))
    return NULL;
  
  split = g_strsplit (line, "\t", -1);
  if (split != NULL)
    {
      gchar *file_name;  
      gchar *file_path;  
      gchar *project_key;
      
      tmp = split;

      file_name = *tmp;
      file_path = *++tmp;
      project_key = *++tmp;
      
      if (file_name != NULL && 
          file_path != NULL && 
          project_key != NULL)
        {
          index = file_search_index_new ();
          file_search_index_set_file_name (index, file_name);
          file_search_index_set_file_path (index, file_path);
          file_search_index_set_project_key (index, project_key);
        }

      g_strfreev (split);
    }
    
  return index;
}

static void
render_indexes (FileSearchDialog *dialog, 
                GList            *indexes)
{
  FileSearchDialogPrivate *priv;
  GtkTreeIter iter;

  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  while (indexes != NULL)
    {
      FileSearchIndex *index = indexes->data;      
      gtk_list_store_append (priv->store, &iter);
      gtk_list_store_set (priv->store, &iter, 
                          FILE_NAME, file_search_index_get_file_name (index), 
                          FILE_PATH, file_search_index_get_file_path (index), 
                          PROJECT_KEY, file_search_index_get_project_key (index), 
                          -1);
                          
      indexes = g_list_next (indexes);
    }
}

static gboolean
filter_callback (GtkTreeModel     *model,
                 GtkTreeIter      *iter,
                 FileSearchDialog *dialog)
{  
  FileSearchDialogPrivate *priv;
  const gchar *text = NULL;
  gchar *value = NULL;

  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  text = gtk_entry_get_text (GTK_ENTRY (priv->entry));

  if (text == NULL)
    return FALSE;
  
  gtk_tree_model_get (model, iter, FILE_NAME, &value, -1);
  
  if (value == NULL)
    return FALSE;

  if (contains_text (value, text))
    {
      g_free (value);    
      return TRUE;
    }

  g_free (value);
  
  return FALSE;
}

static gboolean
contains_text (const gchar *value, 
               const gchar *text)
{
  gchar *result;    
  result = g_strstr_len (value, -1, text);

  if (result != NULL)
    {
      return TRUE;
    }
  
  return FALSE;
}              

static void
row_activated_action (FileSearchDialog  *dialog,
                      GtkTreePath       *path,
                      GtkTreeViewColumn *column)
{
  FileSearchDialogPrivate *priv;
  GtkTreeSelection *tree_selection;
  GtkTreeModel *tree_model;
  GList *selected_rows = NULL;
  GList *tmp = NULL;  
  
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->tree));
  selected_rows = gtk_tree_selection_get_selected_rows (tree_selection, &tree_model);

  tmp = selected_rows;
  
  if (tmp != NULL)
    {
      GtkTreeIter treeiter;
      CodeSlayerProject *project;
      CodeSlayerDocument *document;
      gchar *file_path; 
      GtkTreePath *tree_path = tmp->data;
      
      gtk_tree_model_get_iter (tree_model, &treeiter, tree_path);
      gtk_tree_model_get (GTK_TREE_MODEL (priv->filter), &treeiter, FILE_PATH, &file_path, -1);
      
      document = codeslayer_document_new ();
      project = codeslayer_get_project_by_file_path (priv->codeslayer, file_path);
      codeslayer_document_set_file_path (document, file_path);
      codeslayer_document_set_project (document, project);
      
      codeslayer_select_editor (priv->codeslayer, document);
      gtk_widget_hide (priv->dialog);
      
      g_object_unref (document);
      g_free (file_path);
      gtk_tree_path_free (tree_path);
    }

  g_list_free (selected_rows);
}                     
