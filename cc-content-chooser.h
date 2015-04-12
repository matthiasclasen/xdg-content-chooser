/* xdg-content-chooser
 * Copyright (C) 2015 Matthias Clasen
 *
 * This probram is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <gtk/gtk.h>

G_DECLARE_FINAL_TYPE(CcContentChooser, cc_content_chooser, CC, CONTENT_CHOOSER, GtkWindow)

GtkWidget *cc_content_chooser_new (void);

void cc_content_chooser_set_app_id     (CcContentChooser  *chooser,
                                        const char        *app_id);
void cc_content_chooser_set_mime_types (CcContentChooser  *chooser,
                                        const char       **mime_types);
const char *cc_content_chooser_get_uri (CcContentChooser  *chooser);
