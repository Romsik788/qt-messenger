#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <QThread>
#include <QQueue>
#include <QFile>

// path to common includes
#include "..\common_includes/std.h"

class Task_manager : public QThread
{
    Q_OBJECT

public:
    explicit Task_manager();

signals:
    void send_message(QByteArray, QByteArray, QByteArray);

private slots:
    void on_add_task(QByteArray);
    void on_lock();
    void on_unlock();

private:
    QQueue<QByteArray> receive_queue;
    bool lock = false;
    bool file_mode = false;
    QByteArray file_buffer;
    QByteArray file_path;
    int file_size;

private:
    void handle_task();
    bool validate_user(QByteArray, QByteArray);
    bool validate_signup(QByteArray, QByteArray);
    QByteArray get_log(QByteArray, QByteArray);
    QList<int> get_log_ids(QByteArray, QByteArray);
    QByteArray get_log_by_id(int);
    bool update_log(QByteArray, QByteArray, QByteArray);
    bool update_log_photo(QByteArray, QByteArray, QByteArray);
    QByteArray get_log_part(QByteArray, QByteArray, unsigned int, unsigned int);
    QByteArray get_log_part_ts(QByteArray, QByteArray, unsigned int, unsigned int);
    unsigned int log_line_count(QByteArray, QByteArray);
    unsigned int log_last_ts(QByteArray, QByteArray);
    QByteArray find_contacts_of_user(QByteArray);
    QByteArray find_contact(QByteArray, int count = max_contact_search);
    QByteArray find_group(QByteArray, int count = max_contact_search);
    bool create_group(QByteArray, QByteArray);
    bool join_group(QByteArray, QByteArray);
    int get_user_count();
    QString get_user_info(int user_no);
    bool find_user(QByteArray);
    void add_user(QByteArray);
    void delete_user(QByteArray);
    void disconnect();
    bool is_destination_group(QByteArray);
    QList<QByteArray> get_group_users(QByteArray);

public:
    void run();
};

#endif
