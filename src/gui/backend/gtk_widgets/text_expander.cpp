// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/utils/gtk.h"
#include "common/utils/logger.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/text_expander.h"

#include <glib/gi18n.h>

#define TEXT_EXPANDER_WIDGET_TYPE (text_expander_widget_get_type ())
G_DEFINE_TYPE (TextExpanderWidget, text_expander_widget, EXPANDER_BOX_WIDGET_TYPE)

static void
on_focus_enter (GtkEventControllerFocus * focus_controller, gpointer user_data)
{
  TextExpanderWidget * self = Z_TEXT_EXPANDER_WIDGET (user_data);

  z_info ("text focused");
  self->has_focus = true;
}

static void
on_focus_leave (GtkEventControllerFocus * focus_controller, gpointer user_data)
{
  TextExpanderWidget * self = Z_TEXT_EXPANDER_WIDGET (user_data);

  z_info ("text focus out");
  self->has_focus = false;

  if (self->setter && self->obj)
    {
      GtkTextIter start_iter, end_iter;
      gtk_text_buffer_get_start_iter (
        GTK_TEXT_BUFFER (self->buffer), &start_iter);
      gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (self->buffer), &end_iter);
      char * content = gtk_text_buffer_get_text (
        GTK_TEXT_BUFFER (self->buffer), &start_iter, &end_iter, false);
      self->setter (content);
      text_expander_widget_refresh (self);
    }
}

/**
 * Refreshes the text.
 */
void
text_expander_widget_refresh (TextExpanderWidget * self)
{
  if (self->getter && self->obj)
    {
      z_return_if_fail (self->buffer);
      gtk_text_buffer_set_text (
        GTK_TEXT_BUFFER (self->buffer), self->getter ().c_str (), -1);
      gtk_label_set_text (self->label, self->getter ().c_str ());
    }
}

/**
 * Sets up the TextExpanderWidget.
 */
void
text_expander_widget_setup (
  TextExpanderWidget * self,
  bool                 wrap_text,
  GenericStringGetter  getter,
  GenericStringSetter  setter,
  void *               obj)
{
  self->getter = getter;
  self->setter = setter;
  self->obj = obj;

  gtk_label_set_wrap (self->label, wrap_text);
  if (wrap_text)
    gtk_label_set_wrap_mode (self->label, PANGO_WRAP_WORD_CHAR);

  text_expander_widget_refresh (self);
}

static void
text_expander_widget_class_init (TextExpanderWidgetClass * klass)
{
}

static void
text_expander_widget_init (TextExpanderWidget * self)
{
  GtkWidget * box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  self->scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_widget_set_vexpand (GTK_WIDGET (self->scroll), 1);
  gtk_widget_set_visible (GTK_WIDGET (self->scroll), 1);
  gtk_widget_set_size_request (GTK_WIDGET (self->scroll), -1, 124);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->scroll));

  self->viewport = GTK_VIEWPORT (gtk_viewport_new (nullptr, nullptr));
  gtk_widget_set_visible (GTK_WIDGET (self->viewport), 1);
  gtk_scrolled_window_set_child (self->scroll, GTK_WIDGET (self->viewport));

  self->label = GTK_LABEL (gtk_label_new (""));
  gtk_viewport_set_child (self->viewport, GTK_WIDGET (self->label));
  gtk_widget_set_vexpand (GTK_WIDGET (self->label), true);

  self->edit_btn = GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->edit_btn));
  gtk_menu_button_set_icon_name (self->edit_btn, "pencil");

  self->popover = GTK_POPOVER (gtk_popover_new ());
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self->edit_btn), GTK_WIDGET (self->popover));

  self->buffer = gtk_text_buffer_new (nullptr);
  z_return_if_fail (self->buffer);
  self->editor = GTK_TEXT_VIEW (gtk_text_view_new_with_buffer (self->buffer));
  gtk_popover_set_child (self->popover, GTK_WIDGET (self->editor));
  gtk_widget_set_visible (GTK_WIDGET (self->editor), true);
  gtk_text_view_set_accepts_tab (self->editor, false);

#if 0
  /* set style */
  GtkSourceStyleSchemeManager * style_mgr =
    gtk_source_style_scheme_manager_get_default ();
  gtk_source_style_scheme_manager_prepend_search_path (
    style_mgr, CONFIGURE_SOURCEVIEW_STYLES_DIR);
  gtk_source_style_scheme_manager_force_rescan (style_mgr);
  GtkSourceStyleScheme * scheme = gtk_source_style_scheme_manager_get_scheme (
    style_mgr, "monokai-extended-zrythm");
  gtk_source_buffer_set_style_scheme (self->buffer, scheme);
#endif

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (box));

  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self), "gnome-icon-library-chat-symbolic");

  GtkEventControllerFocus * focus_controller =
    GTK_EVENT_CONTROLLER_FOCUS (gtk_event_controller_focus_new ());
  g_signal_connect (
    G_OBJECT (focus_controller), "leave", G_CALLBACK (on_focus_leave), self);
  g_signal_connect (
    G_OBJECT (focus_controller), "enter", G_CALLBACK (on_focus_enter), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->editor), GTK_EVENT_CONTROLLER (focus_controller));
}
