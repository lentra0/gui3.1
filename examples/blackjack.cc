#include <cstring>

#ifdef min
#undef min
#endif

#ifdef max 
#undef max
#endif

#include <random>
#include <vector>
#include <string>

#include "window.h"
#include "image.h" 
#include "text.h"
#include "button.h"
#include "GUI.h"
#include "blackjack.h"

BlackjackWindow::BlackjackWindow() {
    m_ClassName = __FUNCTION__;
    SetBackColor(RGB(0.0, 0.5, 0.0)); // Зеленый стол
}

void BlackjackWindow::OnCreate() {
    // Тексты для отображения состояния игры
    m_dealerText = new Text("Дилер: ");  // Важно: задаем начальный текст
    m_dealerText->SetFont(NULL, 20, -1, -1);
    m_dealerText->SetTextColor(RGB(1.0, 1.0, 1.0));
    m_dealerText->SetBackColor(GetBackColor());
    AddChild(m_dealerText, Point(20, 20), Rect(300, 30));

    m_playerText = new Text("Игрок: ");
    m_playerText->SetFont(NULL, 20, -1, -1);
    m_playerText->SetTextColor(RGB(1.0, 1.0, 1.0));
    m_playerText->SetBackColor(GetBackColor());
    AddChild(m_playerText, Point(20, 120), Rect(300, 30));

    m_scoreText = new Text("Счет: 0 | 0");
    m_scoreText->SetFont(NULL, 16, -1, -1);
    m_scoreText->SetTextColor(RGB(1.0, 1.0, 1.0));
    m_scoreText->SetBackColor(GetBackColor());
    AddChild(m_scoreText, Point(20, 70), Rect(200, 30));

    m_resultText = new Text("");
    m_resultText->SetFont(NULL, 18, -1, -1);
    m_resultText->SetTextColor(RGB(1.0, 1.0, 0.0));
    m_resultText->SetBackColor(GetBackColor());
    AddChild(m_resultText, Point(20, 170), Rect(300, 30));

    // Кнопки управления
    AddChild(new TextButton("Взять", CMD_HIT), 
            Point(20, 220), Rect(80, 40));
    
    AddChild(new TextButton("Хватит", CMD_STAND),
            Point(120, 220), Rect(80, 40));
    
    AddChild(new TextButton("Новая игра", CMD_NEW_GAME),
            Point(220, 220), Rect(100, 40));

    // Начинаем первую игру
    startNewGame();
}

void BlackjackWindow::OnDraw(Context *cr) {
    Point size = GetInteriorSize();
    cr->SetColor(GetBackColor());
    cr->FillRectangle(Point(0,0), size);
}

bool BlackjackWindow::OnKeyPress(uint64_t keyval) {
    switch(keyval) {
        case 'q':
            DeleteMe();
            return true;
        case 'n':
            startNewGame();
            return true;
        case 'h':
            game.playerHit();
            updateDisplay();
            return true;
        case 's':
            game.playerStand();
            updateDisplay();
            return true;
        default:
            return true;
    }
}

void BlackjackWindow::OnNotify(Window *child, uint32_t type, const Point &position) {
    switch(type) {
        case CMD_HIT:
            game.playerHit();
            updateDisplay();
            break;
        case CMD_STAND:
            game.playerStand();
            updateDisplay();
            break;
        case CMD_NEW_GAME:
            startNewGame();
            break;
    }
}

void BlackjackWindow::startNewGame() {
    game.dealInitialCards();
    updateDisplay();
}

std::string BlackjackWindow::cardsToString(const std::vector<Card>& cards, bool hideFirst) {
    std::string result;
    bool first = true;
    for (const auto& card : cards) {
        if (first && hideFirst) {
            result += "** ";
            first = false;
            continue;
        }
        result += card.toString() + " ";
        first = false;
    }
    return result;
}

void BlackjackWindow::updateDisplay() {
    bool hideDealerCard = (game.getGameState() == PLAYER_TURN);
    
    // Обновляем карты дилера
    std::string dealerStr = "Дилер: " + cardsToString(game.getDealerHand(), hideDealerCard);
    m_dealerText->SetText(dealerStr.c_str());

    // Обновляем карты игрока
    std::string playerStr = "Игрок: " + cardsToString(game.getPlayerHand());
    m_playerText->SetText(playerStr.c_str());

    // Обновляем счет
    std::string scoreStr = "Счет: ";
    if (hideDealerCard) {
        scoreStr += "?? | ";
    } else {
        scoreStr += std::to_string(game.getDealerScore()) + " | ";
    }
    scoreStr += std::to_string(game.getPlayerScore());
    m_scoreText->SetText(scoreStr.c_str());

    // Обновляем результат
    if (game.getGameState() == GAME_OVER) {
        m_resultText->SetText(game.getGameResult().c_str());
    } else {
        m_resultText->SetText("");
    }

    ReDraw();
}

int main(int argc, char **argv) {
    BlackjackWindow *window = new BlackjackWindow();
    int res = Run(argc, argv, window, 400, 300);
    delete window;
    return res;
}
