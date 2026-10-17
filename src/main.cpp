// Wires the HTTP routes: the common contract plus the routes of theme 1 (contacts book).

#include <chrono>
#include <iostream>
#include <optional>
#include <string>

#include <httplib.h>

#include "app.hpp"
#include "contact_api.hpp"

namespace {

void SendJson(httplib::Response& response, int status, const nlohmann::json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json; charset=utf-8");
}

void SendNotFound(httplib::Response& response, const std::string& message) {
    SendJson(response, 404, map_project::ErrorBody("not_found", message));
}

/// The route pattern only lets digits through, so the only problem left is a huge number.
std::optional<int> ParseId(const std::string& text) {
    if (text.empty() || text.size() > 9) {
        return std::nullopt;
    }
    return std::stoi(text);
}

}  // namespace

int main() {
    using namespace map_project;

    const auto started_at = std::chrono::steady_clock::now();
    Store store;
    httplib::Server server;

    // ---- Common contract ----

    server.Get("/health", [&](const httplib::Request&, httplib::Response& response) {
        response.set_content(HealthBody(started_at).dump(), "application/json; charset=utf-8");
    });

    server.Get("/version", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(VersionBody().dump(), "application/json; charset=utf-8");
    });

    server.Post("/reset", [&](const httplib::Request&, httplib::Response& response) {
        store.Reset();
        response.status = 204;
    });

    server.Get("/", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(HomePage(), "text/html; charset=utf-8");
    });

    // ---- Theme 1: contacts book ----

    server.Post("/contacts", [&](const httplib::Request& request, httplib::Response& response) {
        const ParsedContact parsed = ParseContact(request.body);
        if (!parsed.ok) {
            SendJson(response, 400, ErrorBody("validation", parsed.message));
            return;
        }
        const auto added = store.AddContact(parsed.contact);
        if (!added) {
            SendJson(response, 409, ErrorBody("conflict", "a contact with this email already exists"));
            return;
        }
        SendJson(response, 201, ContactToJson(*added));
    });

    server.Get("/contacts", [&](const httplib::Request& request, httplib::Response& response) {
        std::optional<std::string> category;
        std::optional<std::string> query;
        if (request.has_param("category")) {
            category = request.get_param_value("category");
        }
        if (request.has_param("q")) {
            query = request.get_param_value("q");
        }
        SendJson(response, 200, ContactsToJson(store.ListContacts(category, query)));
    });

    server.Get(R"(/contacts/(\d+))", [&](const httplib::Request& request, httplib::Response& response) {
        const auto id = ParseId(request.matches[1].str());
        const auto contact = id ? store.GetContact(*id) : std::nullopt;
        if (!contact) {
            SendNotFound(response, "contact not found");
            return;
        }
        SendJson(response, 200, ContactToJson(*contact));
    });

    server.Delete(R"(/contacts/(\d+))", [&](const httplib::Request& request, httplib::Response& response) {
        const auto id = ParseId(request.matches[1].str());
        if (!id || !store.RemoveContact(*id)) {
            SendNotFound(response, "contact not found");
            return;
        }
        response.status = 204;
    });

    server.Get("/stats", [&](const httplib::Request&, httplib::Response& response) {
        SendJson(response, 200, StatsToJson(store.Stats()));
    });

    // Unknown routes get the standard error body; routes that already set one keep theirs.
    server.set_error_handler([](const httplib::Request&, httplib::Response& response) {
        if (response.status == 404 && response.body.empty()) {
            response.set_content(ErrorBody("not_found", "route does not exist").dump(),
                                 "application/json; charset=utf-8");
        }
    });

    std::cout << kAppName << " " << kAppVersion << " starting on port 8080" << std::endl;
    if (!server.listen("0.0.0.0", 8080)) {
        std::cerr << "failed to bind port 8080" << std::endl;
        return 1;
    }
    return 0;
}