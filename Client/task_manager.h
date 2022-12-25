#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <QThread>
#include <QQueue>

class Task_manager : public QThread
{
    Q_OBJECT

public:
    explicit Task_manager(QObject *parent = 0);

signals:
    void login_true();
    void contacts_received(QByteArray);
    void login_false();
    void login_duplicate();
    void signup_true();
    void signup_false();
    void contact_online(QByteArray);
    void contact_offline(QByteArray);
    void create_group(QString, bool);
    void update_log(QString);
    void create_log(QByteArray, QByteArray);
    void create_log_photo(QByteArray, QByteArray);
    void create_log_finish(QByteArray);
    void someone_connected(QByteArray);
    void someone_disconnected(QByteArray);
    void receive_message(QByteArray, QByteArray, QString);
    void receive_message_photo(QByteArray, QByteArray, QByteArray);
    void add_contacts(QStringList);
    void add_groups(QStringList);
    void set_username(QByteArray);
    void set_user_id(QByteArray);
    void reconnect();

private slots:
    void on_add_task(QByteArray);
    void on_lock();
    void on_unlock();

private:
    QQueue<QByteArray> receive_queue;
    QQueue<int> photo_queve;
    QByteArray photo_contact;
    bool lock = false;

private:
    void handle_task();

public:
    void run();
    void disconnect();
    void add_photo_to_queue(int id);
    void start_get_photos(QByteArray);
};

#endif
