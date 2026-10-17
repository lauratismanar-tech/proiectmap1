// JSON <-> contact conversion for the HTTP layer (theme 1).
// Kept out of main.cpp so it can be unit tested without starting a server.
#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contacts.hpp"
#include "store.hpp"

namespace map_project {

/// Result of reading and validating the body of POST /contacts.
struct ParsedContact {
    bool ok = false;
    Contact contact;
    std::string message;  // why the body was rejected, when ok is false
};

inline ParsedContact ParseContact(const std::string& body) {
    ParsedContact result;

    const nlohmann::json json = nlohmann::json::parse(body, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        result.message = "body must be a valid JSON object";
        return result;
    }

    // Reads one field that must be present and must be a string.
    const auto read_text = [&](const char* key, std::string& out) -> bool {
        const auto found = json.find(key);
        if (found == json.end() || !found->is_string()) {
            result.message = std::string(key) + " is required and must be a string";
            return false;
        }
        out = found->get<std::string>();
        return true;
    };

    Contact contact;
    if (!read_text("name", contact.name) || !read_text("email", contact.email) ||
        !read_text("phone", contact.phone) || !read_text("category", contact.category)) {
        return result;
    }

    if (!IsValidName(contact.name)) {
        result.message = "name must have between 1 and " + std::to_string(kMaxNameLength) +
                         " characters";
    } else if (!IsValidEmail(contact.email)) {
        result.message = "email must contain one @, text before it and a dot after it";
    } else if (!IsValidPhone(contact.phone)) {
        result.message = "phone must have exactly 10 digits, optionally preceded by +";
    } else if (!IsValidCategory(contact.category)) {
        result.message = "category must be one of: family, friends, work, other";
    } else {
        result.ok = true;
        result.contact = contact;
    }
    return result;
}

inline nlohmann::json ContactToJson(const Contact& contact) {
    return {{"id", contact.id},
            {"name", contact.name},
            {"email", contact.email},
            {"phone", contact.phone},
            {"category", contact.category}};
}

inline nlohmann::json ContactsToJson(const std::vector<Contact>& contacts) {
    nlohmann::json list = nlohmann::json::array();
    for (const auto& contact : contacts) {
        list.push_back(ContactToJson(contact));
    }
    return list;
}

inline nlohmann::json StatsToJson(const ContactStats& stats) {
    nlohmann::json by_category = nlohmann::json::object();
    for (const auto& entry : stats.by_category) {
        by_category[entry.first] = entry.second;
    }
    return {{"total", stats.total}, {"by_category", by_category}};
}

}  // namespace map_project