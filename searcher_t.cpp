//
// Created by Anton Shelepov on 08/01/2019.
//

#include "searcher_t.h"

#include <QtConcurrent/QtConcurrent>
#include <iostream>

searcher_t::searcher_t(std::unordered_map<QString, std::vector<trigram>>& ind, std::mutex& m)
        : index(ind), index_mutex(m)
{

}

searcher_t::~searcher_t() {
    search_pool.clear();
    search_pool.waitForDone();
}

void searcher_t::search_string(QString const& str) {
    std::unique_lock<std::mutex> lock(index_mutex);
    for (auto& i : index) {
        find_string_in_file(str, i.first);
    }
    lock.unlock();
    search_pool.waitForDone();
    qDebug() << "finished search";
}

QFuture<void> searcher_t::find_string_in_file(QString const& str, QString const& path) {
    return QtConcurrent::run(&search_pool, [this, str, path]() {
        if (contains(path, str)) {
            emit release_entry(path);
        }
        if (!isInterruptionRequested()) {
            emit inc_progress_bar();
        }
    });
}

bool searcher_t::contains(QString const& path, QString const& str) {
    if (!check_trigrams(path, str)) {
        return false;
    }
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream text_stream(&file);
        text_stream.setCodec("UTF-8");
        QString buffer = str + QChar(0);
        QVector<size_t> p(2 * str.size() + BUFFER_SIZE + 1);
        prefix_function_check(p, buffer, buffer.size());
        QString add;
        if (isInterruptionRequested()) {
            return false;
        }
        bool f = true;
        while ((buffer.append(text_stream.read(BUFFER_SIZE))).size() > str.size() + 1) {
            if(isInterruptionRequested()) {
                qDebug() << "interrupted";
                return false;
            }
            if(prefix_function_check(p, buffer, str.size(), f ? str.size() + 1 : str.size() * 2 + 1)) {
                return true;
            }
            f = false;
            for (size_t i = 0; i < str.size(); i++) {
                p[str.size() + 1 + i] = p[buffer.size() - str.size() + i];
            }
            //buffer = buffer.mid(0, str.size() + 1) + buffer.mid(buffer.size() - str.size(), str.size());
            buffer = str + QChar(0);
        }
    }
    return false;
}

bool searcher_t::check_trigrams(QString const& path, QString const& str) {
    if (str.size() < 3) {
        return true;
    }
    trigram cur = 0;
    next_trigram(cur, str[0]);
    next_trigram(cur, str[1]);
    std::lock_guard<std::mutex> lock(index_mutex);
    auto& ind = index[path];
    for (size_t i = 2; i < str.size(); i++) {
        next_trigram(cur, str.at(i));
        if (!std::binary_search(ind.begin(), ind.end(), cur)) {
            return false;
        }
    }
    return true;
}

bool searcher_t::prefix_function_check(QVector<size_t>& p, QString const& s, size_t need, size_t start_from) {
    size_t j;
    for (size_t i = start_from; i < s.size(); i++) {
        if (!valid_for_text(s.at(i))) {
            return false;
        }
        j = p[i - 1];
        while (j > 0 && s.at(i) != s.at(j)) {
            j = p[j - 1];
        }
        j += (s.at(i) == s.at(j));
        p[i] = j;
        if (p[i] == need) {
            return true;
        }
    }
    return false;
}

void searcher_t::run() {
    search_string(string_to_find);
}

void searcher_t::set_string(QString const& str) {
    string_to_find = str;
}
