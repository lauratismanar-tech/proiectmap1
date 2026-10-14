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