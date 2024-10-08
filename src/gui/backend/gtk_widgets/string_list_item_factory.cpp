// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/logger.h"
#include "gui/backend/gtk_widgets/libadwaita_wrapper.h"
#include "gui/backend/gtk_widgets/string_list_item_factory.h"

static void
setup_cb (
  GtkListItemFactory * factory,
  GtkListItem *        list_item,
  gpointer             user_data)
{
  GtkLabel * label = GTK_LABEL (gtk_label_new (nullptr));
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
  gtk_list_item_set_child (list_item, GTK_WIDGET (label));
}

static void
bind_cb (GtkListItemFactory * factory, GtkListItem * list_item, gpointer user_data)
{
  GtkLabel *  lbl = GTK_LABEL (gtk_list_item_get_child (list_item));
  GObject *   gobj = G_OBJECT (gtk_list_item_get_item (list_item));
  std::string str = "(invalid)";
  if (GTK_IS_STRING_OBJECT (gobj))
    {
      GtkStringObject * sobj =
        GTK_STRING_OBJECT (gtk_list_item_get_item (list_item));
      str = gtk_string_object_get_string (sobj);
    }
  else if (ADW_IS_ENUM_LIST_ITEM (gobj))
    {
      AdwEnumListItem * enum_item = ADW_ENUM_LIST_ITEM (gobj);
      z_return_if_fail (user_data);
      auto getter = (StringListItemFactoryEnumStringGetter) user_data;
      str = getter (adw_enum_list_item_get_value (enum_item));
    }
  gtk_label_set_text (lbl, str.c_str ());
}

static void
teardown_cb (
  GtkListItemFactory * factory,
  GtkListItem *        list_item,
  gpointer             user_data)
{
  gtk_list_item_set_child (list_item, nullptr);
}

GtkListItemFactory *
string_list_item_factory_new (StringListItemFactoryEnumStringGetter getter)
{
  GtkSignalListItemFactory * factory =
    GTK_SIGNAL_LIST_ITEM_FACTORY (gtk_signal_list_item_factory_new ());
  g_signal_connect (factory, "setup", G_CALLBACK (setup_cb), nullptr);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_cb), (gpointer) getter);
  g_signal_connect (factory, "teardown", G_CALLBACK (teardown_cb), nullptr);

  return GTK_LIST_ITEM_FACTORY (factory);
}
