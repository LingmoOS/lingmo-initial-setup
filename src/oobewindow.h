// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "ui/window.h"

namespace Lingmo::ui {
class Application;
class Button;
class ImageView;
class Label;
class View;
}

class OobeWindow
{
public:
    OobeWindow();
    ~OobeWindow();

    void show();

private:
    void complete();
    void refreshTheme();

    Lingmo::ui::Window m_window;
    std::unique_ptr<Lingmo::ui::View> m_view;
    Lingmo::ui::Label* m_title = nullptr;
    Lingmo::ui::Label* m_subtitle = nullptr;
    Lingmo::ui::ImageView* m_logo = nullptr;
    Lingmo::ui::Button* m_start = nullptr;
};
