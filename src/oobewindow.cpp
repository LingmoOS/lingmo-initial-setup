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
#include "ui/painter.h"
#include "ui/popup_glass.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "oobestate.h"

using namespace Lingmo::ui;

namespace {

class WelcomeCard final : public View
{
public:
    WelcomeCard(Label*& title, Label*& subtitle, ImageView*& logo, Button*& start,
                std::function<void()> onStart)
    {
        setClipsChildren(true);
        setChildClipRadius(Theme::current().windowRadius);

        auto* row = addChild<RowLayout>();
        row->setLayoutHints({.fillWidth = true, .fillHeight = true});
        row->setPadding({56, 48, 56, 48});
        row->setSpacing(48);

        logo = row->addChild<ImageView>(std::string(OOBE_DATA_DIR) + "/images/logo.svg");
        logo->setFillMode(ImageView::FillMode::PreserveAspectFit);
        logo->setCornerRadius(28, themeCorners());
        logo->setLayoutHints({.preferredWidth = 236, .preferredHeight = 236,
                              .verticalAlignment = Alignment::Center});

        auto* column = row->addChild<ColumnLayout>();
        column->setLayoutHints({.fillWidth = true, .fillHeight = true,
                                .horizontalAlignment = Alignment::Start});
        column->setSpacing(Theme::current().largeSpacing);

        title = column->addChild<Label>("Welcome to Lingmo Aurora");
        title->setFontSize(28);
        title->setWeight(600);

        subtitle = column->addChild<Label>("A calm, capable desktop for your everyday work.");
        subtitle->setFontSize(14);
        subtitle->setColor(ColorId::DisabledText);

        auto* spacer = column->addChild<View>();
        spacer->setLayoutHints({.fillHeight = true, .stretch = 1});

        start = column->addChild<Button>("Get Started");
        start->setName("oobe-start");
        start->setLayoutHints({.preferredWidth = 180, .preferredHeight = 48,
                               .horizontalAlignment = Alignment::Center});
        start->onClicked = std::move(onStart);
    }

protected:
    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        const Color shadow = Color::rgb(0x000000).withAlpha(theme.darkMode ? 0.42 : 0.25);
        const Color surface = theme.secondBackground.withAlpha(theme.darkMode ? 0.88 : 0.84);
        painter.drawShadow(localRect(), theme.windowRadius, 24, shadow, {0, 12});
        painter.fillRoundedRect(localRect(), theme.windowRadius, surface, themeCorners());
        painter.drawSmoothRectangle(localRect().adjusted(0.5, 0.5, -0.5, -0.5),
                                    theme.windowRadius, Color::rgb(0x000000).withAlpha(0), 1,
                                    theme.text.withAlpha(theme.darkMode ? 0.08 : 0.12));
    }
};

class OobeView final : public View
{
public:
    OobeView(Label*& title, Label*& subtitle, ImageView*& logo, Button*& start,
             std::function<void()> onStart)
        : m_card(addChild<WelcomeCard>(title, subtitle, logo, start, std::move(onStart)))
    {
        setClipsChildren(false);
    }

protected:
    void layout() override
    {
        const RectF bounds = localRect();
        const double margin = 48;
        const double cardWidth = std::min(960.0, std::max(0.0, bounds.width - 2 * margin));
        const double cardHeight = std::min(600.0, std::max(0.0, bounds.height - 2 * margin));
        m_card->setGeometry({(bounds.width - cardWidth) / 2, (bounds.height - cardHeight) / 2,
                             cardWidth, cardHeight});

        if (auto* surface = window()) {
            const Theme& theme = Theme::current();
            surface->setBackgroundBlur(bounds, 0);
            if (surface->backgroundGlassAvailable()) {
                auto glass = popupGlass(theme.darkMode);
                glass.blurSigma = 10;
                glass.displacement = 0;
                glass.tintAlpha = theme.darkMode ? 0.16 : 0.10;
                surface->setBackgroundGlass(glass);
            }
        }
    }

    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        painter.fillRect(localRect(), theme.background.withAlpha(theme.darkMode ? 0.28 : 0.14));
    }

private:
    WelcomeCard* m_card;
};

} // namespace

OobeWindow::OobeWindow()
{
    m_window.setTitle("Lingmo Aurora Setup");
    m_window.setFullScreen(true);
    m_window.setUserResizable(false);
    m_window.setSkipTaskbar(true);
    m_window.setAlwaysOnTop(true);
    m_window.setBackgroundColor(Color::rgb(0x000000).withAlpha(0));
    m_view = std::make_unique<OobeView>(m_title, m_subtitle, m_logo, m_start,
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
