#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// ================= CONSTANTS =================
const int BOARD_SIZE = 8;
const int CELL_SIZE = 80;
const int WINDOW_HEIGHT = BOARD_SIZE * CELL_SIZE + 60;
const float PIECE_RADIUS = CELL_SIZE / 2.f - 8.f;

// ================= COLORS =================
const sf::Color PLAYER_1_COLOR = sf::Color(220, 40, 40);
const sf::Color PLAYER_2_COLOR = sf::Color(30, 30, 30);
const sf::Color SELECTED_OUTLINE = sf::Color(255, 215, 0);
const sf::Color LIGHT_SQUARE = sf::Color(240, 217, 181);
const sf::Color DARK_SQUARE  = sf::Color(139, 69, 19);
const sf::Color BG_COLOR     = sf::Color(40, 42, 54);

// ================= STRUCT =================
struct Checker {
    sf::CircleShape shape;
    bool isKing = false;
    int player = 0;      // 1 or 2
    bool selected = false;
};

// ================= HELPERS =================
int getRow(float y) { return static_cast<int>(y / CELL_SIZE); }
int getCol(float x) { return static_cast<int>(x / CELL_SIZE); }

bool isDarkSquare(int r, int c) {
    return (r + c) % 2 == 1;
}

bool isValidPosition(int r, int c) {
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

// ================= MOVE VALIDATION =================
bool isValidMove(int sr, int sc, int tr, int tc,
                 const Checker& checker,
                 const std::vector<Checker>& checkers,
                 int& capturedIndex) {

    capturedIndex = -1;
    if (!isValidPosition(tr, tc)) return false;

    int dr = tr - sr;
    int dc = tc - sc;

    if (std::abs(dr) != std::abs(dc) || dr == 0) return false;

    if (!checker.isKing) {
        if (checker.player == 1 && dr <= 0) return false;
        if (checker.player == 2 && dr >= 0) return false;
    }

    for (const auto& ch : checkers) {
        if (getRow(ch.shape.getPosition().y) == tr &&
            getCol(ch.shape.getPosition().x) == tc)
            return false;
    }

    int steps = std::abs(dr);

    if (steps == 1) return true;

    if (steps == 2) {
        int mr = sr + dr / 2;
        int mc = sc + dc / 2;

        for (size_t i = 0; i < checkers.size(); ++i) {
            if (getRow(checkers[i].shape.getPosition().y) == mr &&
                getCol(checkers[i].shape.getPosition().x) == mc &&
                checkers[i].player != checker.player) {
                capturedIndex = static_cast<int>(i);
                return true;
            }
        }
    }
    return false;
}

bool hasAnyValidMove(int player, const std::vector<Checker>& checkers) {
    for (const auto& c : checkers) {
        if (c.player != player) continue;

        int sr = getRow(c.shape.getPosition().y);
        int sc = getCol(c.shape.getPosition().x);

        for (int dr = -2; dr <= 2; ++dr)
            for (int dc = -2; dc <= 2; ++dc) {
                if (dr == 0 || std::abs(dr) != std::abs(dc)) continue;
                int cap;
                if (isValidMove(sr, sc, sr + dr, sc + dc, c, checkers, cap))
                    return true;
            }
    }
    return false;
}

int checkWinner(const std::vector<Checker>& checkers) {
    bool has1 = false, has2 = false;
    for (const auto& c : checkers) {
        if (c.player == 1) has1 = true;
        if (c.player == 2) has2 = true;
    }
    if (!has1) return 2;
    if (!has2) return 1;
    return 0;
}

bool shouldPromote(int player, int row) {
    if (player == 1) return row == 0;
    if (player == 2) return row == BOARD_SIZE - 1;
    return false;
}

// ================= MAIN =================
int main() {
    sf::Font font;
    if (!font.loadFromFile("assets/OpenSans-Regular.ttf")) {
        std::cerr << "Failed to load font!\n";
        return 1;
    }

    // ========== MENU WINDOW ==========
    sf::RenderWindow menu(sf::VideoMode(640, 480), "Checkers - Menu", sf::Style::Close);

    sf::Text title("CHECKERS", font, 48);
    title.setFillColor(sf::Color::White);
    title.setPosition(200, 60);

    sf::Text opt1("1 vs Computer (not implemented)", font, 28);
    sf::Text opt2("2 Players (Local)", font, 28);
    opt1.setPosition(120, 200);
    opt2.setPosition(180, 260);

    bool vsComputer = false;
    bool chosen = false;

    while (menu.isOpen() && !chosen) {
        sf::Event e;
        while (menu.pollEvent(e)) {
            if (e.type == sf::Event::Closed) return 0;
            if (e.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i m = sf::Mouse::getPosition(menu);
                if (opt1.getGlobalBounds().contains(m.x, m.y)) {
                    vsComputer = true;
                    chosen = true;
                }
                if (opt2.getGlobalBounds().contains(m.x, m.y)) {
                    vsComputer = false;
                    chosen = true;
                }
            }
        }
        menu.clear(BG_COLOR);
        menu.draw(title);
        menu.draw(opt1);
        menu.draw(opt2);
        menu.display();
    }
    menu.close();

    // ========== GAME WINDOW ==========
    sf::RenderWindow window(
        sf::VideoMode(BOARD_SIZE * CELL_SIZE, WINDOW_HEIGHT),
        vsComputer ? "Checkers vs AI" : "Checkers - 2 Players"
    );

    std::vector<Checker> checkers;

    auto createChecker = [&](int r, int c, int player) {
        Checker ch;
        ch.player = player;
        ch.shape = sf::CircleShape(PIECE_RADIUS);
        ch.shape.setOrigin(PIECE_RADIUS, PIECE_RADIUS);
        ch.shape.setPosition(
            c * CELL_SIZE + CELL_SIZE / 2.f,
            r * CELL_SIZE + CELL_SIZE / 2.f
        );
        ch.shape.setFillColor(player == 1 ? PLAYER_1_COLOR : PLAYER_2_COLOR);
        return ch;
    };

    // Player 1 (top)
    for (int r = 0; r < 3; ++r)
        for (int c = (r % 2 == 0 ? 1 : 0); c < BOARD_SIZE; c += 2)
            checkers.push_back(createChecker(r, c, 1));

    // Player 2 (bottom)
    for (int r = 5; r < 8; ++r)
        for (int c = (r % 2 == 0 ? 1 : 0); c < BOARD_SIZE; c += 2)
            checkers.push_back(createChecker(r, c, 2));

    int currentPlayer = 1;
    int selectedIndex = -1;

    sf::Text status("", font, 24);
    status.setPosition(20, BOARD_SIZE * CELL_SIZE + 12);

    // ========== GAME LOOP ==========
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

                sf::Vector2i m = sf::Mouse::getPosition(window);
                int r = getRow(m.y);
                int c = getCol(m.x);

                if (!isValidPosition(r, c)) continue;

                if (selectedIndex == -1) {
                    for (size_t i = 0; i < checkers.size(); ++i) {
                        if (checkers[i].player == currentPlayer &&
                            getRow(checkers[i].shape.getPosition().y) == r &&
                            getCol(checkers[i].shape.getPosition().x) == c) {
                            selectedIndex = static_cast<int>(i);
                            checkers[i].shape.setOutlineThickness(5);
                            checkers[i].shape.setOutlineColor(SELECTED_OUTLINE);
                            break;
                        }
                    }
                } else {
                    int sr = getRow(checkers[selectedIndex].shape.getPosition().y);
                    int sc = getCol(checkers[selectedIndex].shape.getPosition().x);
                    int cap;

                    if (isValidMove(sr, sc, r, c, checkers[selectedIndex], checkers, cap)) {
                        checkers[selectedIndex].shape.setPosition(
                            c * CELL_SIZE + CELL_SIZE / 2.f,
                            r * CELL_SIZE + CELL_SIZE / 2.f
                        );

                        if (cap != -1)
                            checkers.erase(checkers.begin() + cap);

                        if (shouldPromote(currentPlayer, r)) {
                            checkers[selectedIndex].isKing = true;
                            checkers[selectedIndex].shape.setOutlineThickness(4);
                            checkers[selectedIndex].shape.setOutlineColor(sf::Color::Yellow);
                        }

                        currentPlayer = currentPlayer == 1 ? 2 : 1;
                    }

                    checkers[selectedIndex].shape.setOutlineThickness(0);
                    selectedIndex = -1;
                }
            }
        }

        int winner = checkWinner(checkers);
        status.setString(
            winner ? (winner == 1 ? "RED WINS!" : "BLACK WINS!")
                   : (currentPlayer == 1 ? "RED's turn" : "BLACK's turn")
        );

        window.clear(BG_COLOR);

        sf::RectangleShape square({CELL_SIZE, CELL_SIZE});
        for (int r = 0; r < BOARD_SIZE; ++r)
            for (int c = 0; c < BOARD_SIZE; ++c) {
                square.setPosition(c * CELL_SIZE, r * CELL_SIZE);
                square.setFillColor(isDarkSquare(r, c) ? DARK_SQUARE : LIGHT_SQUARE);
                window.draw(square);
            }

        for (auto& ch : checkers)
            window.draw(ch.shape);

        window.draw(status);
        window.display();
    }

    return 0;
}
