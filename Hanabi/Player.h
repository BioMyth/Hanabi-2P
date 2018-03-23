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
#pragma region pFunc
private:

    PassingData getBestDiscard();
    PassingData getBestPlay();
    float getDiscardability(HCard& card);
    
    void handleDiscardEvent(DiscardEvent *event);
    void handlePlayEvent(PlayEvent *event);
    void handleColorHintEvent(ColorHintEvent *event, HCard (&h)[5]);
    void handleNumberHintEvent(NumberHintEvent *event, HCard (&h)[5]);
    float getPlayability(HCard& card);

    Event* getBestHintInformation();
    Event* getBestHintCard();
    int savedByNumber(int checking);
    int savedByColor(int checking);
    
    void updateHand(HCard (&h)[5]);
#pragma endregion
private:
    int cardsRemaining[NUM_COLORS][NUM_NUMBERS];
    HCard MyHand[5];
    HCard PartnersHand[5];
    std::vector<Card> LastHand;
    std::vector<Card> Discards;


    std::vector<int> boardState;

    int hintsLeft;
    int fusesLeft;
    int deckSize;
};

Player::Player()
{
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int number = 0; number < NUM_NUMBERS; number++) {
            cardsRemaining[color][number] = 4;
        }
    }
    hintsLeft = MAX_HINTS;
    fusesLeft = 3;
    deckSize = NUM_COLORS * NUM_NUMBERS * 4;
}

Player::Player(const Player& p)
{

    for (int i = 0; i < HAND_SIZE; i++) {
        for (int color = 0; color < NUM_COLORS; color++) {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                MyHand[i].possibleCards[color][number] = p.MyHand[i].possibleCards[color][number];
                PartnersHand[i].possibleCards[color][number] = p.PartnersHand[i].possibleCards[color][number];
            }
        }
    }
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int number = 0; number < NUM_NUMBERS; number++) {
            cardsRemaining[color][number] = p.cardsRemaining[color][number];
        }
    }

    boardState = p.boardState;

    Discards = p.Discards;
    hintsLeft = p.hintsLeft;
    LastHand = p.LastHand;
    fusesLeft = p.fusesLeft;
    deckSize = p.deckSize;
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

    PassingData bestPlay = getBestPlay();

    PassingData bestDiscard = getBestDiscard();

    if (bestPlay.value > .9)
        return new PlayEvent(bestPlay.index);

    if (hintsLeft == 0)
        return new DiscardEvent(bestDiscard.index);

    if (bestPlay.value > (.7 + ((3 - fusesLeft) * .1)))
        return new PlayEvent(bestPlay.index);


    if (hintsLeft > 4 ) {
        Event* ret = getBestHintCard();
        if (ret == nullptr)
            ret = getBestHintInformation();

        return ret;
    }

    if (bestPlay.value > (.6 + (3 - fusesLeft) * .15)) {
        return new PlayEvent(bestPlay.index);
    }

    if (hintsLeft > 1) {
        Event* ret = getBestHintCard();
        if (ret == nullptr)
            ret = getBestHintInformation();

        return ret;
    }
    return new DiscardEvent(bestDiscard.index);
}

void Player::tell(Event* e, vector<int> board, int hints, int fuses, vector<Card> oHand, int deckSize)
{

    LastHand = vector<Card>(oHand); // This is the line that is actually failing!!!
    boardState = board;
    hintsLeft = hints;
    fusesLeft = fuses;
    deckSize = deckSize;
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

void Player::updateHand(HCard (&h)[5]) {
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int number = 0; number < NUM_NUMBERS; number++) {
            if (cardsRemaining[color][number] == 0) {
                for (int card = 0; card < HAND_SIZE; card++) {
                    h[card].possibleCards[color][number] = false;
                }
            }
        }
    }
}

void pushCardsBack(HCard (&h)[5], int fromIndex) {
    for (int i = fromIndex; i < HAND_SIZE - 1; i++) {
        h[i] = h[i + 1];
    }
    h[HAND_SIZE - 1].Reset();
}

void Player::handleDiscardEvent(DiscardEvent *event) {
    Discards.push_back(event->c);
    if (event->wasItThisPlayer) {
        pushCardsBack(MyHand, event->position);
        cardsRemaining[event->c.color][event->c.number]--;
    }
    else {
        pushCardsBack(PartnersHand, event->position);
        cardsRemaining[LastHand.back().color][LastHand.back().number]--;
    }
    updateHand(MyHand);
    updateHand(PartnersHand);
    cardsRemaining[event->c.color][event->c.number]--;
}

void Player::handlePlayEvent(PlayEvent *event) {
    Discards.push_back(event->c);
    if (event->wasItThisPlayer) {
        pushCardsBack(MyHand, event->position);
        cardsRemaining[event->c.color][event->c.number]--;
    }
    else {
        pushCardsBack(PartnersHand, event->position);
        cardsRemaining[LastHand.back().color][LastHand.back().number]--;
    }
    updateHand(MyHand);
    updateHand(PartnersHand);
}

void Player::handleColorHintEvent(ColorHintEvent *event, HCard (&h)[5]) {
    for (int i = 0; i < HAND_SIZE; i++) {
        if (find(event->indices.begin(), event->indices.end(), i) != event->indices.end()) {
            for (int color = 0; color < NUM_COLORS; color++) {
                for (int number = 0; number < NUM_NUMBERS; number++) {
                    if (color != event->color)
                        h[i].possibleCards[color][number] = false;
                }
            }
        }
        else {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                h[i].possibleCards[event->color][number] = false;
            }
        }
    }
}

void Player::handleNumberHintEvent(NumberHintEvent *event, HCard (&h)[5]) {
    for (int i = 0; i < HAND_SIZE; i++) {
        if (find(event->indices.begin(), event->indices.end(), i) != event->indices.end()) {
            for (int color = 0; color < NUM_COLORS; color++) {
                for (int number = 0; number < NUM_NUMBERS; number++) {
                    if (number != event->number)
                        h[i].possibleCards[color][number] = false;
                }
            }
        }
        else {
            for (int color = 0; color < NUM_COLORS; color++) {
                h[i].possibleCards[color][event->number] = false;
            }
        }
    }
}

PassingData Player::getBestPlay() {
    PassingData ret(0, getPlayability(MyHand[0]));
    for (int i = 1; i < HAND_SIZE; i++) {
        float currentValue = getPlayability(MyHand[i]);
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
                possibleCards += cardsRemaining[color][number];
                if ((boardState[color] + 1) == number)
                    goodCards += cardsRemaining[color][number];
            }
        }
    }
    float playability = goodCards / possibleCards;
    return playability;
}

PassingData Player::getBestDiscard() {
    PassingData ret(0, getDiscardability(MyHand[0]));
    for (size_t index = 1; index < HAND_SIZE; index++) {
        float indexDiscardability = getDiscardability(MyHand[index]);
        if (indexDiscardability < ret.value) {
            ret.index = index;
            ret.value = indexDiscardability;
        }
    }
    return ret;
}

float Player::getDiscardability(HCard& card) {
    float sum = 0;
    //float possibleCards = 0;
    float playability = getPlayability(card);
    for (size_t color = 0; color < NUM_COLORS; color++) {
        for (size_t number = 0; number < NUM_NUMBERS; number++) {
            //if (card.possibleCards[color][number]) {
                float numCardLeft = cardsRemaining[color][number];
                //possibleCards += numCardLeft;
                if (number < boardState[color])
                    continue;

                if (card.possibleCards[color][number]) {
                    float v = ((5 - numCardLeft));
                    sum += v;
                }
            }
        //}
    }
    return sum / (deckSize + 5);//possibleCards;
}

Event* Player::getBestHintInformation() { // His fault partially
    PassingData bestNumber(0, savedByNumber(LastHand[0].number));
    PassingData bestColor(0, savedByColor(LastHand[0].color));
    for (size_t i = 1; i < LastHand.size(); i++) {
        int number = savedByNumber(LastHand[i].number);
        if (number > bestNumber.value) {
            bestNumber.index = i;
            bestNumber.value = number;
        }

        int color = savedByColor(LastHand[i].color);
        if (color > bestColor.value) {
            bestColor.index = i;
            bestColor.value = color;
        }

    }
    if (bestColor.value > bestNumber.value)
        return new ColorHintEvent(vector<int>(), LastHand[bestColor.index].color);
    else
        return new NumberHintEvent(vector<int>(), LastHand[bestNumber.index].number);
}

Event* Player::getBestHintCard() {
    vector<int> viableCards;
    for (int i = 0; i < LastHand.size(); i++) {
        if ((boardState[LastHand[i].color] + 1) == LastHand[i].number)
            viableCards.push_back(i);
    }
    
    if (viableCards.size() < 1)
        return nullptr;

    PassingData bestNumber(0, savedByNumber(LastHand[0].number));
    PassingData bestColor(0, savedByColor(LastHand[0].color));
    for (auto iter = viableCards.begin(); iter != viableCards.end(); iter++) {
        int number = savedByNumber(LastHand[(*iter)].number);
        if (number > bestNumber.value) {
            bestNumber.index = (*iter);
            bestNumber.value = number;
        }

        int color = savedByColor(LastHand[(*iter)].color);
        if (color > bestColor.value) {
            bestColor.index = (*iter);
            bestColor.value = color;
        }

    }
    if (bestColor.value == 0 && bestNumber.value == 0)
        return nullptr;
    if (bestColor.value > bestNumber.value)
        return new ColorHintEvent(vector<int>(), LastHand[bestColor.index].color);
    else
        return new NumberHintEvent(vector<int>(), LastHand[bestNumber.index].number);
}

int Player::savedByNumber(int checking) {
    int ret = 0;
    for (int j = 0; j < HAND_SIZE && j < LastHand.size(); j++) {
        for (int color = 0; color < NUM_COLORS; color++) {
            ret += (PartnersHand[j].possibleCards[color][checking] && LastHand[j].number != checking);
            for (int number = 0; number < NUM_NUMBERS; number++) {
                ret += (number != checking && PartnersHand[j].possibleCards[color][number] && LastHand[j].number == checking);
            }
        }
    }
    return ret;
}

int Player::savedByColor(int checking) {
    int ret = 0;
    for (int j = 0; j < HAND_SIZE && j < LastHand.size(); j++) {
        for (int number = 0; number < NUM_NUMBERS; number++) {
            ret += (PartnersHand[j].possibleCards[checking][number] && LastHand[j].color != checking);
            for (int color = 0; color < NUM_COLORS; color++) {
                ret += (color != checking && PartnersHand[j].possibleCards[color][number] && LastHand[j].color == checking);
            }
        }
    }
    return ret;
}

#endif