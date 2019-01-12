//
// Created by Anton Shelepov on 07/01/2019.
//

#include <QDirIterator>
#include "worker_t.h"
#include "searcher_t.h"
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QCloseEvent>
#include "trigram.h"

worker_t::worker_t() : index(), index_mutex(), searcher(new searcher_t(index, index_mutex)) {
    connect(searcher.get(), SIGNAL(release_entry(QString)), this, SLOT(return_entry(QString)));
    connect(searcher.get(), SIGNAL(finished()), this, SLOT(searching_finished()));
    connect(searcher.get(), SIGNAL(inc_progress_bar()), this, SLOT(inc_progress_bar()));
    //connect(&watcher, SIGNAL(fileChanged(const QString&)), this, SLOT(index_file(const QString&)));
}

worker_t::~worker_t() {
    shutdown_worker();
}

void worker_t::add_directory(QString path) {
    QDirIterator it(path, QDir::Hidden | QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QVector<QString> files;
    if (indexed.find(path) != indexed.end()) {
        return;
    }
    indexed.insert(path);
    deleted.erase(path);
    while (it.hasNext()) {
        if (shutdown) {
            return;
        }
        QString file = it.next();
        std::lock_guard<std::mutex> lock(index_mutex);
        if (index[file].empty()) {
            files.push_back(file);
        }
    }
    left_to_index[path] = files.size();
    for (auto& f : files) {
        index_file(path, f);
    }
}

void worker_t::remove_directory(QString path) {
    QDirIterator it(path, QDir::Hidden | QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    if (deleted.find(path) != deleted.end()) {
        return;
    }
    indexed.erase(path);
    deleted.insert(path);

    while (it.hasNext()) {
        if (shutdown) {
            return;
        }
        QString file = it.next();
        std::lock_guard<std::mutex> lock(index_mutex);
        index[file].clear();
    }

    emit directory_removed(path);
}

void worker_t::index_file(QString const& dir, QString const& path) {
    QtConcurrent::run(&index_pool, [this, dir, path]() {
        if (deleted.find(dir) != deleted.end()) {
            return;
        }
        auto trigrams = get_trigrams(path);
        std::lock_guard<std::mutex> lock1(index_mutex);
        if (trigrams.size() <= MAX_TRIGRAMS_NUMBER) {
            index[path] = std::move(trigrams);
        } else {
            index[path].clear();
        }
        std::lock_guard<std::mutex> lock2(left_to_index_mutex);
        left_to_index[dir]--;
        if (left_to_index[dir] % 500 == 0) {
            emit files_left(left_to_index[dir], dir);
        }
        //qDebug() << "trigrams were built: " + path;
    });
}

std::vector<trigram> worker_t::get_trigrams(QString const& path) {
    std::unordered_set<trigram> res;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream text_stream(&file);
        text_stream.setCodec("UTF-8");
        QString buffer;
        trigram cur = 0;
        while (true) {
            if (shutdown) {
                //qDebug() << "shutdown";
                return {};
            }
            buffer = text_stream.read(BUFFER_SIZE);
            if (buffer.isEmpty() || res.size() > MAX_TRIGRAMS_NUMBER) {
                break;
            }
            if (res.empty() && buffer.size() > 2) {
                next_trigram(cur, buffer[0]);
                next_trigram(cur, buffer[1]);
                if (!valid_for_text(buffer[0]) || !valid_for_text(buffer[1])) {
                    return {};
                }
            }
            for (size_t i = res.empty() ? 2 : 0; i < buffer.size(); i++) {
                if (!valid_for_text(buffer.at(i))) {
                    return {};
                }
                next_trigram(cur, buffer.at(i));
                res.insert(cur);
            }
        }
    } else {
        emit add_error("Couldn't open file: " + path);
    }
    std::vector<trigram> real_res;
    real_res.reserve(res.size());
    for (auto& x : res) {
        real_res.push_back(x);
    }
    std::sort(real_res.begin(), real_res.end());
    return real_res;
}

void worker_t::return_entry(QString path) {
    if (released.find(path) != released.end()) {
        return;
    }
    released.insert(path);
    waiting.push_back(path);
    try_release();
}

void worker_t::try_release(bool last) {
    if (searcher->isInterruptionRequested()) {
        return;
    }
    if (waiting.size() >= releasing_number || last) {
        emit release_entries(waiting, last);
        waiting.clear();
        if (last) {
            released.clear();
        }
    }
}

void worker_t::search_string(QString str) {
    emit set_progress_bar_max(index.size());
    search_progress = 0;
    releasing_number = index.size() / 10 + 1;
    searcher->set_string(str);
    searcher->start();
}

void worker_t::searching_finished() {
    try_release(true);
}

void worker_t::kill_pool() {
    shutdown = true;
    index_pool.clear();
    index_pool.waitForDone();
    shutdown = false;
}

void worker_t::kill_searcher() {
    searcher->requestInterruption();
    //search_pool.clear();
    //search_pool.waitForDone();
    searcher->wait();
//    search_progress = index.size();
//    inc_progress_bar();
    emit searcher_killed();
}

void worker_t::shutdown_worker() {
    kill_pool();
    kill_searcher();
}

void worker_t::inc_progress_bar() {
    search_progress++;
    if (search_progress % 100) {
        emit update_progress_bar(search_progress);
    }
}
