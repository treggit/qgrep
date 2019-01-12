//
// Created by Anton Shelepov on 08/01/2019.
//

#include "searcher_t.h"

#include <QtConcurrent/QtConcurrent>

searcher_t::searcher_t(std::unordered_map<QString, std::vector<trigram>>& ind, std::mutex& m)
        : index(ind), index_mutex(m)
{

}

searcher_t::~searcher_t() {
    search_pool.clear();
    search_pool.waitForDone();
}

void searcher_t::search_string(QString const& str) {
    QVector<QFuture<void>> future;
    std::unique_lock<std::mutex> lock(index_mutex);
    for (auto& i : index) {
        future.append(find_string_in_file(str, i.first));
    }
    lock.unlock();
    for (auto& f : future) {
        f.waitForFinished();
    }
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
        QVector<size_t> p(str.size() + 1);
        prefix_function_check(p, buffer, buffer.size());
        QString add;
        if (isInterruptionRequested()) {
            return false;
        }
        while (!(add = text_stream.read(BUFFER_SIZE)).isEmpty()) {
            if (isInterruptionRequested()) {
                return false;
            }
            buffer.append(add);
            if (prefix_function_check(p, buffer, str.size(), str.size() + 1)) {
                return true;
            }
            buffer = buffer.mid(0, str.size() + 1) + buffer.mid(buffer.size() - str.size(), str.size());

            if (isInterruptionRequested()) {
                return false;
            }
        }
    }

    return false;
}

bool searcher_t::check_trigrams(QString const& path, QString const& str) {
    if (str.size() < 3) {
        return true;
    }
    trigram cur = 0;
    std::lock_guard<std::mutex> lock(index_mutex);
    auto& ind = index[path];
    next_trigram(cur, str[0]);
    next_trigram(cur, str[1]);
    for (size_t i = 2; i < str.size(); i++) {
        next_trigram(cur, str.at(i));
        if (std::lower_bound(ind.begin(), ind.end(), cur) == ind.end()) {
            return false;
        }
    }
    return true;
}

bool searcher_t::prefix_function_check(QVector<size_t>& p, QString const& s, size_t need, size_t start_from) {
    p.resize(s.size());
    for (size_t i = start_from; i < s.size(); i++) {
        size_t j = p[i - 1];
        while (s.at(i) != s.at(j) && j > 0) {
            j = p[j - 1];
        }
        if (s.at(i) == s.at(j)) {
            j++;
        }
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
