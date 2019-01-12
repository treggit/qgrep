//
// Created by Anton Shelepov on 2019-01-10.
//

#include "trigram.h"

void next_trigram(trigram& cur, QChar ch) {
    cur = ((cur << 8) | ch.unicode()) & ((1 << 24) - 1);
}

bool valid_for_text(QChar ch) {
    return ch != QChar(0);
}
