// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oobewindow.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/paths.h"
#include "lingmo/appearance/appearance.h"
#include "ui/application.h"
#include "ui/cell_layout.h"
#include "ui/check_box.h"
#include "ui/dialog_window.h"
#include "ui/fast_blur.h"
#include "ui/image_view.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "oobestate.h"

using namespace Lingmo::ui;

namespace {

class PageStack final : public View
{
public:
    View* addPage(std::unique_ptr<View> page)
    {
        View* result = page.get();
        m_pages.push_back(result);
        addChild(std::move(page));
        result->setVisible(m_pages.size() == 1);
        return result;
    }

    void setCurrent(size_t index)
    {
        if (index >= m_pages.size())
            return;
        m_current = index;
        for (size_t i = 0; i < m_pages.size(); ++i)
            m_pages[i]->setVisible(i == m_current);
        requestLayout();
        requestPaint();
    }

protected:
    void layout() override
    {
        for (View* page : m_pages)
            page->setGeometry(localRect());
    }

private:
    std::vector<View*> m_pages;
    size_t m_current = 0;
};

class SetupCard final : public View
{
public:
    SetupCard(Lingmo::appearance::Appearance& appearance, std::function<void()> onComplete)
        : m_appearance(appearance)
        , m_onComplete(std::move(onComplete))
    {
        setClipsChildren(true);
        setChildClipRadius(Theme::current().windowRadius);

        m_row = addChild<RowLayout>();
        m_row->setLayoutHints({.fillWidth = true, .fillHeight = true});
        m_row->setPadding({56, 48, 56, 48});
        m_row->setSpacing(48);

        m_logo = m_row->addChild<ImageView>(std::string(OOBE_DATA_DIR) + "/images/logo.svg");
        m_logo->setFillMode(ImageView::FillMode::PreserveAspectFit);
        m_logo->setCornerRadius(28, themeCorners());
        m_logo->setLayoutHints({.preferredWidth = 236, .preferredHeight = 236,
                                .verticalAlignment = Alignment::Center});

        m_panel = m_row->addChild<ColumnLayout>();
        m_panel->setLayoutHints({.fillWidth = true, .fillHeight = true});
        m_panel->setSpacing(Theme::current().largeSpacing);

        m_title = m_panel->addChild<Label>();
        m_title->setFontSize(28);
        m_title->setWeight(600);
        m_subtitle = m_panel->addChild<Label>();
        m_subtitle->setFontSize(14);
        m_subtitle->setColor(ColorId::DisabledText);

        m_pages = m_panel->addChild<PageStack>();
        m_pages->setLayoutHints({.fillWidth = true, .fillHeight = true, .stretch = 1});
        addPages();

        m_actions = m_panel->addChild<RowLayout>();
        m_actions->setLayoutHints({.fillWidth = true});
        m_actions->setSpacing(Theme::current().mediumSpacing);

        m_back = m_actions->addChild<DialogButton>("Back");
        m_back->setLayoutHints({.preferredWidth = 112, .preferredHeight = 44});
        m_back->onClicked = [this] {
            if (m_page > 0)
                showPage(m_page - 1);
        };

        auto* spacer = m_actions->addChild<View>();
        spacer->setLayoutHints({.fillWidth = true, .stretch = 1});

        m_next = m_actions->addChild<DialogButton>("Continue", true);
        m_next->setName("oobe-next");
        m_next->setLayoutHints({.preferredWidth = 144, .preferredHeight = 44});
        m_next->onClicked = [this] {
            if (m_page + 1 < kPageCount)
                showPage(m_page + 1);
            else if (m_onComplete)
                m_onComplete();
        };

        showPage(0);
    }

protected:
    void layout() override
    {
        m_row->setGeometry(localRect());
    }

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

private:
    static constexpr size_t kPageCount = 4;

    static std::unique_ptr<ColumnLayout> page(std::string text)
    {
        auto result = std::make_unique<ColumnLayout>();
        result->setSpacing(Theme::current().mediumSpacing);
        auto* body = result->addChild<Label>(std::move(text));
        body->setWrap(true);
        body->setFontSize(16);
        return result;
    }

    void addPages()
    {
        m_pages->addPage(page("A calm, capable desktop for your everyday work.\n\nThis setup takes a few moments and can be changed later in Settings."));

        auto language = page("Choose the language and region you want to use during this session.");
        auto* english = language->addChild<RadioButton>("English (United States)");
        auto* chinese = language->addChild<RadioButton>("中文（简体）");
        m_languageGroup.add(english);
        m_languageGroup.add(chinese);
        english->setChecked(true);
        m_pages->addPage(std::move(language));

        auto appearance = page("Choose how Lingmo Aurora should look. You can change this later in Settings.");
        auto* dark = appearance->addChild<CheckBox>("Use dark appearance");
        dark->setChecked(m_appearance.darkMode());
        dark->onToggled = [this](bool checked) { m_appearance.setDarkMode(checked); };
        m_pages->addPage(std::move(appearance));

        m_pages->addPage(page("Your basic preferences are ready. Select Finish to enter the Lingmo Aurora desktop."));
    }

    void showPage(size_t page)
    {
        m_page = page;
        static constexpr const char* titles[] = {
            "Welcome to Lingmo Aurora",
            "Language & region",
            "Personalize your desktop",
            "You're ready to go",
        };
        static constexpr const char* subtitles[] = {
            "Let's get your new desktop ready.",
            "These choices apply to your first session.",
            "Choose a comfortable appearance.",
            "Finish setup to start using Aurora.",
        };
        m_title->setText(titles[page]);
        m_subtitle->setText(subtitles[page]);
        m_pages->setCurrent(page);
        m_back->setVisible(page > 0);
        m_next->setText(page + 1 == kPageCount ? "Finish" : "Continue");
        requestLayout();
        requestPaint();
    }

    Lingmo::appearance::Appearance& m_appearance;
    std::function<void()> m_onComplete;
    RowLayout* m_row = nullptr;
    ImageView* m_logo = nullptr;
    ColumnLayout* m_panel = nullptr;
    Label* m_title = nullptr;
    Label* m_subtitle = nullptr;
    PageStack* m_pages = nullptr;
    RowLayout* m_actions = nullptr;
    DialogButton* m_back = nullptr;
    DialogButton* m_next = nullptr;
    ButtonGroup m_languageGroup;
    size_t m_page = 0;
};

class OobeView final : public View
{
public:
    explicit OobeView(std::function<void()> onComplete)
        : m_appearance()
        , m_wallpaper(addChild<FastBlurredCopy>(22))
        , m_tint(addChild<BackdropTint>())
        , m_card(addChild<SetupCard>(m_appearance, std::move(onComplete)))
    {
        setClipsChildren(false);

        std::string wallpaper = m_appearance.wallpaper();
        if (wallpaper.empty() || !std::filesystem::is_regular_file(wallpaper))
            wallpaper = "/usr/share/backgrounds/lingmoos/default.jpg";
        m_wallpaperImage = m_wallpaper->setContent<ImageView>(std::move(wallpaper));
        m_wallpaperImage->setFillMode(ImageView::FillMode::PreserveAspectCrop);
        m_wallpaperImage->onStatusChanged = [this](ImageView::Status) {
            m_wallpaper->contentChanged();
        };
    }

protected:
    void layout() override
    {
        const RectF bounds = localRect();
        const double margin = 48;
        const double cardWidth = std::min(960.0, std::max(0.0, bounds.width - 2 * margin));
        const double cardHeight = std::min(600.0, std::max(0.0, bounds.height - 2 * margin));
        m_wallpaper->setGeometry(bounds);
        m_tint->setGeometry(bounds);
        m_card->setGeometry({(bounds.width - cardWidth) / 2, (bounds.height - cardHeight) / 2,
                             cardWidth, cardHeight});
    }

private:
    class BackdropTint final : public View
    {
    protected:
        void paint(Painter& painter) override
        {
            const Theme& theme = Theme::current();
            painter.fillRect(localRect(), theme.background.withAlpha(theme.darkMode ? 0.28 : 0.14));
        }
    };

    Lingmo::appearance::Appearance m_appearance;
    FastBlurredCopy* m_wallpaper;
    BackdropTint* m_tint;
    ImageView* m_wallpaperImage = nullptr;
    SetupCard* m_card;
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
    m_view = std::make_unique<OobeView>([this] { complete(); });
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
