#pragma once
#include "GameConstants.h"
#include <cstring>

struct HCard {
    HCard() {
        memset(possibleCards, true, NUM_COLORS * NUM_NUMBERS);
    }
    HCard(HCard& toCopy) {
        memcpy(possibleCards, toCopy.possibleCards, NUM_COLORS * NUM_NUMBERS);
    }
    void Reset() {
        memset(possibleCards, true, NUM_COLORS * NUM_NUMBERS);
    }
    bool possibleCards[NUM_COLORS][NUM_NUMBERS];
};

class Hand {
public:
    HCard cards[HAND_SIZE];
};
