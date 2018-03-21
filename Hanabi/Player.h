#include "GameConstants.h"
#include "Card.h"
#include "Events.h"
#include <vector>
#include <iostream>

#include "Hand.h"

using std::vector;
using std::cout;
using std::endl;

#ifndef PLAYER_H
#define PLAYER_H

class Player
{
public:
    Player();
    Player(const Player& p);
    void tell(Event* e, vector<int> board, int hints, int fuses, vector<Card> oHand, int deckSize);
    Event* ask();
private:
    Hand MyHand;
    Hand PartnersHand;
    vector<Card> LastHand;
    std::vector<Card> Discards;
    std::vector<Card> seenCards;
    bool safeDiscards[NUM_COLORS][NUM_NUMBERS];

    int hintsLeft;

    int getBestDiscard();
    int getBestPlay();
    float getDiscardability(HCard card);
    int Player::numCardRemaining(int color, int number);
    
    void handleDiscardEvent(DiscardEvent *event);
    void handlePlayEvent(PlayEvent *event);
    void handleColorHintEvent(ColorHintEvent *event);
    void handleNumberHintEvent(NumberHintEvent *event);
    void handleDrawEvent(DrawEvent *event);

    Event* getBestHint();
    //void updateHand();
    int savedByNumber(int checking);
    int savedByColor(int checking);
};

Player::Player()
{
    memset(safeDiscards, false, NUM_COLORS * NUM_NUMBERS);
}

Player::Player(const Player& p)
{
    memcpy(MyHand.cards, p.MyHand.cards, NUM_COLORS * NUM_NUMBERS);
    memcpy(PartnersHand.cards, p.PartnersHand.cards, NUM_COLORS * NUM_NUMBERS);
    memcpy(safeDiscards, p.safeDiscards, NUM_COLORS * NUM_NUMBERS);

    Discards = p.Discards;
    seenCards = p.seenCards;
    hintsLeft = p.hintsLeft;
    LastHand = p.LastHand;
}

void Player::tell(Event* e, vector<int> board, int hints, int fuses, vector<Card> oHand, int deckSize)
{
    hintsLeft = hints;
    /* Possible kinds of event:
        DiscardEvent - can be for us or other player
            c - the card discarded
            wasItThisPlayer - true if we discarded, false otherwise
            position - the index in hand of the discarded card (0 base)
        ColorHintEvent - always for other player
            indices - all indices of the chosen color
            color - the color in question
        NumberHintEvent - always for the other player
            indices - all indices of the chosen number
            color - the number in question
        PlayEvent - can be for us or other player
            position - the index in hand of the discarded card (0 base)
            c - the card played
            legal - whether the card was a legal play
            wasItThisPlayer - true if we discarded, false otherwise
    */
   
    switch (e->getAction()){
    case DISCARD:
        handleDiscardEvent(static_cast<DiscardEvent*>(e));
        break;
    case COLOR_HINT:
        handleColorHintEvent(static_cast<ColorHintEvent*>(e));
        break;
    case NUMBER_HINT:
        handleNumberHintEvent(static_cast<NumberHintEvent*>(e));
        break;
    case PLAY:
        handlePlayEvent(static_cast<PlayEvent*>(e));
        break;
    case DRAW:
        handleDrawEvent(static_cast<DrawEvent*>(e));
        break;
    case NO_OP:
        break;
    }
}


void Player::handleDiscardEvent(DiscardEvent *event) {
    Discards.push_back(event->c);
    seenCards.push_back(event->c);
    if (event->wasItThisPlayer) {
        MyHand.cards[event->position].Reset();
    }
    else {
        PartnersHand.cards[event->position].Reset();
    }
}

void Player::handlePlayEvent(PlayEvent *event) {

}

void Player::handleColorHintEvent(ColorHintEvent *event) {

}
void Player::handleNumberHintEvent(NumberHintEvent *event) {

}
void Player::handleDrawEvent(DrawEvent *event) {

}



Event* Player::ask()
{
    /* You must produce an event of the appropriate type. Not all member
        variables of a given event type need to be filled in; some will be
        ignored even if they are. Summary follows.
    Options:
        ColorHintEvent - you must declare a color; no other member variables
            necessary.
        NumberHintEvent - you must declare a number; no other member variables
            necessary.
        PlayEvent - you must declare the index to be played; no other member
            variables necessary.
        DiscardEvent - you must declare the index to be discarded; no other
            member variables necessary.
    */
    // Get most Playable card (determined by ???)
    int indexToPlay = getBestPlay();
    float playbility = getDiscardability(MyHand.cards[indexToPlay]);
    
    int indexToDiscard = getBestDiscard();
    float discardability = getDiscardability(MyHand.cards[indexToDiscard]);

    if (playbility > .3) {
        return new PlayEvent(indexToPlay);
    }
    
    if (hintsLeft < MAX_HINTS) {

        if (discardability < .3f) {
            // Discard Card
            return new DiscardEvent(indexToDiscard);
        }
    }
    if (hintsLeft == 0) {
        return new DiscardEvent(indexToDiscard);
    }
    if (hintsLeft > 5) {
        return getBestHint();
    }
    //return new PlayEvent(indexToPlay);
    vector<int> indicies;
    return new ColorHintEvent(indicies, LastHand[0].color);
}

int Player::getBestPlay() {
    int max = 0;
    float maxValue = getDiscardability(MyHand.cards[0]);
    for (size_t index = 1; index < HAND_SIZE; index++) {
        float indexDiscardability = getDiscardability(MyHand.cards[index]);
        if (indexDiscardability > maxValue) {
            max = index;
            maxValue = indexDiscardability;
        }
    }
    return max;
}



int Player::getBestDiscard() {
    int min = 0;
    float minValue = getDiscardability(MyHand.cards[0]);
    for (size_t index = 1; index < HAND_SIZE; index++) {
        float indexDiscardability = getDiscardability(MyHand.cards[index]);
        if (indexDiscardability < minValue) {
            min = index;
            minValue = indexDiscardability;
        }
    }
    return min;
}

// You want to discard the smallest of these
float Player::getDiscardability(HCard card) {
    float sum = 0;
    int possibleCards = 0;
    for (size_t color = 0; color < NUM_COLORS; color++) {
        for (size_t number = 0; number < NUM_NUMBERS; number++) {
            int numCardLeft = numCardRemaining(color, number);
            possibleCards += numCardLeft;
            if (safeDiscards[color][number])
                continue;
            // Chance to be card
            float c = (numCardLeft / 4) / ((NUM_COLORS * NUM_NUMBERS * 4) - seenCards.size());
            // Value of this card(importance to keep)
            float v = safeDiscards[color][number] ? 0 : (5 - numCardLeft);
            sum += c * v;
        }
    }
    return sum / possibleCards;
}

int Player::numCardRemaining(int color, int number) {
    int numInDeck = 4;
    for (auto iter = seenCards.begin(); iter != seenCards.end(); iter++) {
        if (iter->color == color && iter->number == number)
            numInDeck--;
    }
    return numInDeck;
}

Event* Player::getBestHint() {
    int bestNumber = 0, bestNumberVal = -1;
    int bestColor = 0, bestColorVal = -1;
    for (int i = 0; i < LastHand.size(); i++) {
        int number = savedByNumber(LastHand[i].number);
        if (number > bestNumberVal) {
            bestNumber = LastHand[i].number;
            bestNumberVal = number;
        }

        int color = savedByColor(LastHand[i].color);
        if (color > bestColorVal) {
            bestColor = LastHand[i].color;
            bestColorVal = color;
        }
    }

    if (bestColorVal > bestNumberVal)
        return new ColorHintEvent(vector<int>(), bestColor);
    else
        return new NumberHintEvent(vector<int>(), bestNumber);
}

int Player::savedByNumber(int checking) {
    int ret = 0;
    for (int j = 0; j < HAND_SIZE; j++) {
        for (int color = 0; color < NUM_COLORS; color++) {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                ret += (number != checking && PartnersHand.cards[j].possibleCards[color][number]);
            }
        }
    }
    return ret;
}

int Player::savedByColor(int checking) {
    int ret = 0;
    for (int j = 0; j < HAND_SIZE; j++) {
        for (int color = 0; color < NUM_COLORS; color++) {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                ret += (color != checking && PartnersHand.cards[j].possibleCards[color][number]);
            }
        }
    }
    return ret;
}
#endif