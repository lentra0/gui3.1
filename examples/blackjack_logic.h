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
    Card(Suit s, Rank r) : suit(s), rank(r) {}
    
    // Получить текстовое представление карты
    std::string toString() const {
        std::string ranks[] = {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
        std::string suits[] = {"♥","♦","♣","♠"};
        return ranks[rank-1] + suits[suit];
    }
    
    // Получить значение карты
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
    PLAYER_TURN,     // Ход игрока
    DEALER_TURN,     // Ход дилера 
    GAME_OVER       // Игра окончена
};

// Игровая логика
class BlackjackGame {
public:
    BlackjackGame();
    
    void dealInitialCards();         // Раздать начальные карты
    void playerHit();               // Игрок берет карту
    void playerStand();             // Игрок пасует
    void dealerPlay();              // Ход дилера
    
    const std::vector<Card>& getPlayerHand() const { return playerHand; }
    const std::vector<Card>& getDealerHand() const { return dealerHand; }
    
    int getPlayerScore() const;      // Подсчет очков игрока
    int getDealerScore() const;      // Подсчет очков дилера
    GameState getGameState() const { return state; }
    std::string getGameResult() const;  // Получить результат игры

private:
    std::vector<Card> deck;          // Колода
    std::vector<Card> playerHand;    // Карты игрока
    std::vector<Card> dealerHand;    // Карты дилера
    GameState state;                 // Текущее состояние игры
    
    void initDeck();                // Инициализация колоды
    void shuffleDeck();             // Перемешать колоду
    Card drawCard();                // Взять карту из колоды
    int calculateScore(const std::vector<Card>& hand) const;  // Подсчет очков
};

#endif
