#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

// Цвета из макета Claude Design.
namespace Palette
{
const juce::Colour ground     { 0xff0a1233 };
const juce::Colour panel      { 0xff121c45 };
const juce::Colour brandBlue  { 0xff2f5bff };
const juce::Colour lavash     { 0xfff2c886 };
const juce::Colour lavashHi   { 0xfffbe3b0 };
const juce::Colour lavashDark { 0xffc9924f };
const juce::Colour sauceRed   { 0xffe2321f };
const juce::Colour sauceHi    { 0xffff6b57 };
const juce::Colour hot        { 0xffff4436 };
const juce::Colour garlic     { 0xfff6f1e7 };
const juce::Colour meat       { 0xff6e3418 };
const juce::Colour muted      { 0xffa9b3d6 };
const juce::Colour grey       { 0xff8a93b8 };
const juce::Colour green      { 0xff2fd27a };
const juce::Colour skyBlue    { 0xff7fa8ff };
} // namespace Palette

// Встроенные шрифты: Manrope (текст) и Unbounded (заголовки), оба OFL.
struct HitMakerFonts
{
    HitMakerFonts();

    // weight: 400, 600, 700, 800. Размер — как font-size в CSS.
    juce::Font manrope (float size, int weight = 400) const;
    juce::Font unbounded (float size, bool extraBold = true) const;

    juce::Typeface::Ptr manropeRegular, manropeSemiBold, manropeBold, manropeExtraBold;
    juce::Typeface::Ptr unboundedBold, unboundedExtraBold;
};

class KnobSlider : public juce::Slider
{
public:
    KnobSlider() = default;

    std::unique_ptr<juce::Drawable> icon;
};

// Тумблер с подписью из двух частей: жирная + приглушённая.
class TwoPartToggle : public juce::ToggleButton
{
public:
    juce::String mainText, extraText;
};

class ShawarmaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ShawarmaLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& background,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;

    juce::Font getSliderPopupFont (juce::Slider&) override { return fonts->manrope (13.0f, 700); }

private:
    juce::SharedResourcePointer<HitMakerFonts> fonts;
};

// Уведомления в стиле мессенджера, стопкой справа сверху.
class ToastStack : public juce::Component
{
public:
    ToastStack();

    void add (const juce::String& from, const juce::String& text, juce::Colour accent,
              double delaySeconds = 0.0, double lifeSeconds = 5.5);
    void tick (double now);
    void paint (juce::Graphics&) override;

private:
    struct Toast
    {
        juce::String from, text;
        juce::Colour accent;
        double showAt = 0, hideAt = 0;
    };

    float heightFor (const Toast&) const;

    std::vector<Toast> toasts;
    double lastNow = 0;
    juce::SharedResourcePointer<HitMakerFonts> fonts;
};

// Заблокированная крутилка в память о Виталии. Клик — мудрость из архива.
class VitalyKnob : public juce::Component
{
public:
    VitalyKnob();

    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    std::unique_ptr<juce::Drawable> lockIcon;
};

// Большая кнопка. Компонент больше самой кнопки — по краям место под свечение и тень.
class HitButton : public juce::Button
{
public:
    static constexpr int glowX = 24, glowY = 18;

    HitButton() : juce::Button ("hit") {}

    juce::String caption, subcaption;
    bool done = false;
    float pulse = 0.0f;

    bool hitTest (int x, int y) override;
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    juce::SharedResourcePointer<HitMakerFonts> fonts;
};

class CreditsOverlay : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }

private:
    juce::SharedResourcePointer<HitMakerFonts> fonts;
};

class HitMakerEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit HitMakerEditor (HitMakerProcessor&);
    ~HitMakerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

#if HITMAKER_SELFTEST
    // Только для оффлайн-скриншотов: перематываем анимацию готовки.
    void debugStartCooking() { startCooking(); }
    void debugAdvance (double seconds);
    void debugShowCredits() { creditsOverlay.setVisible (true); }
    void debugSetIdea (const juce::String& text) { ideaText = text; }
#endif

private:
    void timerCallback() override;
    void advance (double now, double dt);

    void paintHeader (juce::Graphics&);
    void paintProgress (juce::Graphics&);
    void paintIdea (juce::Graphics&);

    void startCooking();
    void finishCooking();
    void onHitClicked();
    void setHitParam (bool on);
    bool isHitOn() const;
    void refreshHitButton();
    void scheduleVitalyCameo (double now);

    struct Knob
    {
        KnobSlider slider;
        juce::Label title, subtitle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    HitMakerProcessor& hitProcessor;
    juce::SharedResourcePointer<HitMakerFonts> fonts;
    ShawarmaLookAndFeel lookAndFeel;
    juce::Random rng;
    juce::Image logo;

    std::array<Knob, 5> knobs;
    VitalyKnob vitalyKnob;
    juce::Label vitalyTitle, vitalySubtitle;

    HitButton hitButton;
    TwoPartToggle stasToggle, hornToggle;
    juce::TextButton ideaButton, creditsButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stasAttachment, hornAttachment;

    juce::String ideaText;

    ToastStack toasts;
    CreditsOverlay creditsOverlay;

    bool cooking = false;
    double cookProgress = 0.0;
    double cookDuration = 6.0;
    juce::StringArray cookStages;
    juce::String stageText;

    double lastTime = 0.0;
    double nextVitalyCameo = 0.0;
    float pulsePhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HitMakerEditor)
};
