//
// Created by Anton Shelepov on 07/01/2019.
//

#ifndef QGREP_WORKER_H
#define QGREP_WORKER_H

#include <QThread>
#include <unordered_map>
#include <unordered_set>
#include <QThreadPool>
#include <QFileSystemWatcher>
#include <QFuture>
#include <QVector>
#include "searcher_t.h"
#include "trigram.h"

class worker_t : public QThread {
Q_OBJECT

public:
    worker_t();
    ~worker_t();

private slots:
    void add_directory(QString path);
    void remove_directory(QString path);
    void search_string(QString str);
    void return_entry(QString path);
    void searching_finished();
    void index_file(QString const& dir, QString const& path);
    void kill_searcher();
    void kill_pool();
    void shutdown_worker();
    void inc_progress_bar();

signals:
    void finished_indexing(QString path);
    void release_entries(QVector<QString> entries, bool last);
    void searcher_killed();
    void files_left(size_t num, QString dir);
    void directory_removed(QString);
    void add_error(QString error);
    void update_progress_bar(int);
    void set_progress_bar_max(int max);
    void interrupt_search();

private:
    QThreadPool index_pool;
    std::unordered_map<QString, std::vector<trigram>> index;
    std::mutex index_mutex;

    std::unordered_set<QString> released;
    QVector<QString> waiting;

    std::unique_ptr<searcher_t> searcher;

    std::unordered_map<QString, size_t> left_to_index;
    std::mutex left_to_index_mutex;

    std::unordered_set<QString> indexed;
    std::unordered_set<QString> deleted;

    std::vector<trigram> get_trigrams(QString const& path);
    void try_release(bool last = false);
    bool shutdown = false;
    int search_progress = 0;

    const size_t BUFFER_SIZE = 1024;
    const size_t MAX_TRIGRAMS_NUMBER = 20000;
    size_t releasing_number = 1000;
};

#endif //QGREP_WORKER_H
