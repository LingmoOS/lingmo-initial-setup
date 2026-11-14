// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "oobewindow.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/paths.h"
#include "lingmo/appearance/appearance.h"
#include "ui/application.h"
#include "ui/animation.h"
#include "ui/cell_layout.h"
#include "ui/check_box.h"
#include "ui/dialog_window.h"
#include "ui/fast_blur.h"
#include "ui/icon.h"
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
    PageStack()
    {
        setClipsChildren(true);
    }

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
        if (index == m_current && m_hasCurrent)
            return;

        stopTransition();
        if (!m_hasCurrent) {
            m_current = index;
            m_hasCurrent = true;
            for (size_t i = 0; i < m_pages.size(); ++i) {
                m_pages[i]->setVisible(i == m_current);
                m_pages[i]->setOpacity(i == m_current ? 1 : 0);
                m_pages[i]->setTransform({});
            }
            return;
        }

        View* previous = m_pages[m_current];
        View* next = m_pages[index];
        const double direction = index > m_current ? 1 : -1;
        m_current = index;
        next->setVisible(true);
        next->setOpacity(0);
        next->setTransform({{direction * 36, 0}, 1});
        raiseChild(next);
        requestLayout();
        requestPaint();

        if (!window()) {
            finishTransition(previous, next);
            return;
        }

        m_transition = std::make_unique<Animation>(window());
        m_transition->setDuration(std::chrono::milliseconds(260));
        m_transition->setEasing(Animation::Easing::OutCubic);
        m_transition->onUpdate = [this, previous, next, direction](double value) {
            previous->setOpacity(1 - value);
            previous->setTransform({{-direction * 36 * value, 0}, 1});
            next->setOpacity(value);
            next->setTransform({{direction * 36 * (1 - value), 0}, 1});
        };
        m_transition->onFinished = [this, previous, next] { finishTransition(previous, next); };
        m_transition->start();
    }

protected:
    void layout() override
    {
        for (View* page : m_pages)
            page->setGeometry(localRect());
    }

private:
    void stopTransition()
    {
        if (!m_transition)
            return;
        m_transition->stop();
        m_transition.reset();
        for (View* page : m_pages) {
            page->setVisible(false);
            page->setOpacity(1);
            page->setTransform({});
        }
        if (m_hasCurrent)
            m_pages[m_current]->setVisible(true);
    }

    void finishTransition(View* previous, View* next)
    {
        previous->setVisible(false);
        previous->setOpacity(1);
        previous->setTransform({});
        next->setVisible(true);
        next->setOpacity(1);
        next->setTransform({});
        requestPaint();
    }

    std::vector<View*> m_pages;
    size_t m_current = 0;
    bool m_hasCurrent = false;
    std::unique_ptr<Animation> m_transition;
};

class PageVisual final : public View
{
public:
    explicit PageVisual(std::string wallpaper)
        : m_wallpaper(addChild<ImageView>(std::move(wallpaper)))
        , m_icon(addChild<ThemeIcon>("applications-education-language", 92))
    {
        m_wallpaper->setFillMode(ImageView::FillMode::PreserveAspectCrop);
        m_wallpaper->setCornerRadius(118, themeCorners());
        m_icon->setVisible(false);
    }

    void setWallpaper(std::string wallpaper)
    {
        m_wallpaper->setSource(std::move(wallpaper));
        m_wallpaper->requestPaint();
    }

    void setPage(size_t page)
    {
        if (page == m_page)
            return;
        static constexpr const char* icons[] = {
            "", "applications-education-language", "zorin-appearance", "emblem-checked"};
        View* oldView = m_page == 0 ? static_cast<View*>(m_wallpaper) : static_cast<View*>(m_icon);
        View* newView = page == 0 ? static_cast<View*>(m_wallpaper) : static_cast<View*>(m_icon);
        m_page = page;
        if (m_transition)
            m_transition->stop();
        m_transition.reset();
        oldView->setVisible(true);
        oldView->setOpacity(1);
        if (page != 0)
            m_icon->setIconName(icons[std::min(page, size_t(3))]);
        newView->setVisible(true);
        newView->setOpacity(0);
        if (!window()) {
            finishTransition(oldView, newView);
            return;
        }
        m_transition = std::make_unique<Animation>(window());
        m_transition->setDuration(std::chrono::milliseconds(220));
        m_transition->setEasing(Animation::Easing::InOutCubic);
        m_transition->onUpdate = [oldView, newView](double value) {
            oldView->setOpacity(1 - value);
            newView->setOpacity(value);
        };
        m_transition->onFinished = [this, oldView, newView] { finishTransition(oldView, newView); };
        m_transition->start();
    }

protected:
    void layout() override
    {
        m_wallpaper->setGeometry(localRect());
        m_icon->setGeometry(localRect());
    }

    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        if (!m_icon->isVisible())
        {
            painter.drawShadow(localRect(), localRect().width / 2, 16,
                               Color::rgb(0x000000).withAlpha(theme.darkMode ? 0.30 : 0.18), {0, 8});
            return;
        }
        const RectF circle = localRect().adjusted(18, 18, -18, -18);
        painter.drawShadow(circle, circle.width / 2, 14,
                           Color::rgb(0x000000).withAlpha(theme.darkMode ? 0.28 : 0.16), {0, 6});
        painter.fillEllipse(circle, theme.secondBackground.withAlpha(theme.darkMode ? 0.84 : 0.88));
        painter.fillEllipse(circle.adjusted(3, 3, -3, -3),
                           theme.highlight.withAlpha(theme.darkMode ? 0.16 : 0.10));
    }

private:
    void finishTransition(View* oldView, View* newView)
    {
        if (oldView != newView)
            oldView->setVisible(false);
        oldView->setOpacity(1);
        newView->setVisible(true);
        newView->setOpacity(1);
        requestPaint();
    }

    ImageView* m_wallpaper;
    ThemeIcon* m_icon;
    size_t m_page = 0;
    std::unique_ptr<Animation> m_transition;
};

class StepIndicator final : public View
{
public:
    void setCurrent(size_t page)
    {
        if (page == m_current)
            return;
        m_current = page;
        requestPaint();
    }

protected:
    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        const double gap = 6;
        const double height = 5;
        const double width = (geometry().width - gap * 3) / 4;
        for (size_t i = 0; i < 4; ++i) {
            const RectF segment {(width + gap) * i, 0, width, height};
            const Color color = i <= m_current ? theme.highlight : theme.alternateBackground;
            painter.fillRoundedRect(segment, height / 2, color);
        }
    }

private:
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
        m_row->setPadding({64, 52, 64, 52});
        m_row->setSpacing(56);

        std::string wallpaper = m_appearance.wallpaper();
        std::error_code error;
        if (wallpaper.empty() || !std::filesystem::is_regular_file(wallpaper, error))
            wallpaper = "/usr/share/backgrounds/lingmoos/default.jpg";
        m_visual = m_row->addChild<PageVisual>(std::move(wallpaper));
        m_visual->setLayoutHints({.preferredWidth = 264, .preferredHeight = 264,
                                  .verticalAlignment = Alignment::Center});

        m_panel = m_row->addChild<ColumnLayout>();
        m_panel->setLayoutHints({.fillWidth = true, .fillHeight = true});
        m_panel->setSpacing(16);

        m_title = m_panel->addChild<Label>();
        m_title->setFontSize(30);
        m_title->setWeight(600);
        m_subtitle = m_panel->addChild<Label>();
        m_subtitle->setFontSize(15);
        m_subtitle->setColor(ColorId::DisabledText);

        m_steps = m_panel->addChild<StepIndicator>();
        m_steps->setLayoutHints({.fillWidth = true, .preferredHeight = 5});

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
        m_appearanceChanged = m_appearance.changed.connect([this] {
            updateWallpaper();
        });
    }

protected:
    void layout() override
    {
        const double width = geometry().width;
        const double inset = std::clamp(width * 0.065, 32.0, 64.0);
        const double spacing = std::clamp(width * 0.055, 32.0, 56.0);
        const double visualSize = std::clamp(width * 0.30, 176.0, 264.0);
        m_row->setPadding({inset, 52, inset, 52});
        m_row->setSpacing(spacing);
        m_visual->setLayoutHints({.preferredWidth = visualSize, .preferredHeight = visualSize,
                                  .verticalAlignment = Alignment::Center});
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
        m_steps->setCurrent(page);
        m_pages->setCurrent(page);
        m_visual->setPage(page);
        m_back->setVisible(page > 0);
        m_next->setText(page + 1 == kPageCount ? "Finish" : "Continue");
        requestLayout();
        requestPaint();
    }

    void updateWallpaper()
    {
        std::string wallpaper = m_appearance.wallpaper();
        std::error_code error;
        if (wallpaper.empty() || !std::filesystem::is_regular_file(wallpaper, error))
            wallpaper = "/usr/share/backgrounds/lingmoos/default.jpg";
        m_visual->setWallpaper(std::move(wallpaper));
    }

    Lingmo::appearance::Appearance& m_appearance;
    base::Subscription m_appearanceChanged;
    std::function<void()> m_onComplete;
    RowLayout* m_row = nullptr;
    PageVisual* m_visual = nullptr;
    ColumnLayout* m_panel = nullptr;
    Label* m_title = nullptr;
    Label* m_subtitle = nullptr;
    StepIndicator* m_steps = nullptr;
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

        m_wallpaperImage = m_wallpaper->setContent<ImageView>();
        m_wallpaperImage->setFillMode(ImageView::FillMode::PreserveAspectCrop);
        m_wallpaperImage->onStatusChanged = [this](ImageView::Status) {
            m_wallpaper->contentChanged();
        };
        m_appearanceChanged = m_appearance.changed.connect([this] { updateWallpaper(); });
        updateWallpaper();
    }

protected:
    void layout() override
    {
        const RectF bounds = localRect();
        const double margin = 32;
        const double cardWidth = std::min(1040.0, std::max(0.0, bounds.width - 2 * margin));
        const double cardHeight = std::min(640.0, std::max(0.0, bounds.height - 2 * margin));
        m_wallpaper->setGeometry(bounds);
        m_tint->setGeometry(bounds);
        m_card->setGeometry({(bounds.width - cardWidth) / 2, (bounds.height - cardHeight) / 2,
                             cardWidth, cardHeight});
    }

private:
    void updateWallpaper()
    {
        std::string wallpaper = m_appearance.wallpaper();
        std::error_code error;
        if (wallpaper.empty() || !std::filesystem::is_regular_file(wallpaper, error))
            wallpaper = "/usr/share/backgrounds/lingmoos/default.jpg";
        m_wallpaperImage->setSource(std::move(wallpaper));
        m_wallpaper->contentChanged();
    }

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
    base::Subscription m_appearanceChanged;
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
