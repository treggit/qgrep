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
#include "QString_std_hash.h"
#include <memory>
#include <mutex>

class worker_t : public QThread {
Q_OBJECT

public:
    worker_t();
    ~worker_t();

private slots:
    void add_directory(QString const& path, bool reindexed = false);
    void remove_directory(QString const& path);
    void search_string(QString const& str);
    void return_entry(QString const& path);
    void searching_finished();
    void index_file(QString const& dir, QString const& path);
    void kill_searcher();
    void kill_pool();
    void shutdown_worker();
    void inc_progress_bar();
    void reindex_file(QString path);
    void reindex_dir(QString path);

signals:
    void finished_indexing(QString const& path);
    void release_entries(QVector<QString> const& entries, bool last);
    void searcher_killed();
    void files_left(size_t num, QString const& dir);
    void directory_removed(QString const&);
    void add_error(QString const& error);
    void update_progress_bar(int);
    void set_progress_bar_max(int max);
    void interrupt_search();

private:
    QThreadPool index_pool;
    std::unordered_map<QString, std::vector<trigram>> index;
    std::mutex index_mutex;

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
    QFileSystemWatcher system_watcher;
};

#endif //QGREP_WORKER_H
