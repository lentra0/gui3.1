#include "blackjack_logic.h"
#include <algorithm>
#include <random>
#include <chrono>

BlackjackGame::BlackjackGame() {
    initDeck();
    state = PLAYER_TURN;
}

void BlackjackGame::initDeck() {
    deck.clear();
    for(int s = HEARTS; s <= SPADES; s++) {
        for(int r = ACE; r <= KING; r++) {
            deck.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }
    shuffleDeck();
}

void BlackjackGame::shuffleDeck() {
    auto rng = std::default_random_engine(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    std::shuffle(deck.begin(), deck.end(), rng);
}

Card BlackjackGame::drawCard() {
    if(deck.empty()) {
        initDeck();
    }
    Card card = deck.back();
    deck.pop_back();
    return card;
}

void BlackjackGame::dealInitialCards() {
    playerHand.clear();
    dealerHand.clear();
    
    playerHand.push_back(drawCard());
    dealerHand.push_back(drawCard());
    playerHand.push_back(drawCard());
    dealerHand.push_back(drawCard());
    
    state = PLAYER_TURN;
}

void BlackjackGame::playerHit() {
    if(state != PLAYER_TURN) return;
    
    playerHand.push_back(drawCard());
    if(getPlayerScore() > 21) {
        state = GAME_OVER;
    }
}

void BlackjackGame::playerStand() {
    if(state != PLAYER_TURN) return;
    
    state = DEALER_TURN;
    dealerPlay();
}

void BlackjackGame::dealerPlay() {
    while(getDealerScore() < 17) {
        dealerHand.push_back(drawCard());
    }
    state = GAME_OVER;
}

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

std::string BlackjackGame::getGameResult() const {
    if(state != GAME_OVER) return "Игра продолжается";
    
    int pScore = getPlayerScore();
    int dScore = getDealerScore();
    
    if(pScore > 21) return "Победа дилера! У вас перебор";
    if(dScore > 21) return "Вы победили! У дилера перебор";
    if(pScore > dScore) return "Вы победили!";
    if(dScore > pScore) return "Победа дилера!";
    return "Ничья!";
}
