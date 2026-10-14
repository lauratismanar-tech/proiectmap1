// In-memory storage for the contacts book (theme 1).
// Pure logic, no HTTP, so it can be unit tested without starting a server.
#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "contacts.hpp"

namespace map_project {

struct ContactStats {
    int total = 0;
    std::map<std::string, int> by_category;  // only categories that have contacts
};

/// Holds the application data in memory.
/// Everything is lost when the container restarts.
class Store {
public:
    void Reset() {
        std::lock_guard<std::mutex> guard(mutex_);
        next_id_ = 1;
        contacts_.clear();
    }

    int NextId() {
        std::lock_guard<std::mutex> guard(mutex_);
        return next_id_;
    }

    int TakeNextId() {
        std::lock_guard<std::mutex> guard(mutex_);
        return next_id_++;
    }

    /// Stores a contact that was already validated and returns it with its new id.
    /// Returns nullopt, without consuming an id, when the email is already used.
    /// The comparison ignores case, but the email is stored as the user typed it.
    std::optional<Contact> AddContact(Contact contact) {
        std::lock_guard<std::mutex> guard(mutex_);
        const std::string key = NormalizeEmail(contact.email);
        for (const auto& entry : contacts_) {
            if (NormalizeEmail(entry.second.email) == key) {
                return std::nullopt;
            }
        }
        contact.id = next_id_++;
        contacts_[contact.id] = contact;
        return contact;
    }

    std::optional<Contact> GetContact(int id) const {
        std::lock_guard<std::mutex> guard(mutex_);
        const auto found = contacts_.find(id);
        if (found == contacts_.end()) {
            return std::nullopt;
        }
        return found->second;
    }

    /// Returns true if the contact existed and was removed.
    bool RemoveContact(int id) {
        std::lock_guard<std::mutex> guard(mutex_);
        return contacts_.erase(id) > 0;
    }

    /// Contacts sorted by name ascending, ties broken by id ascending.
    /// When both filters are present, both are applied.
    /// The name search is a case-insensitive "contains".
    std::vector<Contact> ListContacts(const std::optional<std::string>& category,
                                      const std::optional<std::string>& query) const {
        std::lock_guard<std::mutex> guard(mutex_);
        const std::string needle = query ? Lower(*query) : std::string();

        std::vector<Contact> result;
        for (const auto& entry : contacts_) {
            const Contact& contact = entry.second;
            if (category && contact.category != *category) {
                continue;
            }
            if (query && Lower(contact.name).find(needle) == std::string::npos) {
                continue;
            }
            result.push_back(contact);
        }
        std::sort(result.begin(), result.end(), [](const Contact& a, const Contact& b) {
            if (a.name != b.name) {
                return a.name < b.name;
            }
            return a.id < b.id;
        });
        return result;
    }

    ContactStats Stats() const {
        std::lock_guard<std::mutex> guard(mutex_);
        ContactStats stats;
        stats.total = static_cast<int>(contacts_.size());
        for (const auto& entry : contacts_) {
            ++stats.by_category[entry.second.category];
        }
        return stats;
    }

private:
    static std::string Lower(std::string text) {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    mutable std::mutex mutex_;
    int next_id_ = 1;
    std::map<int, Contact> contacts_;
};

}  // namespace map_project