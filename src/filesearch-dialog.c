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
static gboolean key_press_action           (FileSearchDialog      *dialog,
                                            GdkEventKey           *event);
static GList* get_indexes                  (FileSearchDialog      *dialog);
static FileSearchIndex* get_index          (gchar                 *line);
static void render_indexes                 (FileSearchDialog      *dialog, 
                                            GList                 *indexes);
static void select_tree                    (FileSearchDialog      *dialog, 
                                            GdkEventKey           *event);
static void row_activated_action           (FileSearchDialog      *dialog);
static gboolean filter_callback            (GtkTreeModel          *model,
                                            GtkTreeIter           *iter,
                                            FileSearchDialog      *dialog);
static gchar* get_globbing                 (const gchar           *entry, 
                                            gboolean               match_case);
static gint sort_compare                   (GtkTreeModel            *model, 
                                            GtkTreeIter             *a,
                                            GtkTreeIter             *b, 
                                            gpointer                 userdata);

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
  gchar        *find_globbing;
  GPatternSpec *find_pattern; 
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
  priv->find_globbing = NULL;
  priv->find_pattern = NULL;
}

static void
file_search_dialog_finalize (FileSearchDialog *dialog)
{
  FileSearchDialogPrivate *priv;
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  if (priv->dialog != NULL)
    gtk_widget_destroy (priv->dialog);

  if (priv->find_pattern != NULL)
    g_pattern_spec_free (priv->find_pattern);

  if (priv->find_globbing != NULL)
    g_free (priv->find_globbing);
  
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
      GtkTreeSortable *sortable;
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
      
      sortable = GTK_TREE_SORTABLE (priv->store);
      gtk_tree_sortable_set_sort_func (sortable, FILE_NAME, sort_compare,
                                       GINT_TO_POINTER (FILE_NAME), NULL);
      gtk_tree_sortable_set_sort_column_id (sortable, FILE_NAME, GTK_SORT_ASCENDING);

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
                                
      g_signal_connect_swapped (G_OBJECT (priv->entry), "key-press-event",
                                G_CALLBACK (key_press_action), dialog);
                                
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
key_press_action (FileSearchDialog *dialog,
                  GdkEventKey      *event)
{
  if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down)
    {
      select_tree (dialog, event);    
      return TRUE;    
    }
  if (event->keyval == GDK_KEY_Return)
    {
      row_activated_action (dialog);
      return TRUE;    
    }
    
  return FALSE;
}

static gboolean
key_release_action (FileSearchDialog *dialog,
                    GdkEventKey      *event)
{
  FileSearchDialogPrivate *priv;
  gint text_length;
  
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  if (event->keyval == GDK_KEY_Up ||
      event->keyval == GDK_KEY_Down ||
      event->keyval == GDK_KEY_Left ||
      event->keyval == GDK_KEY_Right)
    return FALSE;

  text_length = gtk_entry_get_text_length (GTK_ENTRY (priv->entry));
  
  if (text_length == 0)
    {
      gtk_list_store_clear (priv->store);
    }
  else if (text_length >= 1) 
    {
      const gchar *text;
      gboolean first_char_changed = FALSE;
      
      text = gtk_entry_get_text (GTK_ENTRY (priv->entry));
      
      if (priv->find_globbing != NULL)
        {
          first_char_changed = text == priv->find_globbing;
          g_free (priv->find_globbing);
        }

      priv->find_globbing = get_globbing (text, TRUE);
      
      if (priv->find_pattern != NULL)
        g_pattern_spec_free (priv->find_pattern);
      
      priv->find_pattern = g_pattern_spec_new (priv->find_globbing);

      if (!first_char_changed && 
          gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->filter), NULL) > 0)
        {
          gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
        }
      else
        {
          GList *indexes;
          
          gtk_list_store_clear (priv->store);

          indexes = get_indexes (dialog);
          
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
get_indexes (FileSearchDialog *dialog)
{
  FileSearchDialogPrivate *priv;
  GList *results = NULL;

  GIOChannel *channel = NULL;
  gchar *line;
  gsize len;

  gchar *group_folder_path;
  gchar *group_indexes_file;
  
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);

  group_folder_path = codeslayer_get_group_config_folder_path (priv->codeslayer);
  group_indexes_file = g_strconcat (group_folder_path, G_DIR_SEPARATOR_S, "filesearch", NULL);
  
  channel = g_io_channel_new_file (group_indexes_file, "r", NULL);
  if (channel == NULL)
    {
      GtkWidget *dialog;
      dialog =  gtk_message_dialog_new (NULL, 
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                        "The search file does not exist. First index the files in the tools menu.");
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      g_free (group_folder_path);
      g_free (group_indexes_file);
      return NULL;
    }
  
  while (g_io_channel_read_line (channel, &line, &len, NULL, NULL) != G_IO_STATUS_EOF)
    {
      FileSearchIndex *index = get_index (line);
      if (g_pattern_match_string (priv->find_pattern, file_search_index_get_file_name (index)))
        results = g_list_prepend (results, index);
      else
        g_object_unref (index);
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
  gchar *value = NULL;

  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  if (priv->find_pattern == NULL)
    return FALSE;
  
  gtk_tree_model_get (model, iter, FILE_NAME, &value, -1);
  
  if (value == NULL)
    return FALSE;

  if (g_pattern_match_string (priv->find_pattern, value))
    {
      g_free (value);    
      return TRUE;
    }

  g_free (value);
  
  return FALSE;
}

static gchar*
get_globbing (const gchar *entry, 
              gboolean     match_case)
{
  gchar *entry_text;
  gchar *result;
  
  entry_text = g_strconcat (entry, "*", NULL);
  
  if (match_case)
    {
      result = entry_text;
    }
  else
    {
      result = codeslayer_utils_to_lowercase (entry_text);
      g_free (entry_text);
    }
  
  return result;
}

static void
select_tree (FileSearchDialog *dialog, 
             GdkEventKey      *event)
{
  FileSearchDialogPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  
  priv = FILE_SEARCH_DIALOG_GET_PRIVATE (dialog);
  
  if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->filter), NULL) <= 0)
    return;  

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  
  if (gtk_tree_selection_get_selected (selection, &priv->filter, &iter))
    {
      GtkTreePath *path;
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->filter), &iter);

      if (event->keyval == GDK_KEY_Up)
        gtk_tree_path_prev (path);
      else if (event->keyval == GDK_KEY_Down)
        gtk_tree_path_next (path);

      if (path != NULL)
        {
          if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->filter), &iter, path))
            {
              gtk_tree_selection_select_iter (selection, &iter);
              gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->tree), path, 
                                            NULL, FALSE, 0.0, 0.0);
            
            }
          gtk_tree_path_free (path);
        }
    }
  else
    {
      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->filter), &iter))
        gtk_tree_selection_select_iter (selection, &iter);
    }
}

static void
row_activated_action (FileSearchDialog  *dialog)
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
      gchar *file_path; 
      GtkTreePath *tree_path = tmp->data;
      
      gtk_tree_model_get_iter (tree_model, &treeiter, tree_path);
      gtk_tree_model_get (GTK_TREE_MODEL (priv->filter), &treeiter, FILE_PATH, &file_path, -1);
      
      codeslayer_select_editor_by_file_path (priv->codeslayer, file_path, 0);
      gtk_widget_hide (priv->dialog);
      
      g_free (file_path);
      gtk_tree_path_free (tree_path);
    }

  g_list_free (selected_rows);
}                     

gint
sort_compare (GtkTreeModel *model, 
              GtkTreeIter  *a,
              GtkTreeIter  *b, 
              gpointer      userdata)
{
  gint sortcol;
  gint ret = 0;

  sortcol = GPOINTER_TO_INT (userdata);
  
  switch (sortcol)
    {
    case FILE_NAME:
      {
        gchar *text1, *text2;

        gtk_tree_model_get (model, a, FILE_NAME, &text1, -1);
        gtk_tree_model_get (model, b, FILE_NAME, &text2, -1);

        ret = g_strcmp0 (text1, text2);

        g_free (text1);
        g_free (text2);
      }
      break;
    }

  return ret;
}
