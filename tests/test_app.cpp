#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <chrono>

#include "app.hpp"
#include "contacts.hpp"

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
