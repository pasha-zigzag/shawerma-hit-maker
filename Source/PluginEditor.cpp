#include "PluginEditor.h"
#include "Icons.h"
#include "Jokes.h"

#include <HitMakerAssets.h>

#include <algorithm>

namespace
{
juce::String utf8 (const char* text) { return juce::String::fromUTF8 (text); }

double nowSeconds() { return juce::Time::getMillisecondCounterHiRes() * 0.001; }

float textWidth (const juce::Font& font, const juce::String& text)
{
    return juce::TextLayout::getStringWidth (font, text);
}

std::unique_ptr<juce::Drawable> loadSvg (const char* svg)
{
    if (auto xml = juce::parseXML (juce::String::fromUTF8 (svg)))
        return juce::Drawable::createFromSVG (*xml);
    return nullptr;
}

constexpr int kWidth = 780;
constexpr int kHeight = 540;
constexpr int kMargin = 20;
constexpr int kHeaderHeight = 64;

// Вертикальная раскладка — как в макете.
constexpr int kPanelY = 78, kPanelH = 160;
constexpr int kKnobSize = 92;
constexpr int kButtonY = 254, kButtonW = 360, kButtonH = 84;
constexpr int kProgressY = 348, kProgressW = 520;
constexpr int kRowY = 400, kRowH = 44;
constexpr int kIdeaY = 454, kIdeaH = 72;

struct KnobInfo
{
    const char* paramId;
    const char* title;
    const char* subtitle;
    juce::uint32 colour;
    const char* icon;
};

const KnobInfo kKnobs[] = {
    { ParamIDs::meat,    "Мясо",    "низ / бас",      0xffff7a3d, Icons::meat },
    { ParamIDs::sauce,   "Соус",    "тепло",          0xfff6f1e7, Icons::sauce },
    { ParamIDs::spice,   "Острота", "верх / яркость", 0xffff4436, Icons::spice },
    { ParamIDs::lavash,  "Лаваш",   "ширина стерео",  0xfff2c886, Icons::lavash },
    { ParamIDs::ambient, "Эмбиент", "by Марина",      0xff7fa8ff, Icons::ambient },
};

void fillPanel (juce::Graphics& g, juce::Rectangle<float> area, float radius)
{
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (area, radius);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (area.reduced (0.5f), radius, 1.0f);
}

// Дуга крутилки: подложка на весь ход и цветная часть до текущего значения.
void drawKnobRing (juce::Graphics& g, juce::Point<float> centre, float startAngle, float endAngle,
                   float valueAngle, juce::Colour accent)
{
    constexpr float ringRadius = 42.5f, ringWidth = 7.0f;
    const juce::PathStrokeType stroke (ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::butt);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.strokePath (track, stroke);

    if (valueAngle > startAngle + 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, valueAngle, true);
        g.setColour (accent);
        g.strokePath (value, stroke);
    }
}

juce::Colour toastAccent (const juce::String& from)
{
    if (from.startsWith ("Sonic"))  return Palette::lavash;
    if (from.startsWith ("Ninja"))  return Palette::sauceHi;
    return Palette::skyBlue;
}
} // namespace

//==============================================================================
HitMakerFonts::HitMakerFonts()
{
    using namespace HitMakerAssets;
    auto load = [] (const char* data, int size) { return juce::Typeface::createSystemTypefaceFor (data, (size_t) size); };

    manropeRegular     = load (ManropeRegular_ttf, ManropeRegular_ttfSize);
    manropeSemiBold    = load (ManropeSemiBold_ttf, ManropeSemiBold_ttfSize);
    manropeBold        = load (ManropeBold_ttf, ManropeBold_ttfSize);
    manropeExtraBold   = load (ManropeExtraBold_ttf, ManropeExtraBold_ttfSize);
    unboundedBold      = load (UnboundedBold_ttf, UnboundedBold_ttfSize);
    unboundedExtraBold = load (UnboundedExtraBold_ttf, UnboundedExtraBold_ttfSize);
}

juce::Font HitMakerFonts::manrope (float size, int weight) const
{
    auto typeface = weight >= 800 ? manropeExtraBold
                  : weight >= 700 ? manropeBold
                  : weight >= 600 ? manropeSemiBold
                                  : manropeRegular;
    return juce::Font (juce::FontOptions (typeface).withPointHeight (size));
}

juce::Font HitMakerFonts::unbounded (float size, bool extraBold) const
{
    return juce::Font (juce::FontOptions (extraBold ? unboundedExtraBold : unboundedBold).withPointHeight (size));
}

//==============================================================================
ShawarmaLookAndFeel::ShawarmaLookAndFeel()
{
    setColour (juce::Label::textColourId, Palette::garlic);
    setColour (juce::BubbleComponent::backgroundColourId, Palette::panel);
    setColour (juce::BubbleComponent::outlineColourId, Palette::lavash);
    setColour (juce::TooltipWindow::textColourId, Palette::garlic);
}

void ShawarmaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                            float startAngle, float endAngle, juce::Slider& slider)
{
    const auto centre = juce::Rectangle<int> (x, y, width, height).toFloat().getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    drawKnobRing (g, centre, startAngle, endAngle, angle, slider.findColour (juce::Slider::rotarySliderFillColourId));

    // Диск-лаваш: тень, радиальный градиент, полоски от гриля, затемнение снизу.
    constexpr float discRadius = 33.0f;
    const auto disc = juce::Rectangle<float> (discRadius * 2.0f, discRadius * 2.0f).withCentre (centre);
    juce::Path discPath;
    discPath.addEllipse (disc);

    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 12, { 0, 5 }).drawForPath (g, discPath);

    juce::ColourGradient lavashFill (Palette::lavashHi, disc.getX() + disc.getWidth() * 0.35f, disc.getY() + disc.getHeight() * 0.3f,
                                     Palette::lavashDark, disc.getX() + disc.getWidth() * 1.0f, disc.getBottom() + disc.getHeight() * 0.05f, true);
    lavashFill.addColour (0.45, Palette::lavash);
    g.setGradientFill (lavashFill);
    g.fillPath (discPath);

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (discPath);

        g.setColour (Palette::meat.withAlpha (0.13f));
        for (float offset = -disc.getWidth(); offset < disc.getWidth() * 2.0f; offset += 12.0f)
            g.drawLine (disc.getX() + offset, disc.getBottom(), disc.getX() + offset + disc.getHeight(), disc.getY(), 3.0f);

        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, 0.0f, disc.getY() + disc.getHeight() * 0.7f,
                                                 Palette::meat.withAlpha (0.3f), 0.0f, disc.getBottom(), false));
        g.fillPath (discPath);
    }

    // Указатель — точка у края диска.
    const auto dot = centre.getPointOnCircumference (discRadius - 6.5f, angle);
    g.setColour (Palette::meat);
    g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (dot));

    if (auto* knob = dynamic_cast<KnobSlider*> (&slider); knob != nullptr && knob->icon != nullptr)
        knob->icon->drawWithin (g, juce::Rectangle<float> (32.0f, 32.0f).withCentre (centre.translated (0.0f, -4.0f)),
                                juce::RectanglePlacement::centred, 1.0f);

    g.setColour (Palette::meat);
    g.setFont (fonts->manrope (11.0f, 800));
    g.drawText (slider.getTextFromValue (slider.getValue()),
                juce::Rectangle<float> (34.0f, 14.0f).withCentre (centre.translated (0.0f, 19.0f)),
                juce::Justification::centred, false);
}

void ShawarmaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    g.setColour (on ? juce::Colour (0xff2a1420) : Palette::panel.brighter (highlighted ? 0.08f : 0.0f));
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (on ? Palette::hot.withAlpha (0.55f) : juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    auto content = bounds.reduced (14.0f, 0.0f);
    const auto track = content.removeFromLeft (34.0f).withSizeKeepingCentre (34.0f, 20.0f);
    g.setColour (on ? Palette::hot : juce::Colour (0xff2a3463));
    g.fillRoundedRectangle (track, 10.0f);
    g.setColour (on ? juce::Colour (0xfffff8f0) : Palette::grey);
    g.fillEllipse (track.getX() + (on ? 17.0f : 3.0f), track.getY() + 3.0f, 14.0f, 14.0f);
    content.removeFromLeft (10.0f);

    juce::String mainText = button.getButtonText(), extraText;
    if (auto* twoPart = dynamic_cast<TwoPartToggle*> (&button))
    {
        mainText = twoPart->mainText;
        extraText = twoPart->extraText;
    }

    juce::AttributedString text;
    text.setJustification (juce::Justification::centredLeft);
    text.setWordWrap (juce::AttributedString::none);
    text.append (mainText, fonts->manrope (13.0f, extraText.isEmpty() ? 800 : 700), Palette::garlic);
    if (extraText.isNotEmpty())
        text.append (" " + extraText, fonts->manrope (13.0f, 400), on ? juce::Colour (0xffffb4aa) : Palette::muted);

    juce::TextLayout layout;
    layout.createLayout (text, content.getWidth());
    layout.draw (g, content);
}

void ShawarmaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& background,
                                                bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto colour = background;
    if (down)
        colour = colour.darker (0.15f);
    else if (highlighted)
        colour = colour.isTransparent() ? juce::Colours::white.withAlpha (0.05f) : colour.brighter (0.1f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 12.0f);

    if (button.getProperties()["outline"])
    {
        g.setColour (Palette::lavash.withAlpha (0.4f));
        g.drawRoundedRectangle (bounds, 12.0f, 1.0f);
    }
}

void ShawarmaLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const int weight = button.getProperties().getWithDefault ("weight", 700);
    g.setFont (fonts->manrope (13.0f, weight));
    g.setColour (button.findColour (juce::TextButton::textColourOffId));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

//==============================================================================
ToastStack::ToastStack()
{
    setInterceptsMouseClicks (false, false);
}

void ToastStack::add (const juce::String& from, const juce::String& text, juce::Colour accent,
                      double delaySeconds, double lifeSeconds)
{
    const double start = (lastNow > 0 ? lastNow : nowSeconds()) + delaySeconds;
    toasts.push_back ({ from, text, accent, start, start + lifeSeconds });

    // Больше пяти одновременно — уже спам, даже для Ninja Records.
    while (toasts.size() > 5)
        toasts.erase (toasts.begin());
}

void ToastStack::tick (double now)
{
    lastNow = now;
    toasts.erase (std::remove_if (toasts.begin(), toasts.end(), [now] (const Toast& t) { return now > t.hideAt; }),
                  toasts.end());
}

float ToastStack::heightFor (const Toast& toast) const
{
    constexpr float textAreaWidth = 296.0f - 28.0f - 32.0f - 12.0f;
    return textWidth (fonts->manrope (12.5f), toast.text) > textAreaWidth ? 74.0f : 56.0f;
}

void ToastStack::paint (juce::Graphics& g)
{
    constexpr float toastWidth = 296.0f, gap = 8.0f;
    float y = 76.0f;

    for (const auto& toast : toasts)
    {
        if (lastNow < toast.showAt)
            continue;

        const float appear = (float) juce::jlimit (0.0, 1.0, (lastNow - toast.showAt) / 0.25);
        const float vanish = (float) juce::jlimit (0.0, 1.0, (toast.hideAt - lastNow) / 0.4);
        const float alpha = juce::jmin (appear, vanish);
        const float slide = (1.0f - appear) * 40.0f;
        const float height = heightFor (toast);

        const auto card = juce::Rectangle<float> ((float) getWidth() - toastWidth - 14.0f + slide, y, toastWidth, height);
        juce::Path cardPath;
        cardPath.addRoundedRectangle (card, 14.0f);

        juce::DropShadow (juce::Colours::black.withAlpha (0.45f * alpha), 24, { 0, 10 }).drawForPath (g, cardPath);
        g.setColour (juce::Colour (0xff121834).withAlpha (0.97f * alpha));
        g.fillPath (cardPath);
        g.setColour (juce::Colours::white.withAlpha (0.08f * alpha));
        g.strokePath (cardPath, juce::PathStrokeType (1.0f));

        auto content = card.reduced (14.0f, 11.0f);
        auto avatar = content.removeFromLeft (32.0f).removeFromTop (32.0f);
        content.removeFromLeft (12.0f);

        g.setColour (toast.accent.withAlpha (alpha));
        g.fillEllipse (avatar);
        g.setColour (Palette::ground.withAlpha (alpha));
        g.setFont (fonts->unbounded (13.0f));
        g.drawText (toast.from.substring (0, 1).toUpperCase(), avatar, juce::Justification::centred, false);

        auto titleRow = content.removeFromTop (17.0f);
        g.setColour (Palette::muted.withAlpha (alpha));
        g.setFont (fonts->manrope (11.0f));
        g.drawText (utf8 ("сейчас"), titleRow.removeFromRight (44.0f), juce::Justification::centredRight, false);
        g.setColour (toast.accent.withAlpha (alpha));
        g.setFont (fonts->manrope (13.0f, 800));
        g.drawText (toast.from, titleRow, juce::Justification::centredLeft, true);

        content.removeFromTop (2.0f);
        g.setColour (Palette::garlic.withAlpha (alpha));
        g.setFont (fonts->manrope (12.5f));
        g.drawFittedText (toast.text, content.toNearestInt(), juce::Justification::topLeft, 2, 1.0f);

        y += (height + gap) * appear;
    }
}

//==============================================================================
VitalyKnob::VitalyKnob()
    : lockIcon (loadSvg (Icons::lock))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void VitalyKnob::paint (juce::Graphics& g)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    const float start = juce::MathConstants<float>::pi * 1.25f;
    const float end = juce::MathConstants<float>::pi * 2.75f;
    drawKnobRing (g, centre, start, end, start, {});

    const auto disc = juce::Rectangle<float> (66.0f, 66.0f).withCentre (centre);
    g.setColour (juce::Colour (0xff3a4366).brighter (isMouseOver() ? 0.12f : 0.0f));
    g.fillEllipse (disc);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (disc.reduced (0.5f), 1.0f);

    if (lockIcon != nullptr)
        lockIcon->drawWithin (g, juce::Rectangle<float> (22.0f, 26.0f).withCentre (centre), juce::RectanglePlacement::centred, 1.0f);
}

void VitalyKnob::mouseDown (const juce::MouseEvent&)
{
    if (onClick)
        onClick();
}

//==============================================================================
bool HitButton::hitTest (int x, int y)
{
    return getLocalBounds().reduced (glowX, glowY).contains (x, y);
}

void HitButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto body = getLocalBounds().reduced (glowX, glowY).toFloat();
    const auto top = done ? juce::Colour (0xff3fe08a) : Palette::sauceHi;
    const auto bottom = done ? juce::Colour (0xff17a85a) : Palette::sauceRed;
    const auto glow = done ? Palette::green : Palette::hot;

    if (down)
        body = body.reduced (2.0f);

    juce::Path bodyPath;
    bodyPath.addRoundedRectangle (body, 20.0f);

    // Кольцо-свечение пульсирует — кнопка просит, чтобы её нажали.
    g.setColour (glow.withAlpha (0.10f + 0.08f * pulse));
    g.fillRoundedRectangle (body.expanded (6.0f), 26.0f);
    juce::DropShadow (glow.withAlpha (0.35f), 30, { 0, 12 }).drawForPath (g, bodyPath);

    g.setGradientFill (juce::ColourGradient (top.brighter (highlighted ? 0.1f : 0.0f), 0.0f, body.getY(),
                                             bottom, 0.0f, body.getBottom(), false));
    g.fillPath (bodyPath);

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (bodyPath);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.fillRect (body.withHeight (2.0f));
    }

    const auto text = done ? juce::Colour (0xfff4fff8) : juce::Colour (0xfffff8f0);
    auto content = body.withSizeKeepingCentre (body.getWidth(), 48.0f);

    g.setColour (text);
    g.setFont (fonts->unbounded (27.0f).withExtraKerningFactor (0.02f));
    g.drawText (caption, content.removeFromTop (30.0f), juce::Justification::centred, false);

    content.removeFromTop (2.0f);
    g.setColour (text.withAlpha (done ? 0.9f : 0.85f));
    g.setFont (fonts->manrope (12.0f, 600));
    g.drawText (subcaption, content, juce::Justification::centred, false);
}

//==============================================================================
void CreditsOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.78f));

    auto panel = getLocalBounds().toFloat().reduced (60.0f, 28.0f);
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (panel, 16.0f);
    g.setColour (Palette::lavash);
    g.drawRoundedRectangle (panel, 16.0f, 1.5f);

    auto area = panel.reduced (32.0f, 18.0f);
    g.setColour (Palette::lavash);
    g.setFont (fonts->unbounded (22.0f));
    g.drawText (utf8 ("Титры"), area.removeFromTop (32.0f), juce::Justification::centred, false);
    g.setColour (Palette::muted);
    g.setFont (fonts->manrope (12.0f));
    g.drawText (utf8 ("шаВеRMA Hit Maker v1.0 · Melody Makers · Resonance Music Academy"),
                area.removeFromTop (20.0f), juce::Justification::centred, false);
    area.removeFromTop (10.0f);

    for (const auto& credit : Jokes::credits())
    {
        auto row = area.removeFromTop (37.0f);
        g.setColour (Palette::lavash);
        g.setFont (fonts->unbounded (13.0f, false));
        g.drawText (credit.name, row.removeFromLeft (140.0f).withHeight (18.0f), juce::Justification::centredLeft, false);
        g.setColour (Palette::garlic);
        g.setFont (fonts->manrope (13.0f));
        g.drawFittedText (credit.role, row.toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
    }

    g.setColour (Palette::muted.withAlpha (0.7f));
    g.setFont (fonts->manrope (12.0f));
    g.drawText (utf8 ("клик в любом месте — закрыть"), panel.reduced (0.0f, 10.0f), juce::Justification::centredBottom, false);
}

//==============================================================================
HitMakerEditor::HitMakerEditor (HitMakerProcessor& p)
    : AudioProcessorEditor (&p), hitProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    logo = juce::ImageCache::getFromMemory (HitMakerAssets::logo_png, HitMakerAssets::logo_pngSize);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        const auto& info = kKnobs[i];

        knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        knob.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (info.colour));
        knob.slider.setDoubleClickReturnValue (true, 0.0);
        knob.slider.icon = loadSvg (info.icon);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (hitProcessor.apvts, info.paramId, knob.slider);
        addAndMakeVisible (knob.slider);

        knob.title.setText (utf8 (info.title), juce::dontSendNotification);
        knob.title.setFont (fonts->unbounded (13.0f, false));
        knob.title.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (knob.title);

        knob.subtitle.setText (utf8 (info.subtitle), juce::dontSendNotification);
        knob.subtitle.setFont (fonts->manrope (11.0f));
        knob.subtitle.setColour (juce::Label::textColourId, Palette::muted);
        knob.subtitle.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (knob.subtitle);
    }

    vitalyKnob.onClick = [this]
    {
        toasts.add (utf8 ("Виталий (из архива)"), Jokes::vitalyWisdom (rng), Palette::grey, 0.0, 6.0);
    };
    addAndMakeVisible (vitalyKnob);

    for (auto* label : { &vitalyTitle, &vitalySubtitle })
    {
        label->setColour (juce::Label::textColourId, Palette::grey);
        label->setJustificationType (juce::Justification::centred);
        label->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }
    vitalyTitle.setText (utf8 ("Режим Виталия"), juce::dontSendNotification);
    vitalyTitle.setFont (fonts->unbounded (13.0f, false));
    vitalySubtitle.setText (utf8 ("вышел из группы"), juce::dontSendNotification);
    vitalySubtitle.setFont (fonts->manrope (11.0f));

    hitButton.onClick = [this] { onHitClicked(); };
    addAndMakeVisible (hitButton);

    stasToggle.mainText = utf8 ("Режим Стаса");
    stasToggle.extraText = utf8 ("+6 dB, чтобы было слышно");
    stasToggle.setButtonText (stasToggle.mainText);
    stasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (hitProcessor.apvts, ParamIDs::stas, stasToggle);
    addAndMakeVisible (stasToggle);

    hornToggle.mainText = "AIR HORN";
    hornToggle.setButtonText (hornToggle.mainText);
    hornAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (hitProcessor.apvts, ParamIDs::airhorn, hornToggle);
    addAndMakeVisible (hornToggle);

    ideaButton.setButtonText (utf8 ("Идея для Максима"));
    ideaButton.setColour (juce::TextButton::buttonColourId, Palette::lavash);
    ideaButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff3a1d0c));
    ideaButton.getProperties().set ("weight", 800);
    ideaButton.onClick = [this]
    {
        ideaText = Jokes::ideaForMaxim (rng);
        repaint();
    };
    addAndMakeVisible (ideaButton);

    creditsButton.setButtonText (utf8 ("Титры"));
    creditsButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    creditsButton.setColour (juce::TextButton::textColourOffId, Palette::lavash);
    creditsButton.getProperties().set ("outline", true);
    creditsButton.onClick = [this] { creditsOverlay.setVisible (true); };
    addAndMakeVisible (creditsButton);

    addAndMakeVisible (toasts);
    addChildComponent (creditsOverlay);

    setSize (kWidth, kHeight);

    lastTime = nowSeconds();
    nextVitalyCameo = lastTime + 25.0 + rng.nextDouble() * 20.0;
    refreshHitButton();
    startTimerHz (30);
}

HitMakerEditor::~HitMakerEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HitMakerEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::ground);

    paintHeader (g);
    fillPanel (g, juce::Rectangle<int> (kMargin, kPanelY, kWidth - 2 * kMargin, kPanelH).toFloat(), 14.0f);
    paintProgress (g);
    paintIdea (g);
}

void HitMakerEditor::paintHeader (juce::Graphics& g)
{
    // Логотип шаВеRMA в круге с синим свечением.
    const auto logoArea = juce::Rectangle<float> ((float) kMargin, 6.0f, 52.0f, 52.0f);
    juce::Path logoCircle;
    logoCircle.addEllipse (logoArea);
    juce::DropShadow (Palette::brandBlue.withAlpha (0.45f), 14, { 0, 4 }).drawForPath (g, logoCircle);

    if (logo.isValid())
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (logoCircle);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (logo, logoArea, juce::RectanglePlacement::fillDestination);
    }

    const float textX = logoArea.getRight() + 12.0f;
    const auto titleFont = fonts->unbounded (21.0f).withExtraKerningFactor (-0.01f);
    const auto brand = utf8 ("шаВеRMA");

    g.setFont (titleFont);
    g.setColour (Palette::garlic);
    g.drawText (brand, juce::Rectangle<float> (textX, 13.0f, 200.0f, 24.0f), juce::Justification::centredLeft, false);
    g.setColour (Palette::lavash);
    g.drawText ("HIT MAKER", juce::Rectangle<float> (textX + textWidth (titleFont, brand) + 9.0f, 13.0f, 200.0f, 24.0f),
                juce::Justification::centredLeft, false);

    g.setColour (Palette::muted);
    g.setFont (fonts->manrope (11.0f).withExtraKerningFactor (0.02f));
    g.drawText ("Resonance Music Academy " + utf8 ("·") + " Geniuses is at work here",
                juce::Rectangle<float> (textX, 38.0f, 400.0f, 14.0f), juce::Justification::centredLeft, false);

    // Справа: плашка группы и версия.
    const auto versionFont = fonts->manrope (11.0f, 600);
    const float versionWidth = textWidth (versionFont, "v1.0");
    const float right = (float) (kWidth - kMargin);
    g.setFont (versionFont);
    g.drawText ("v1.0", juce::Rectangle<float> (right - versionWidth, 22.0f, versionWidth, 20.0f), juce::Justification::centredRight, false);

    const auto pillFont = fonts->manrope (11.0f, 700);
    const float pillWidth = textWidth (pillFont, "Melody Makers") + 20.0f;
    const auto pill = juce::Rectangle<float> (right - versionWidth - 8.0f - pillWidth, 21.0f, pillWidth, 22.0f);
    g.setColour (Palette::lavash.withAlpha (0.35f));
    g.drawRoundedRectangle (pill.reduced (0.5f), 11.0f, 1.0f);
    g.setColour (Palette::lavash);
    g.setFont (pillFont);
    g.drawText ("Melody Makers", pill, juce::Justification::centred, false);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (juce::Rectangle<float> ((float) kMargin, (float) kHeaderHeight - 1.0f, (float) (kWidth - 2 * kMargin), 1.0f));
}

void HitMakerEditor::paintProgress (juce::Graphics& g)
{
    const auto bar = juce::Rectangle<float> ((float) (kWidth - kProgressW) * 0.5f, (float) kProgressY, (float) kProgressW, 10.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (bar, 5.0f);

    const bool hitOn = isHitOn();
    const float fill = cooking ? (float) cookProgress : (hitOn ? 1.0f : 0.0f);
    if (fill > 0.0f)
    {
        const auto filled = bar.withWidth (juce::jmax (bar.getHeight(), bar.getWidth() * fill));
        g.setGradientFill (juce::ColourGradient (Palette::lavash, filled.getX(), 0.0f,
                                                 cooking ? Palette::hot : Palette::green, filled.getRight(), 0.0f, false));
        g.fillRoundedRectangle (filled, 5.0f);
    }

    juce::String status;
    if (cooking)
        status = stageText;
    else if (hitOn)
        status = utf8 ("Готово! Проверь уведомления. Лейблы уже пишут.");
    else
        status = utf8 ("Шаурма разогрета. Знания не требуются — просто жми.");

    const bool emphasised = cooking || hitOn;
    g.setColour (emphasised ? Palette::garlic : Palette::muted);
    g.setFont (fonts->manrope (13.0f, emphasised ? 600 : 400));
    g.drawText (status, juce::Rectangle<float> ((float) kMargin, bar.getBottom() + 9.0f, (float) (kWidth - 2 * kMargin), 18.0f),
                juce::Justification::centred, true);
}

void HitMakerEditor::paintIdea (juce::Graphics& g)
{
    const auto area = juce::Rectangle<int> (kMargin, kIdeaY, kWidth - 2 * kMargin, kIdeaH).toFloat();
    fillPanel (g, area, 14.0f);

    auto inner = area.reduced (16.0f, 10.0f);
    g.setColour (Palette::lavash);
    g.setFont (fonts->manrope (10.0f, 800).withExtraKerningFactor (0.12f));
    g.drawText (utf8 ("ГЕНЕРАТОР ИДЕЙ ДЛЯ МАКСИМА"), inner.removeFromTop (14.0f), juce::Justification::centredLeft, false);

    inner.removeFromTop (4.0f);
    g.setColour (Palette::garlic);
    g.setFont (fonts->manrope (13.0f));
    g.drawFittedText (ideaText.isEmpty() ? utf8 ("Идеи не придумываются? Бывает со всеми. Жми «Идея для Максима» — одна кнопка, как и всё здесь.")
                                         : ideaText,
                      inner.toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
}

void HitMakerEditor::resized()
{
    // Сетка крутилок: 6 колонок внутри панели.
    const int innerX = kMargin + 6, innerW = kWidth - 2 * kMargin - 12, gap = 4;
    const int columnW = (innerW - 5 * gap) / 6;
    const int knobTop = kPanelY + 14;

    auto layoutColumn = [&] (int index, juce::Component& knob, juce::Label& title, juce::Label& subtitle)
    {
        const int x = innerX + index * (columnW + gap);
        knob.setBounds (x + (columnW - kKnobSize) / 2, knobTop, kKnobSize, kKnobSize);
        title.setBounds (x - 4, knobTop + kKnobSize + 6, columnW + 8, 18);
        subtitle.setBounds (x - 4, knobTop + kKnobSize + 24, columnW + 8, 15);
    };

    for (size_t i = 0; i < knobs.size(); ++i)
        layoutColumn ((int) i, knobs[i].slider, knobs[i].title, knobs[i].subtitle);
    layoutColumn (5, vitalyKnob, vitalyTitle, vitalySubtitle);

    hitButton.setBounds ((kWidth - kButtonW) / 2 - HitButton::glowX, kButtonY - HitButton::glowY,
                         kButtonW + 2 * HitButton::glowX, kButtonH + 2 * HitButton::glowY);

    // Нижний ряд: тумблер Стаса тянется, остальное фиксированной ширины.
    auto row = juce::Rectangle<int> (kMargin, kRowY, kWidth - 2 * kMargin, kRowH);
    creditsButton.setBounds (row.removeFromRight (84));
    row.removeFromRight (10);
    ideaButton.setBounds (row.removeFromRight (160));
    row.removeFromRight (10);
    hornToggle.setBounds (row.removeFromRight (132));
    row.removeFromRight (10);
    stasToggle.setBounds (row);

    toasts.setBounds (getLocalBounds());
    creditsOverlay.setBounds (getLocalBounds());
}

void HitMakerEditor::timerCallback()
{
    const double now = nowSeconds();
    const double dt = juce::jlimit (0.0, 0.2, now - lastTime);
    lastTime = now;
    advance (now, dt);
}

void HitMakerEditor::advance (double now, double dt)
{
    pulsePhase += (float) dt;

    if (cooking)
    {
        cookProgress += dt / cookDuration;
        const int stage = juce::jlimit (0, cookStages.size() - 1, (int) (cookProgress * cookStages.size()));
        stageText = cookStages[stage];

        if (cookProgress >= 1.0)
            finishCooking();
    }

    if (now >= nextVitalyCameo)
        scheduleVitalyCameo (now);

    toasts.tick (now);
    refreshHitButton();
    repaint();
}

void HitMakerEditor::scheduleVitalyCameo (double now)
{
    toasts.add (utf8 ("Виталий"), utf8 ("печатает…"), Palette::grey, 0.0, 3.5);
    toasts.add ("Telegram", utf8 ("Виталий покинул группу"), Palette::grey, 4.0, 4.5);
    nextVitalyCameo = now + 70.0 + rng.nextDouble() * 80.0;
}

#if HITMAKER_SELFTEST
void HitMakerEditor::debugAdvance (double seconds)
{
    const double step = 1.0 / 30.0;
    for (double t = 0; t < seconds; t += step)
    {
        lastTime += step;
        advance (lastTime, step);
    }
}
#endif

bool HitMakerEditor::isHitOn() const
{
    return hitProcessor.apvts.getRawParameterValue (ParamIDs::hit)->load() > 0.5f;
}

void HitMakerEditor::setHitParam (bool on)
{
    if (auto* param = hitProcessor.apvts.getParameter (ParamIDs::hit))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (on ? 1.0f : 0.0f);
        param->endChangeGesture();
    }
}

void HitMakerEditor::onHitClicked()
{
    if (cooking)
        return;

    if (isHitOn())
    {
        setHitParam (false);
        toasts.add (utf8 ("Шаурмичная"), utf8 ("Хит разобран обратно на ингредиенты. Лаваш вернули."), Palette::lavash);
        return;
    }

    startCooking();
}

void HitMakerEditor::startCooking()
{
    cookStages = Jokes::cookingStages (rng, 7);
    cookProgress = 0.0;
    stageText = cookStages[0];
    cooking = true;
}

void HitMakerEditor::finishCooking()
{
    cooking = false;
    cookProgress = 0.0;
    setHitParam (true);

    if (hitProcessor.apvts.getRawParameterValue (ParamIDs::airhorn)->load() > 0.5f)
        hitProcessor.triggerAirHorn();

    // Первые два — всегда Sonic Music и Ninja Records, у остальных свои цвета по очереди.
    const juce::Colour accents[] = { Palette::lavash, Palette::sauceHi, Palette::green, Palette::skyBlue };
    double delay = 0.2;
    int index = 0;
    for (const auto& n : Jokes::successNotifications (rng, 4))
    {
        const auto accent = index < 4 ? accents[index] : toastAccent (n.from);
        toasts.add (n.from, n.text, accent, delay, 6.5);
        delay += 0.8;
        ++index;
    }
}

void HitMakerEditor::refreshHitButton()
{
    const bool hitOn = isHitOn();

    if (cooking)
    {
        hitButton.caption = utf8 ("ГОТОВИМ ХИТ…");
        hitButton.subcaption = juce::String (juce::roundToInt (cookProgress * 100.0)) + utf8 (" % · не выключайте шаурмичную");
    }
    else if (hitOn)
    {
        hitButton.caption = utf8 ("ХИТ ГОТОВ");
        hitButton.subcaption = utf8 ("нажми ещё раз, чтобы разобрать обратно");
    }
    else
    {
        hitButton.caption = utf8 ("СДЕЛАТЬ ХИТ");
        hitButton.subcaption = utf8 ("нажал кнопочку — и готово");
    }

    hitButton.done = hitOn && ! cooking;
    hitButton.pulse = 0.5f + 0.5f * std::sin (pulsePhase * 4.0f);
}
