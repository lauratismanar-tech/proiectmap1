#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <chrono>

#include "app.hpp"
#include "contacts.hpp"
#include "contact_api.hpp"

using namespace map_project;

TEST_CASE("health body reports ok and uptime") {
    const auto started_at = std::chrono::steady_clock::now();
    const auto body = HealthBody(started_at);

    CHECK(body["status"] == "ok");
    CHECK(body.contains("uptime_seconds"));
    CHECK(body["uptime_seconds"].get<long>() >= 0);
}

TEST_CASE("version body contains all required fields") {
    const auto body = VersionBody();

    CHECK(body["app"] == kAppName);
    CHECK(body["version"] == kAppVersion);
    CHECK(body.contains("commit"));
    CHECK(body.contains("built_at"));
}

TEST_CASE("env falls back when the variable is missing") {
    CHECK(Env("MAP_VARIABLE_THAT_DOES_NOT_EXIST", "fallback") == "fallback");
}

TEST_CASE("error body has the standard shape") {
    const auto body = ErrorBody("not_found", "route does not exist");

    CHECK(body["error"] == "not_found");
    CHECK(body["message"] == "route does not exist");
}

TEST_CASE("new store starts at one") {
    Store store;
    CHECK(store.NextId() == 1);
}

TEST_CASE("take next id increments") {
    Store store;

    CHECK(store.TakeNextId() == 1);
    CHECK(store.TakeNextId() == 2);
    CHECK(store.NextId() == 3);
}

TEST_CASE("reset returns to one") {
    Store store;
    store.TakeNextId();
    store.TakeNextId();

    store.Reset();

    CHECK(store.NextId() == 1);
}

TEST_CASE("home page mentions the application name") {
    CHECK(HomePage().find(kAppName) != std::string::npos);
}

// ---- Contact validation (theme 1) ----

TEST_CASE("accepts_name_of_exactly_max_length") {
    CHECK(IsValidName(std::string(kMaxNameLength, 'a')));
}

TEST_CASE("rejects_name_one_character_over_max_length") {
    CHECK_FALSE(IsValidName(std::string(kMaxNameLength + 1, 'a')));
}

TEST_CASE("rejects_empty_name") {
    CHECK_FALSE(IsValidName(""));
}

TEST_CASE("counts_diacritics_as_one_character_each") {
    std::string name;
    for (std::size_t i = 0; i < kMaxNameLength; ++i) name += "\xC4\x83";  // 'a with breve', 2 bytes
    CHECK(IsValidName(name));
}

TEST_CASE("accepts_well_formed_email") {
    CHECK(IsValidEmail("ana@example.com"));
}

TEST_CASE("rejects_email_without_at_sign") {
    CHECK_FALSE(IsValidEmail("anaexample.com"));
}

TEST_CASE("rejects_email_with_two_at_signs") {
    CHECK_FALSE(IsValidEmail("ana@@example.com"));
}

TEST_CASE("rejects_email_with_nothing_before_at_sign") {
    CHECK_FALSE(IsValidEmail("@example.com"));
}

TEST_CASE("rejects_email_without_dot_after_at_sign") {
    CHECK_FALSE(IsValidEmail("ana@example"));
}

TEST_CASE("accepts_phone_with_exactly_ten_digits") {
    CHECK(IsValidPhone("0722334455"));
}

TEST_CASE("accepts_phone_with_leading_plus") {
    CHECK(IsValidPhone("+0722334455"));
}

TEST_CASE("rejects_phone_with_nine_digits") {
    CHECK_FALSE(IsValidPhone("072233445"));
}

TEST_CASE("rejects_phone_with_eleven_digits") {
    CHECK_FALSE(IsValidPhone("07223344556"));
}

TEST_CASE("rejects_phone_containing_letters") {
    CHECK_FALSE(IsValidPhone("07223344a5"));
}

TEST_CASE("accepts_every_allowed_category") {
    CHECK(IsValidCategory("family"));
    CHECK(IsValidCategory("friends"));
    CHECK(IsValidCategory("work"));
    CHECK(IsValidCategory("other"));
}

TEST_CASE("rejects_unknown_category") {
    CHECK_FALSE(IsValidCategory("colleagues"));
}

TEST_CASE("normalizes_email_to_lower_case_for_comparison") {
    CHECK(NormalizeEmail("Ana@Example.COM") == "ana@example.com");
}

// ---- Contact store (theme 1) ----

namespace {

Contact MakeContact(const std::string& name, const std::string& email,
                    const std::string& category = "work") {
    Contact contact;
    contact.name = name;
    contact.email = email;
    contact.phone = "0722334455";
    contact.category = category;
    return contact;
}

}  // namespace

TEST_CASE("assigns_id_one_to_first_contact") {
    Store store;
    const auto added = store.AddContact(MakeContact("Ana Pop", "ana@example.com"));

    REQUIRE(added.has_value());
    CHECK(added->id == 1);
}

TEST_CASE("assigns_consecutive_ids") {
    Store store;
    store.AddContact(MakeContact("Ana", "ana@example.com"));
    const auto second = store.AddContact(MakeContact("Bob", "bob@example.com"));

    REQUIRE(second.has_value());
    CHECK(second->id == 2);
}

TEST_CASE("rejects_duplicate_email_ignoring_case") {
    Store store;
    store.AddContact(MakeContact("Ana Pop", "ana@example.com"));

    CHECK_FALSE(store.AddContact(MakeContact("Ana P.", "Ana@Example.COM")).has_value());
}

TEST_CASE("duplicate_email_does_not_consume_an_id") {
    Store store;
    store.AddContact(MakeContact("Ana", "ana@example.com"));
    store.AddContact(MakeContact("Ana again", "ANA@example.com"));

    const auto next = store.AddContact(MakeContact("Bob", "bob@example.com"));

    REQUIRE(next.has_value());
    CHECK(next->id == 2);
}

TEST_CASE("keeps_email_as_typed_by_the_user") {
    Store store;
    store.AddContact(MakeContact("Ana", "Ana@Example.com"));

    CHECK(store.GetContact(1)->email == "Ana@Example.com");
}

TEST_CASE("get_returns_nothing_for_unknown_id") {
    Store store;
    CHECK_FALSE(store.GetContact(999).has_value());
}

TEST_CASE("remove_deletes_an_existing_contact") {
    Store store;
    store.AddContact(MakeContact("Ana", "ana@example.com"));

    CHECK(store.RemoveContact(1));
    CHECK_FALSE(store.GetContact(1).has_value());
}

TEST_CASE("remove_reports_false_for_unknown_id") {
    Store store;
    CHECK_FALSE(store.RemoveContact(42));
}

TEST_CASE("list_is_empty_for_a_new_store") {
    Store store;
    CHECK(store.ListContacts(std::nullopt, std::nullopt).empty());
}

TEST_CASE("list_is_sorted_by_name") {
    Store store;
    store.AddContact(MakeContact("Maria", "m@example.com"));
    store.AddContact(MakeContact("Ana", "a@example.com"));

    const auto list = store.ListContacts(std::nullopt, std::nullopt);

    REQUIRE(list.size() == 2);
    CHECK(list[0].name == "Ana");
    CHECK(list[1].name == "Maria");
}

TEST_CASE("list_breaks_name_ties_by_ascending_id") {
    Store store;
    store.AddContact(MakeContact("Ana", "first@example.com"));
    store.AddContact(MakeContact("Ana", "second@example.com"));

    const auto list = store.ListContacts(std::nullopt, std::nullopt);

    REQUIRE(list.size() == 2);
    CHECK(list[0].id == 1);
    CHECK(list[1].id == 2);
}

TEST_CASE("list_filters_by_category") {
    Store store;
    store.AddContact(MakeContact("Ana", "a@example.com", "work"));
    store.AddContact(MakeContact("Bob", "b@example.com", "family"));

    const auto list = store.ListContacts(std::string("family"), std::nullopt);

    REQUIRE(list.size() == 1);
    CHECK(list[0].name == "Bob");
}

TEST_CASE("list_search_ignores_case_and_matches_inside_the_name") {
    Store store;
    store.AddContact(MakeContact("Ana Pop", "a@example.com"));
    store.AddContact(MakeContact("Bob", "b@example.com"));

    const auto list = store.ListContacts(std::nullopt, std::string("NA P"));

    REQUIRE(list.size() == 1);
    CHECK(list[0].name == "Ana Pop");
}

TEST_CASE("list_applies_category_and_search_together") {
    Store store;
    store.AddContact(MakeContact("Ana Pop", "a@example.com", "work"));
    store.AddContact(MakeContact("Ana Ionescu", "b@example.com", "friends"));

    const auto list = store.ListContacts(std::string("work"), std::string("ana"));

    REQUIRE(list.size() == 1);
    CHECK(list[0].name == "Ana Pop");
}

TEST_CASE("stats_count_contacts_per_category") {
    Store store;
    store.AddContact(MakeContact("Ana", "a@example.com", "work"));
    store.AddContact(MakeContact("Bob", "b@example.com", "work"));
    store.AddContact(MakeContact("Cora", "c@example.com", "family"));

    const auto stats = store.Stats();

    CHECK(stats.total == 3);
    CHECK(stats.by_category.at("work") == 2);
    CHECK(stats.by_category.at("family") == 1);
}

TEST_CASE("stats_are_empty_for_a_new_store") {
    Store store;
    const auto stats = store.Stats();

    CHECK(stats.total == 0);
    CHECK(stats.by_category.empty());
}

TEST_CASE("reset_clears_contacts_and_restarts_ids") {
    Store store;
    store.AddContact(MakeContact("Ana", "a@example.com"));

    store.Reset();

    CHECK(store.Stats().total == 0);
    CHECK(store.AddContact(MakeContact("Bob", "b@example.com"))->id == 1);
}

// ---- HTTP layer: contact parsing and JSON (theme 1) ----

namespace {

std::string ContactBody(const std::string& name, const std::string& email,
                        const std::string& phone, const std::string& category) {
    return nlohmann::json{{"name", name},
                          {"email", email},
                          {"phone", phone},
                          {"category", category}}
        .dump();
}

}  // namespace

TEST_CASE("parses_a_valid_contact_body") {
    const auto parsed =
        ParseContact(ContactBody("Ana Pop", "ana@example.com", "0722334455", "work"));

    REQUIRE(parsed.ok);
    CHECK(parsed.contact.name == "Ana Pop");
    CHECK(parsed.contact.email == "ana@example.com");
    CHECK(parsed.contact.category == "work");
}

TEST_CASE("rejects_body_that_is_not_json") {
    CHECK_FALSE(ParseContact("this is not json").ok);
}

TEST_CASE("rejects_json_that_is_not_an_object") {
    CHECK_FALSE(ParseContact("[1, 2, 3]").ok);
}

TEST_CASE("rejects_body_with_a_missing_field") {
    const std::string body = R"json({"name":"Ana","phone":"0722334455","category":"work"})json";
    CHECK_FALSE(ParseContact(body).ok);
}

TEST_CASE("rejects_phone_sent_as_a_number") {
    const std::string body =
        R"json({"name":"Ana","email":"ana@example.com","phone":722334455,"category":"work"})json";
    CHECK_FALSE(ParseContact(body).ok);
}

TEST_CASE("rejects_body_with_a_name_over_the_limit") {
    const auto body = ContactBody(std::string(kMaxNameLength + 1, 'a'), "ana@example.com",
                                  "0722334455", "work");
    CHECK_FALSE(ParseContact(body).ok);
}

TEST_CASE("rejects_body_with_an_invalid_email") {
    CHECK_FALSE(ParseContact(ContactBody("Ana", "ana-at-example", "0722334455", "work")).ok);
}

TEST_CASE("rejects_body_with_an_invalid_phone") {
    CHECK_FALSE(ParseContact(ContactBody("Ana", "ana@example.com", "12345", "work")).ok);
}

TEST_CASE("rejects_body_with_an_unknown_category") {
    CHECK_FALSE(ParseContact(ContactBody("Ana", "ana@example.com", "0722334455", "boss")).ok);
}

TEST_CASE("contact_json_has_all_fields") {
    Contact contact;
    contact.id = 7;
    contact.name = "Ana";
    contact.email = "Ana@Example.com";
    contact.phone = "0722334455";
    contact.category = "work";

    const auto body = ContactToJson(contact);

    CHECK(body["id"] == 7);
    CHECK(body["name"] == "Ana");
    CHECK(body["email"] == "Ana@Example.com");
    CHECK(body["phone"] == "0722334455");
    CHECK(body["category"] == "work");
}

TEST_CASE("stats_json_has_an_empty_object_for_a_new_store") {
    Store store;
    const auto body = StatsToJson(store.Stats());

    CHECK(body["total"] == 0);
    CHECK(body["by_category"].is_object());
    CHECK(body["by_category"].empty());
}

TEST_CASE("stats_json_reports_total_and_categories") {
    Store store;
    store.AddContact(MakeContact("Ana", "a@example.com", "work"));
    store.AddContact(MakeContact("Bob", "b@example.com", "work"));

    const auto body = StatsToJson(store.Stats());

    CHECK(body["total"] == 2);
    CHECK(body["by_category"]["work"] == 2);
}