// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oobestate.h"
#include "oobewindow.h"

#include "base/paths.h"
#include "ui/application.h"

int main(int argc, char** argv)
{
    Lingmo::ui::Application application(argc, argv);
    application.setApplicationId("lingmo-oobe");

    if (oobe::isComplete(Lingmo::base::paths::home()))
        return 0;

    OobeWindow window;
    window.show();
    return application.run();
}
