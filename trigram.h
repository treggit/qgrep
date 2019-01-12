//
// Created by Anton Shelepov on 2019-01-10.
//

#ifndef QGREP_TRIGRAM_H
#define QGREP_TRIGRAM_H


#include <cstdint>
#include <QChar>

using trigram = uint32_t;

void next_trigram(trigram& cur, QChar ch);
bool valid_for_text(QChar ch);

#endif //QGREP_TRIGRAM_H
