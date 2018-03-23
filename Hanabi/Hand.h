#pragma once
#include "GameConstants.h"
#include <cstring>

struct HCard {
    HCard() {
        for (int color = 0; color < NUM_COLORS; color++) {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                possibleCards[color][number] = true;
            }
        }
    }
    HCard(HCard& toCopy) {
        for (int color = 0; color < NUM_COLORS; color++) {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                possibleCards[color][number] = toCopy.possibleCards[color][number];
            }
        }
    }
    void Reset() {
        for (int color = 0; color < NUM_COLORS; color++) {
            for (int number = 0; number < NUM_NUMBERS; number++) {
                possibleCards[color][number] = true;
            }
        }
    }
    bool possibleCards[NUM_COLORS][NUM_NUMBERS];
};

class Hand {
public:
    HCard cards[HAND_SIZE];
};

struct PassingData {
    PassingData(int index, float value) :index(index), value(value) {}
    PassingData() :index(0), value(0) {};

    int index;
    float value;
};
