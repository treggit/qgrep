//
// Created by Anton Shelepov on 07/01/2019.
//

#ifndef QGREP_SEARCHER_H
#define QGREP_SEARCHER_H

#include <QThread>
#include <unordered_map>
#include <unordered_set>
#include <QThreadPool>
#include <QFileSystemWatcher>
#include <QFuture>
#include <QVector>
#include "trigram.h"
#include "QString_std_hash.h"
#include <mutex>

class searcher_t : public QThread {
Q_OBJECT

public:
    searcher_t(std::unordered_map<QString, std::vector<trigram>>& ind, std::mutex& m);
    ~searcher_t();

    void run() override;
    void set_string(QString const& str);

signals:
    void release_entry(QString path);
    void inc_progress_bar();

//public slots:
//    void interrupt_search();

private:
    std::unordered_map<QString, std::vector<trigram>>& index;
    std::mutex& index_mutex;
    QString string_to_find;
    QThreadPool search_pool;

    QFuture<void> find_string_in_file(QString const& str, QString const& path);
    bool contains(QString const& path, QString const& str);
    bool check_trigrams(QString const& path, QString const& str);
    bool prefix_function_check(QVector<size_t>& p, QString const& s, size_t need, size_t start_from = 1);
    void search_string(QString const& str);

    bool interrupted = false;
    const size_t BUFFER_SIZE = 1024;
};

#endif //QGREP_SEARCHER_H
