#include "blackjack_logic.h"
#include <algorithm>
#include <random>
#include <chrono>

// Конструктор инициализирует колоду и ставит состояние в PLAYER_TURN
BlackjackGame::BlackjackGame() {
    initDeck();
    state = PLAYER_TURN;
}

// initDeck создает стандартную колоду 52 карты и тасует её
void BlackjackGame::initDeck() {
    deck.clear();
    for(int s = HEARTS; s <= SPADES; ++s) {
        for(int r = ACE; r <= KING; ++r) {
            deck.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }
    shuffleDeck();
}

// shuffleDeck использует std::shuffle с генератором, основанным на системном времени 
void BlackjackGame::shuffleDeck() {
    auto rng = std::default_random_engine(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    std::shuffle(deck.begin(), deck.end(), rng);
}

// drawCard достаёт верхнюю карту из колоды (последнюю в векторе)
// Если колода пустая — реинициализируем её
Card BlackjackGame::drawCard() {
    if(deck.empty()) {
        initDeck();
    }
    Card card = deck.back();
    deck.pop_back();
    return card;
}

// Раздача начальных карт: игрок и дилер по 2 карты по очереди
// После раздачи состояние — PLAYER_TURN. Затем проверяем немедленные исходы
void BlackjackGame::dealInitialCards() {
    playerHand.clear();
    dealerHand.clear();
    
    playerHand.push_back(drawCard());
    dealerHand.push_back(drawCard());
    playerHand.push_back(drawCard());
    dealerHand.push_back(drawCard());
    
    state = PLAYER_TURN;

    evaluateImmediateOutcome();
}

// GUI вызывает этот метод при нажатии "Взять"
void BlackjackGame::playerHit() {
    if(state != PLAYER_TURN) return;
    
    playerHand.push_back(drawCard());
    evaluateImmediateOutcome();
}

// Игрок холдит — переводим управление дилеру и запускаем логику дилера
void BlackjackGame::playerStand() {
    if(state != PLAYER_TURN) return;
    
    state = DEALER_TURN;
    dealerPlay();
}

// Дилер тянет карты по простому правилу: пока сумма < 17
void BlackjackGame::dealerPlay() {
    while(getDealerScore() < 17) {
        dealerHand.push_back(drawCard());
    }
    state = GAME_OVER;
}

// calculateScore считает сумму руки с учётом правил для тузов (11 или 1)
int BlackjackGame::calculateScore(const std::vector<Card>& hand) const {
    int score = 0;
    int aces = 0;
    
    for(const auto& card : hand) {
        if(card.getValue() == 11) aces++;
        score += card.getValue();
    }
    
    while(score > 21 && aces > 0) {
        score -= 10;
        aces--;
    }
    
    return score;
}

int BlackjackGame::getPlayerScore() const {
    return calculateScore(playerHand);
}

int BlackjackGame::getDealerScore() const {
    return calculateScore(dealerHand);
}

// getGameResult возвращает строку результата, если state == GAME_OVER,
// иначе возвращает пустую строку
std::string BlackjackGame::getGameResult() const {
    if(state != GAME_OVER) return std::string();
    
    int pScore = getPlayerScore();
    int dScore = getDealerScore();
    
    if(pScore > 21) return "Вы проиграли!";
    if(dScore > 21) return " Вы победили!";
    if(pScore > dScore) return " Вы победили!";
    if(dScore > pScore) return "Вы проиграли!";
    return "     Ничья";
}

// Локальная проверка немедленных исходов после каждой модификации руки
// Устанавливает state = GAME_OVER в случае блекджека/перебора
void BlackjackGame::evaluateImmediateOutcome() {
    if(state == GAME_OVER) return;

    int pScore = getPlayerScore();
    int dScore = getDealerScore();

    if(pScore > 21) {
        state = GAME_OVER;
        return;
    }

    if(pScore == 21) {
        state = GAME_OVER;
        return;
    }

    if(dScore > 21) {
        state = GAME_OVER;
        return;
    }
}

