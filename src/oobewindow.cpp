// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oobewindow.h"

#include <algorithm>
#include <functional>
#include <memory>

#include "base/paths.h"
#include "ui/application.h"
#include "ui/button.h"
#include "ui/cell_layout.h"
#include "ui/color_id.h"
#include "ui/image_view.h"
#include "ui/label.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "oobestate.h"

using namespace Lingmo::ui;

namespace {

class WelcomeView final : public View
{
public:
    WelcomeView(Label*& title, Label*& subtitle, ImageView*& logo, Button*& start,
                std::function<void()> onStart)
    {
        auto* column = addChild<ColumnLayout>();
        column->setLayoutHints({.fillWidth = true, .fillHeight = true,
                                .horizontalAlignment = Alignment::Center});
        column->setSpacing(Theme::current().largeSpacing);

        logo = column->addChild<ImageView>(std::string(OOBE_DATA_DIR) + "/images/logo.svg");
        logo->setFillMode(ImageView::FillMode::PreserveAspectFit);
        logo->setLayoutHints({.preferredWidth = 104, .preferredHeight = 104,
                              .verticalAlignment = Alignment::Center});

        title = column->addChild<Label>("Welcome to Lingmo Aurora");
        title->setFontSize(28);
        title->setWeight(600);
        title->setTextAlignment(Label::TextAlignment::Center);

        subtitle = column->addChild<Label>("A calm, capable desktop for your everyday work.");
        subtitle->setFontSize(14);
        subtitle->setColor(ColorId::DisabledText);
        subtitle->setTextAlignment(Label::TextAlignment::Center);

        auto* spacer = column->addChild<View>();
        spacer->setLayoutHints({.fillHeight = true, .stretch = 1});

        start = column->addChild<Button>("Get Started");
        start->setName("oobe-start");
        start->setLayoutHints({.preferredWidth = 180, .preferredHeight = 48,
                               .horizontalAlignment = Alignment::Center});
        start->onClicked = std::move(onStart);
    }

protected:
    void layout() override
    {
        auto* column = dynamic_cast<ColumnLayout*>(children().front().get());
        if (!column)
            return;
        const double inset = std::min(geometry().width * 0.12, 96.0);
        column->setGeometry({inset, 48, std::max(0.0, geometry().width - 2 * inset),
                             std::max(0.0, geometry().height - 96)});
    }
};

} // namespace

OobeWindow::OobeWindow()
{
    m_window.setTitle("Lingmo Aurora Setup");
    m_window.setMinimumSize({720, 520});
    m_window.resize({900, 620});
    m_window.setBackgroundColor(ColorRecipe(ColorId::Background));
    m_window.setBackgroundBlur({0, 0, 900, 620}, Theme::current().windowRadius);
    m_view = std::make_unique<WelcomeView>(m_title, m_subtitle, m_logo, m_start,
                                           [this] { complete(); });
    m_window.setContentView(std::move(m_view));
}

OobeWindow::~OobeWindow() = default;

void OobeWindow::show()
{
    m_window.show();
}

void OobeWindow::complete()
{
    if (!oobe::markComplete(Lingmo::base::paths::home()))
        return;
    oobe::clearAccountState();
    m_window.hide();
    if (auto* application = Application::instance())
        application->quit();
}

void OobeWindow::refreshTheme()
{
    m_window.requestPaint();
}
