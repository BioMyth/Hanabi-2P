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
    std::vector<int> boardState;

    int hintsLeft;

    PassingData getBestDiscard();
    PassingData getBestPlay();
    float getDiscardability(HCard& card);
    int Player::numCardRemaining(int color, int number);
    
    void handleDiscardEvent(DiscardEvent *event);
    void handlePlayEvent(PlayEvent *event);
    void handleColorHintEvent(ColorHintEvent *event, Hand& h);
    void handleNumberHintEvent(NumberHintEvent *event, Hand& h);
    float getPlayability(HCard& card);

    Event* getBestHint(); 
    Event* getBestHintInformation();
    Event* getBestHintCard();
    //void updateHand();
    int savedByNumber(int checking);
    int savedByColor(int checking);
};

Player::Player()
{
    memset(safeDiscards, false, NUM_COLORS * NUM_NUMBERS);
    hintsLeft = MAX_HINTS;
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
    LastHand = oHand;
    boardState = vector<int>(board);
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
        handleColorHintEvent(static_cast<ColorHintEvent*>(e), MyHand);
        break;
    case NUMBER_HINT:
        handleNumberHintEvent(static_cast<NumberHintEvent*>(e), MyHand);
        break;
    case PLAY:
        handlePlayEvent(static_cast<PlayEvent*>(e));
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
    Discards.push_back(event->c);
    seenCards.push_back(event->c);
    if (event->wasItThisPlayer) {
        MyHand.cards[event->position].Reset();
    }
    else {
        PartnersHand.cards[event->position].Reset();
    }
}

void Player::handleColorHintEvent(ColorHintEvent *event, Hand& h) {
    for (int i = 0; i < HAND_SIZE; i++) {
        if (LastHand[i].color == event->color) {
            for (int color = 0; color < NUM_COLORS; color++) {
                for (int number = 0; number < NUM_NUMBERS; number++) {
                    if (color != event->color)
                        h.cards[i].possibleCards[color][number] = false;
                }
            }
        }
        else {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                h.cards[i].possibleCards[event->color][number] = false;
            }
        }
    }
}
void Player::handleNumberHintEvent(NumberHintEvent *event, Hand& h) {
    for (int i = 0; i < HAND_SIZE; i++) {
        if (LastHand[i].number == event->number) {
            for (int color = 0; color < NUM_COLORS; color++) {
                for (int number = 0; number < NUM_NUMBERS; number++) {
                    if (number != event->number)
                        h.cards[i].possibleCards[color][number] = false;
                }
            }
        }
        else {
            for (int color = 0; color < NUM_COLORS; color++) {
                h.cards[i].possibleCards[color][event->number] = false;
            }
        }
    }
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
    PassingData bestPlay = getBestPlay();
    
    PassingData bestDiscard = getBestDiscard();


    if (bestPlay.value > .90) {
        return new PlayEvent(bestPlay.index + 1);
    }
    if (hintsLeft > 4) {
        Event* tmp = getBestHint();
        ColorHintEvent* CHE = static_cast<ColorHintEvent*>(tmp);
        NumberHintEvent* NHE = static_cast<NumberHintEvent*>(tmp);

        if (CHE != nullptr)
            handleColorHintEvent(CHE, PartnersHand);

        if (NHE != nullptr)
            handleNumberHintEvent(NHE, PartnersHand);
        return tmp;
    }

    if (hintsLeft > 1) {
        Event* tmp = getBestHint();
        ColorHintEvent* CHE = static_cast<ColorHintEvent*>(tmp);
        NumberHintEvent* NHE = static_cast<NumberHintEvent*>(tmp);

        if (CHE != nullptr)
            handleColorHintEvent(CHE, PartnersHand);

        if (NHE != nullptr)
            handleNumberHintEvent(NHE, PartnersHand);
        return tmp;
    }
    return new DiscardEvent(bestDiscard.index);
}

PassingData Player::getBestPlay() {
    PassingData ret(0, getPlayability(MyHand.cards[0]));
    for (int i = 1; i < HAND_SIZE; i++) {
        float currentValue = getPlayability(MyHand.cards[i]);
        if (currentValue > ret.value) {
            ret.index = i;
            ret.value = currentValue;
        }
    }
    return ret;
}

float Player::getPlayability(HCard& card) {
    float possibleCards = 0;
    float goodCards = 0;
    for (size_t color = 0; color < NUM_COLORS; color++) {
        for (size_t number = 0; number < NUM_NUMBERS; number++) {
            if (card.possibleCards[color][number]) {
                int numLeft = numCardRemaining(color, number);
                possibleCards += numLeft;
                if ((boardState[color] + 1) == number)
                    goodCards += numLeft;
            }
        }
    }
    float playability = goodCards / possibleCards;
    return playability;
}



PassingData Player::getBestDiscard() {
    PassingData ret(0, getDiscardability(MyHand.cards[0]));
    for (size_t index = 1; index < HAND_SIZE; index++) {
        float indexDiscardability = getDiscardability(MyHand.cards[index]);
        if (indexDiscardability < ret.value) {
            ret.index = index;
            ret.value = indexDiscardability;
        }
    }
    return ret;
}

// You want to discard the smallest of these Currently Failing
float Player::getDiscardability(HCard& card) {
    float sum = 0;
    float possibleCards = 0;
    for (size_t color = 0; color < NUM_COLORS; color++) {
        for (size_t number = 0; number < NUM_NUMBERS; number++) {
            float numCardLeft = numCardRemaining(color, number);
            possibleCards += numCardLeft;
            if (safeDiscards[color][number])
                continue;
            // Chance to be card
            float c = (4- numCardLeft);//(numCardLeft / 4) / static_cast<float>((NUM_COLORS * NUM_NUMBERS * 4) - seenCards.size());
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
    Event* ret = getBestHintCard();
    if (ret == nullptr)
        ret = getBestHintInformation();

    return ret;
}

Event* Player::getBestHintInformation() {
    PassingData bestNumber(0, -1);
    PassingData bestColor(0, -1);
    for (int i = 0; i < LastHand.size(); i++) {
        int number = savedByNumber(LastHand[i].number);
        if (number > bestNumber.value) {
            bestNumber.index = LastHand[i].number;
            bestNumber.value = number;
        }


        int color = savedByColor(LastHand[i].color);
        if (color > bestColor.value) {
            bestColor.index = LastHand[i].color;
            bestColor.value = color;
        }

    }

    if (bestColor.value > bestNumber.value)
        return new ColorHintEvent(vector<int>(), bestColor.index);
    else
        return new NumberHintEvent(vector<int>(), bestNumber.index);
}

Event* Player::getBestHintCard() {
    vector<int> viableCards;
    for (int i = 0; i < LastHand.size(); i++) {
        if ((boardState[LastHand[i].color] + 1) == LastHand[i].number)
            viableCards.push_back(i);
    }
    
    if (viableCards.size() < 1)
        return nullptr;

    int bestNumber = 0, bestNumberVal = -1;
    int bestColor = 0, bestColorVal = -1;
    for (auto iter = viableCards.begin(); iter != viableCards.end(); iter++) {
        int number = savedByNumber(LastHand[*iter].number);
        if (number > bestNumberVal) {
            bestNumber = LastHand[*iter].number;
            bestNumberVal = number;
        }


        int color = savedByColor(LastHand[*iter].color);
        if (color > bestColorVal) {
            bestColor = LastHand[*iter].color;
            bestColorVal = color;
        }

    }
    if (bestColorVal == 0 && bestNumberVal == 0)
        return nullptr;
    if (bestColorVal > bestNumberVal)
        return new ColorHintEvent(vector<int>(), bestColor);
    else
        return new NumberHintEvent(vector<int>(), bestNumber);
}

int Player::savedByNumber(int checking) {
    int ret = 0;
    for (int j = 0; j < HAND_SIZE; j++) {
        for (int color = 0; color < NUM_COLORS; color++) {
            ret += (PartnersHand.cards[j].possibleCards[color][checking] && LastHand[j].number != checking);
            for (int number = 0; number < NUM_NUMBERS; number++) {
                ret += (number != checking && PartnersHand.cards[j].possibleCards[color][number] && LastHand[j].number == checking);
            }
        }
    }
    return ret;
}

int Player::savedByColor(int checking) {
    int ret = 0;
    for (int j = 0; j < HAND_SIZE; j++) {
        for (int number = 0; number < NUM_NUMBERS; number++) {
            ret += (PartnersHand.cards[j].possibleCards[checking][number] && LastHand[j].color != checking);
            for (int color = 0; color < NUM_COLORS; color++) {
                ret += (color != checking && PartnersHand.cards[j].possibleCards[color][number] && LastHand[j].color == checking);
            }
        }
    }
    return ret;
}
#endif