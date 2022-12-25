// path to common includes
#include "../common_includes/std.h"

#include <QFile>

#include "task_manager.h"
// path to common includes
#include "../common_includes/commands.h"

Task_manager::Task_manager(QObject *parent)
{
}

void Task_manager::run()
{
    exec();
}

void Task_manager::add_photo_to_queue(int id){
    photo_queve.enqueue(id);
}

void Task_manager::on_lock()
{
    lock = true;
}

void Task_manager::on_unlock()
{
    if (receive_queue.size() != 0)
        handle_task();

    lock = false;
}

void Task_manager::disconnect()
{
    exit(0);
}

void Task_manager::on_add_task(QByteArray task)
{
    receive_queue.enqueue(task);

    if (!lock)
        handle_task();
}

void Task_manager::start_get_photos(QByteArray contact){
    photo_contact = contact;
}

void Task_manager::handle_task()
{
    QByteArray message = receive_queue.dequeue();
    QTextStream stream(message);
    QByteArray message_code[2];

    stream >> message_code[0];

    if (message_code[0][0] == SYSTEM_MESSAGE_START)
    {
        if (message_code[0] == ID)
        {
            QByteArray user_id;

            stream >> user_id;
            emit(set_user_id(user_id));
        }

        if (message_code[0] == VALIDATION)
        {
            QByteArray username;

            stream >> message_code[1];

            if (message_code[1] == GOOD)
            {
                stream >> username;
                emit(set_username(username));
                emit(login_true());
            }

            else if (message_code[1] == BAD)
                emit(login_false());

            else if (message_code[1] == DUPLICATE)
                emit(login_duplicate());
        }

        else if (message_code[0] == SIGNUP)
        {
            stream >> message_code[1];

            if (message_code[1] == GOOD)
                emit(signup_true());

            else if (message_code[1] == BAD)
                emit(signup_false());
        }

        else if (message_code[0] == IS_ONLINE)
        {
            QByteArray contact;

            stream >> contact >> message_code[1];

            if (message_code[1] == GOOD)
                emit(contact_online(contact));

            else if (message_code[1] == BAD)
                emit(contact_offline(contact));
        }

        else if (message_code[0] == CREATE_GROUP)
        {
            QByteArray group;

            stream >> group >> message_code[1];

            if (message_code[1] == GOOD)
                emit create_group(group, true);
            else
                emit create_group(group, false);
        }

        else if (message_code[0] == LOG_CONTACTS)
        {
            QByteArray contact;
            //stream >> contact;
            QString s = stream.readAll();

            s.remove(0, 1);
            emit contacts_received(s.toUtf8());
        }

        else if (message_code[0] == LOG)
        {
            QByteArray contact;
            stream >> contact;
            QString s = stream.readAll();

            s.remove(0, 1);
            emit(create_log(contact, s.toUtf8()));
        }

        else if (message_code[0] == LOGPHOTO)
        {
            QByteArray contact;
            stream >> contact;
            QString s = stream.readAll();

            s.remove(0, 1);
            emit create_log_photo(contact, message.remove(0,QString(LOGPHOTO).length()+1));
        }

        else if (message_code[0] == LOG_FINISH)
        {
            QString contact;
            stream >> contact;

            emit(update_log(contact));
        }

        else if (message_code[0] == CONNECTED)
        {
            QByteArray name;

            stream >> name;
            emit(someone_connected(name));
        }

        else if (message_code[0] == DISCONNECTED)
        {
            QByteArray name;

            stream >> name;
            emit(someone_disconnected(name));
        }

        else if (message_code[0] == ADD_CONTACT)
        {
            QString s = stream.readAll();
            s.remove(0, 1);

            if (s.length() != 0)
            {
                QStringList contact_list = s.split(" ");
                add_contacts(contact_list);
            }
        }

        else if (message_code[0] == ADD_GROUP)
        {
            QString s = stream.readAll();
            s.remove(0, 1);

            if (s.length() != 0)
            {
                QStringList group_list = s.split(" ");
                qDebug() << "msg: on_add_group";
                add_groups(group_list);
            }
        }
    }

    else if (message_code[0] == SEND)
    {
        QByteArray username, destination;

        stream >> username >> destination;
        QString s = stream.readAll();
        s.remove(0, 1);

        emit receive_message(username, destination, s);
    }

    else if (message_code[0] == SENDPHOTO)
    {
        QByteArray username, destination;

        stream >> username >> destination;
        //QString s = stream.readAll();
        //s.remove(0, 1);

        emit receive_message_photo(username, destination, message.remove(0, QString(SENDPHOTO).length()+QString(username).length()+QString(destination).length()+3));
    }

    else if (message_code[0] == "test")
    {
        emit(reconnect());
    }
}
