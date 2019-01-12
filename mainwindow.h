#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <worker_t.h>
#include <QTreeWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void add_directory();
    void remove_directory();
    void start_search();
    void add_string_entries(QVector<QString> const& entries, bool last);
    void try_cancel_search();
    void search_cancelled();
    void update_files_number(size_t num, QString dir);
    void update_indexing_status(QString const& path, QString const& status);
    void remove_directory_from_list(QString dir);
    void add_error(QString message);
    void on_listWidget_customContextMenuRequested(const QPoint& pos);
    void open_file();
    void open_file_in_folder();

signals:
    void process_add_directory(QString dir);
    void process_remove_directory(QString dir);
    void search_string(QString str);
    void interrupt_search();
    void shutdown_worker();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    QThread worker_thread;
    bool searching = false;
    bool cancelling = false;

    void init_connections();
    void kill_worker();
    void closeEvent(QCloseEvent* event);
    int exec_message_box(QString const& message);
    QTreeWidgetItem* get_item(QString const& label);
};

#endif // MAINWINDOW_H
