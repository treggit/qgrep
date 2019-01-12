//
// Created by Anton Shelepov on 2019-01-12.
//

#ifndef QGREP_QSTRING_STD_HASH_H
#define QGREP_QSTRING_STD_HASH_H

#include <QString>

namespace std {
    template <>
    struct hash <QString> {
        auto operator() (QString const& s) const noexcept {
            return hash<std::string>()(s.toStdString());
        }
    };
}

#endif //QGREP_QSTRING_STD_HASH_H
