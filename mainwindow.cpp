#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWidgets>
#include <QTabWidget>
#include <QTableWidget>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    ui->tabWidget->setTabText(0, "Indexing");
    ui->tabWidget->setTabText(1, "Search");
    ui->tabWidget->setCurrentIndex(0);
    ui->directoriesList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->directoriesList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->entriesList->setContextMenuPolicy(Qt::CustomContextMenu);
    init_connections();
    worker_thread.start();

    qRegisterMetaType<QVector<QString>>();
    qRegisterMetaType<size_t>("size_t");
    ui->progressBar->reset();
}

void MainWindow::init_connections() {
    auto w = new worker_t();
    w->moveToThread(&worker_thread);

    connect(&worker_thread, SIGNAL(finished()), w, SLOT(deleteLater()));
    //connect(w, SIGNAL(finished_indexing(QString)), this, SLOT(update_indexing_status(QString)));
    connect(this, SIGNAL(process_add_directory(QString const&)), w, SLOT(add_directory(QString const&)));
    connect(this, SIGNAL(process_remove_directory(QString const&)), w, SLOT(remove_directory(QString const&)));
    connect(w, SIGNAL(files_left(size_t, QString const&)), this, SLOT(update_files_number(size_t, QString const&)));
    connect(w, SIGNAL(directory_removed(QString const&)), this, SLOT(remove_directory_from_list(QString const&)));

    connect(w, SIGNAL(add_error(QString)), this, SLOT(add_error(QString const&)));

    connect(ui->addButton, SIGNAL(clicked(bool)), this, SLOT(add_directory()));
    connect(ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(remove_directory()));
    connect(ui->findButton, SIGNAL(clicked(bool)), this, SLOT(start_search()));

    connect(ui->cancelSearchButton, SIGNAL(clicked(bool)), this, SLOT(try_cancel_search()));
    connect(this, SIGNAL(search_string(QString)), w, SLOT(search_string(QString const&)));
    connect(w, SIGNAL(release_entries(QVector<QString> const&, bool)), this, SLOT(add_string_entries(QVector<QString> const&, bool)));
    connect(this, SIGNAL(shutdown_worker()), w, SLOT(shutdown_worker()));

    connect(this, SIGNAL(interrupt_search()), w, SLOT(kill_searcher()));
    connect(w, SIGNAL(searcher_killed()), this, SLOT(search_cancelled()));

    connect(w, SIGNAL(set_progress_bar_max(int)), ui->progressBar, SLOT(setMaximum(int)));
    connect(w, SIGNAL(update_progress_bar(int)), ui->progressBar, SLOT(setValue(int)));

    connect(ui->entriesList, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(on_listWidget_customContextMenuRequested(const QPoint&)));

    connect(ui->entriesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(open_file()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::update_indexing_status(QString const& path, QString const& status) {
    auto item = get_item(path);
    if (item != nullptr) {
        item->setText(1, status);
    }
}

void MainWindow::update_files_number(size_t num, QString const& dir) {
    QString status;
    if (num == 0) {
        status = "Ready";
    } else {
        status = "Indexing, " + QString::number(num) + " files left";
    }

    auto item = get_item(dir);
    if (item != nullptr && item->text(1) != "Removing") {
        item->setText(1, status);
    }
}

void MainWindow::add_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Indexing",
                                                    QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        return;
    }
    auto x = get_item(dir);
    auto item = x == nullptr ? new QTreeWidgetItem() : x;
    if (item->text(1) != "Indexing") {
        item->setText(0, dir);
        item->setText(1, "Indexing");
    }
    if (x == nullptr) {
        ui->directoriesList->addTopLevelItem(item);
    }
    emit process_add_directory(dir);
}

void MainWindow::remove_directory() {
    if (ui->directoriesList->currentItem() == nullptr) {
        return;
    }
    auto answer = exec_message_box("Are you sure you want to remove directory?");
    if (answer == QMessageBox::Yes) {
        auto dir = ui->directoriesList->currentItem()->text(0);
        for (auto it = QTreeWidgetItemIterator(ui->directoriesList); *it; it++) {
            if ((*it)->text(0).startsWith(dir)) {
                (*it)->setText(1, "Removing");
                emit process_remove_directory((*it)->text(0));
            }
        }
    }
}


void MainWindow::start_search() {
    if (searching) {
        try_cancel_search();
    }
    if (cancelling) {
        QMessageBox msg_box;
        msg_box.setText("Cancelling at the moment");
        return;
    }
    QString str = ui->lineEdit->text();
    if (str.isEmpty()) {
        return;
    }
    ui->progressBar->reset();
    ui->searchStatus->setText("Searching");
    ui->entriesList->clear();
    searching = true;
    emit search_string(str);
}

void MainWindow::add_string_entries(QVector<QString> const& entries, bool last) {
    if (!searching) {
        return;
    }

    for (auto& e : entries) {
        ui->entriesList->addItem(e);
    }
    if (last) {
        searching = false;
        ui->progressBar->setValue(ui->progressBar->maximum());
        ui->searchStatus->setText("Done");
    }
}

void MainWindow::kill_worker() {
    emit shutdown_worker();
    worker_thread.quit();
    worker_thread.wait();
}


void MainWindow::closeEvent(QCloseEvent* event) {
    kill_worker();
}

int MainWindow::exec_message_box(QString const& message) {
    QMessageBox msg_box;
    msg_box.setText(message);
    msg_box.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msg_box.setDefaultButton(QMessageBox::Yes);
    return msg_box.exec();
}

void MainWindow::try_cancel_search() {
    if (!searching) {
        return;
    }
    emit interrupt_search();
    ui->searchStatus->setText("Cancelling");
    searching = false;
    cancelling = true;
}

void MainWindow::search_cancelled() {
    cancelling = false;
    ui->progressBar->setValue(0);
    ui->searchStatus->setText("Cancelled");
}

void MainWindow::remove_directory_from_list(QString const& dir) {
    auto item = get_item(dir);
    if (item->text(1) == "Removing") {
        delete item;
    }
}

QTreeWidgetItem* MainWindow::get_item(QString const& label) {
    for (auto it = QTreeWidgetItemIterator(ui->directoriesList); *it; it++) {
        if ((*it)->text(0) == label) {
            return *it;
        }
    }
    return nullptr;
}

void MainWindow::add_error(QString message) {
    ui->errorsList->addItem(message);
}

void MainWindow::open_file() {
    auto path = QUrl(ui->entriesList->currentItem()->data(0).toString());
    path.setScheme("file");
    bool ok = QDesktopServices::openUrl(path);
    if (!ok) {
        QMessageBox msg_box;
        msg_box.setText("Couldn't open: " + path.path());
        msg_box.exec();
    }
}

void MainWindow::open_file_in_folder() {
    auto path = ui->entriesList->currentItem()->data(0).toString();
    auto url = QUrl(path.mid(0, path.lastIndexOf('/')));
    url.setScheme("file");
    bool ok = QDesktopServices::openUrl(url);
    if (!ok) {
        QMessageBox msg_box;
        msg_box.setText("Couldn't open: " + url.path());
        msg_box.exec();
    }
}

void MainWindow::on_listWidget_customContextMenuRequested(const QPoint& pos) {
    auto menu = new QMenu(this);

    menu->addAction(QString("Open"), this, SLOT(open_file()));
    menu->addAction(QString("Open in folder"), this, SLOT(open_file_in_folder()));
    menu->popup(ui->entriesList->viewport()->mapToGlobal(pos));
}