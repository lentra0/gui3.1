#ifndef BLACKJACK_H
#define BLACKJACK_H

#include "blackjack_logic.h"

// Команды кнопок
enum GameCommands {
    CMD_HIT = 1,
    CMD_STAND,
    CMD_NEW_GAME
};

class BlackjackWindow : public Window {
public:
    BlackjackWindow();
    void OnCreate();
    void OnDraw(Context *cr);
    bool OnKeyPress(uint64_t keyval);
    void OnNotify(Window *child, uint32_t type, const Point &position);

private:
    BlackjackGame game;
    Text *m_dealerText;
    Text *m_playerText;
    Text *m_scoreText;
    Text *m_resultText;

    void startNewGame();
    std::string cardsToString(const std::vector<Card>& cards, bool hideFirst = false);
    void updateDisplay();
};

#endif
