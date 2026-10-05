// Contact model and validation rules for theme 1 (contacts book).
// Pure logic, no HTTP, so it can be unit tested without starting a server.
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <string>

namespace map_project {

inline constexpr std::size_t kMaxNameLength = 50;  // parameter L from the assignment
inline constexpr std::size_t kPhoneDigits = 10;

struct Contact {
    int id = 0;
    std::string name;
    std::string email;  // kept exactly as the user typed it
    std::string phone;
    std::string category;
};

/// Counts characters, not bytes: UTF-8 continuation bytes (10xxxxxx) are skipped,
/// so a name with diacritics is not penalised.
inline std::size_t CharacterCount(const std::string& text) {
    return static_cast<std::size_t>(
        std::count_if(text.begin(), text.end(),
                      [](unsigned char c) { return (c & 0xC0) != 0x80; }));
}

/// Between 1 and kMaxNameLength characters, both ends included.
inline bool IsValidName(const std::string& name) {
    const std::size_t length = CharacterCount(name);
    return length >= 1 && length <= kMaxNameLength;
}

/// Exactly one '@', some text before it and at least one '.' after it.
inline bool IsValidEmail(const std::string& email) {
    if (std::count(email.begin(), email.end(), '@') != 1) return false;
    const std::size_t at = email.find('@');
    if (at == 0) return false;
    return email.find('.', at + 1) != std::string::npos;
}

/// Exactly kPhoneDigits digits, optionally preceded by a single '+'.
inline bool IsValidPhone(const std::string& phone) {
    const std::size_t start = (!phone.empty() && phone[0] == '+') ? 1 : 0;
    if (phone.size() - start != kPhoneDigits) return false;
    return std::all_of(phone.begin() + static_cast<std::ptrdiff_t>(start), phone.end(),
                       [](unsigned char c) { return std::isdigit(c) != 0; });
}

inline bool IsValidCategory(const std::string& category) {
    static const std::array<const char*, 4> allowed = {"family", "friends", "work", "other"};
    return std::any_of(allowed.begin(), allowed.end(),
                       [&](const char* value) { return category == value; });
}

/// Lower-cased copy, used only to compare emails. Never stored in place of the original.
inline std::string NormalizeEmail(std::string email) {
    std::transform(email.begin(), email.end(), email.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return email;
}

}  // namespace map_project
