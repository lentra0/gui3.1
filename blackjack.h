#ifndef BLACKJACK_H
#define BLACKJACK_H

#include "blackjack_logic.h"

// Команды, которые посылают кнопки (через Window::NotifyParent)
// CMD_* используются в TextButton/Button как m_command и передаются родителю
enum GameCommands {
    CMD_HIT = 1,
    CMD_STAND,
    CMD_NEW_GAME
};

// BlackjackWindow наследует Window
// Он содержит дочерние виджеты Text (подписи), TextButton (кнопки) и
// управляет отрисовкой карт через Image
class BlackjackWindow : public Window {
public:
    BlackjackWindow();
    void OnCreate();
    void OnDraw(Context *cr);
    bool OnKeyPress(uint64_t keyval);
    void OnNotify(Window *child, uint32_t type, const Point &position);
    void OnSizeChanged();

private:
    BlackjackGame game;           // модель игры
    Text *m_dealerText;           // подписи и тексты — реализованы через Text
    Text *m_playerText;
    Text *m_scoreText;
    Text *m_resultText;
    Button *m_hitButton;          // кнопки — TextButton наследует Button -> Window
    Button *m_standButton;
    Button *m_newGameButton;

    // Локальный флаг GUI
    bool m_manualGameOver;

    void startNewGame();          // инициализация новой игры
    std::string cardsToString(const std::vector<Card>& cards, bool hideFirst = false);
    void updateDisplay();         // преобразует состояние в набор дочерних окон Image/Text
    void updateLayout();          // пересчитывает позиции/размеры постоянных элементов
};

#endif

