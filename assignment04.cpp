#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

class MonopolyBoard {
public:
    struct Node {
        std::string name;
        int cost;
        std::string owner; // Empty means unowned.
        Node* next;
    };
private:
    Node* tail = nullptr; // tail->next is the first space.
    std::size_t count = 0;
public:
    MonopolyBoard() = default;
    // Prevent shallow copies of owning pointers.
    MonopolyBoard(const MonopolyBoard&) = delete;
    MonopolyBoard& operator=(const MonopolyBoard&) = delete;
    ~MonopolyBoard() {
        if (!tail) return;
        Node* current = tail->next;
        tail->next = nullptr; // Break the circle before deleting nodes.
        while (current) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }
    std::size_t size() const { return count; }
    const Node* first() const { return tail ? tail->next : nullptr; }

    // Names must be unique and nonempty; costs must be nonnegative.
    bool insert(const std::string& name, int cost) {
        if (name.empty() || cost < 0 || search(name)) return false;
        Node* node = new Node{name, cost, "", nullptr};
        if (!tail) node->next = node;
        else {
            node->next = tail->next;
            tail->next = node;
        }
        tail = node;
        ++count;
        return true;
    }
    const Node* search(const std::string& name) const {
        if (!tail) return nullptr;
        const Node* current = tail->next;
        do {
            if (current->name == name) return current;
            current = current->next;
        } while (current != tail->next);
        return nullptr;
    }
    bool remove(const std::string& name) {
        if (!tail) return false;
        Node* previous = tail;
        Node* current = tail->next;
        do {
            if (current->name == name) {
                if (current == previous) tail = nullptr;
                else {
                    previous->next = current->next;
                    if (current == tail) tail = previous;
                }
                delete current;
                --count;
                return true;
            }
            previous = current;
            current = current->next;
        } while (current != tail->next);
        return false;
    }
    // start must refer to a current node in this board (or be null).
    const Node* move(const Node* start, std::size_t steps) const {
        if (!tail || !start) return nullptr;
        steps %= count; // Whole laps return to the same space.
        while (steps-- > 0) start = start->next;
        return start;
    }
    bool purchase(const std::string& property, const std::string& player) {
        if (!tail || player.empty()) return false;
        Node* current = tail->next;
        do {
            if (current->name == property) {
                if (!current->owner.empty()) return false;
                current->owner = player;
                return true;
            }
            current = current->next;
        } while (current != tail->next);
        return false;
    }
    void traverse() const {
        if (!tail) {
            std::cout << "(empty board)\n";
            return;
        }
        const Node* current = tail->next;
        std::size_t index = 0;
        do {
            std::cout << index++ << ": " << current->name
                      << " | $" << current->cost << " | "
                      << (current->owner.empty() ? "Unowned" : current->owner)
                      << '\n';
            current = current->next;
        } while (current != tail->next);
    }
};

void check(bool condition, const std::string& label) {
    if (!condition) throw std::runtime_error(label);
    std::cout << "PASS: " << label << '\n';
}

void runTests() {
    MonopolyBoard board;
    check(board.size() == 0 && !board.search("A") && !board.remove("A")
          && !board.move(nullptr, 3) && !board.purchase("A", "Alex"),
          "Empty-board operations");
    std::cout << "Empty traversal: ";
    board.traverse();
    check(board.insert("A", 10) && board.first()->next == board.first(),
          "Insert singleton and close circle");
    check(!board.insert("A", 20) && !board.insert("", 10)
          && !board.insert("Bad", -1), "Reject invalid insertion");
    check(board.remove("A") && board.size() == 0 && !board.first(),
          "Remove only node");
    board.insert("A", 10); board.insert("B", 20);
    board.insert("C", 30); board.insert("D", 40);
    check(board.search("C")->cost == 30 && !board.search("Missing"),
          "Search present and absent names");
    check(board.move(board.first(), 0) == board.first()
          && board.move(board.first(), 4) == board.first()
          && board.move(board.first(), 10)->name == "C", "Zero, full and multiple laps");
    check(board.purchase("B", "Alex") && !board.purchase("B", "Sam")
          && !board.purchase("B", "Alex") && !board.purchase("C", "")
          && board.search("B")->owner == "Alex", "Purchase ownership protection");
    check(board.remove("A") && board.first()->name == "B", "Remove head");
    check(board.remove("C") && board.first()->next->name == "D", "Remove middle");
    check(board.remove("D") && board.first()->next == board.first(), "Remove tail");
    check(!board.remove("Missing") && board.size() == 1, "Absent removal is harmless");
    check(board.remove("B") && !board.first(), "Return to empty board");
}

int main() {
    try {
        runTests();
        MonopolyBoard board;
        const char* names[] = {"Maple Avenue", "Oak Street", "Pine Road",
            "Cedar Lane", "Lake Drive", "Hill Street", "River Road",
            "Park Avenue", "Sunset Boulevard", "Boardwalk"};
        for (int i = 0; i < 10; ++i) board.insert(names[i], 60 + 20 * i);
        std::cout << "\nINITIAL BOARD\n";
        board.traverse();
        struct Player { std::string name; const MonopolyBoard::Node* position; };
        Player players[] = {{"Alex", board.first()}, {"Sam", board.first()}};
        const std::size_t steps[] = {3, 3, 8, 1, 12, 7, 6, 10, 5, 2};
        const int expectedIndices[] = {3, 3, 1, 4, 3, 1, 9, 1, 4, 3};
        const bool expectedPurchases[] = {true, false, true, true, false,
                                         false, true, false, false, false};
        std::cout << "\nTEN TURNS (players start at Maple Avenue)\n";
        // Board edits occur only in tests, before player pointers are created.
        for (int turn = 0; turn < 10; ++turn) {
            Player& player = players[turn % 2];
            const std::string from = player.position->name;
            player.position = board.move(player.position, steps[turn]);
            const bool bought = board.purchase(player.position->name, player.name);
            if (player.position->name != names[expectedIndices[turn]]
                || bought != expectedPurchases[turn]) {
                throw std::runtime_error("Turn result differs from expected result");
            }
            std::cout << "Turn " << turn + 1 << ": " << player.name
                      << " moves " << steps[turn] << " from " << from
                      << " to " << player.position->name << '\n';
            std::cout << "  " << (bought ? "Purchased" : "Purchase blocked")
                      << " | cost $" << player.position->cost
                      << " | owner " << player.position->owner << '\n';
        }
        std::cout << "\nFINAL BOARD\n";
        board.traverse();
        for (const auto& player : players) {
            std::cout << player.name << " ends at " << player.position->name << '\n';
        }
        std::cout << "All tests and 10 expected turn outcomes PASS\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
