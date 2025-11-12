#ifndef BLACKJACK_LOGIC_H
#define BLACKJACK_LOGIC_H

#include <vector>
#include <string>
#include <random>

// Масть карты
enum Suit { HEARTS, DIAMONDS, CLUBS, SPADES };

// Достоинство карты
enum Rank {
    ACE = 1, TWO, THREE, FOUR, FIVE, SIX, SEVEN,
    EIGHT, NINE, TEN, JACK, QUEEN, KING
};

// Карта
class Card {
public:
    Card(Suit s = HEARTS, Rank r = ACE) : suit(s), rank(r) {}

    // Формат: "A_hearts", "10_diamonds", "J_clubs", "K_spades"
    std::string toString() const {
        static const std::string ranks[] = {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
        static const std::string suits[] = {"hearts","diamonds","clubs","spades"};
        return ranks[rank-1] + std::string("_") + suits[suit];
    }

    int getValue() const {
        if (rank >= JACK) return 10;
        if (rank == ACE) return 11;
        return static_cast<int>(rank);
    }

private:
    Suit suit;
    Rank rank;
};

// Состояния игры
enum GameState {
    PLAYER_TURN,
    DEALER_TURN,
    GAME_OVER
};

// Игровая логика
class BlackjackGame {
public:
    BlackjackGame();

    void dealInitialCards();
    void playerHit();
    void playerStand();
    void dealerPlay();

    const std::vector<Card>& getPlayerHand() const { return playerHand; }
    const std::vector<Card>& getDealerHand() const { return dealerHand; }

    int getPlayerScore() const;
    int getDealerScore() const;
    GameState getGameState() const { return state; }
    // Пустая строка означает, что результат ещё не определён (state != GAME_OVER)
    std::string getGameResult() const;

private:
    std::vector<Card> deck;
    std::vector<Card> playerHand;
    std::vector<Card> dealerHand;
    GameState state;

    void initDeck();
    void shuffleDeck();
    Card drawCard();
    int calculateScore(const std::vector<Card>& hand) const;

    // Проверяет немедленные исходы и устанавливает state = GAME_OVER при необходимости
    void evaluateImmediateOutcome();
};

#endif

