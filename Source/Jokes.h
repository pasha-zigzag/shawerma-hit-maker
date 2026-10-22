#pragma once

#include <juce_core/juce_core.h>

// Все тексты плагина. Строки в UTF-8.
namespace Jokes
{
struct Notification
{
    juce::String from;
    juce::String text;
};

struct Credit
{
    juce::String name;
    juce::String role;
};

juce::StringArray cookingStages (juce::Random& rng, int count);
std::vector<Notification> successNotifications (juce::Random& rng, int count);
juce::String vitalyWisdom (juce::Random& rng);
juce::String ideaForMaxim (juce::Random& rng);
std::vector<Credit> credits();
} // namespace Jokes
