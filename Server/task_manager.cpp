#include <QtSql>

#include "task_manager.h"
// path to common includes
#include "..\common_includes\commands.h"

Task_manager::Task_manager()
{
}

void Task_manager::run()
{
    exec();
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
    qDebug() << "on recv msg: len = " << task.length();
    receive_queue.enqueue(task);

    if (!lock)
        handle_task();
}

void Task_manager::handle_task()
{
    QByteArray message = receive_queue.dequeue();
    QByteArray message_code;
    QTextStream stream(message);

    stream >> message_code;

    if (message[0] == SYSTEM_MESSAGE_START)
    {
        if (message_code == VALIDATION)
        {
            QByteArray signal, user_id, username, password;

            signal = VALIDATION;
            signal += ' ';
            stream >> user_id >> username >> password;

            if (!find_user(username))
            {
                if (validate_user(username, password))
                {
                    signal += GOOD;
                    signal += ' ' + username;
                    add_user(username);
                    send_message(VALIDATION, user_id, signal);
                }

                else
                {
                    signal += BAD;
                    send_message(VALIDATION, user_id, signal);
                }
            }

            else
            {
                signal += DUPLICATE;
                signal += ' ' + username;
                send_message(VALIDATION, user_id, signal);
            }
        }

        else if (message_code == SIGNUP)
        {
            QByteArray signal, user_id, username, password;

            signal = SIGNUP;
            signal += ' ';
            stream >> user_id >> username >> password;

            if (validate_signup(username, password))
            {
                signal += GOOD;
                signal += ' ' + username;
                send_message(VALIDATION, user_id, signal);
            }

            else
            {
                signal += BAD;
                send_message(VALIDATION, user_id, signal);
            }
        }

        else if (message_code == CONNECTED)
        {
            QByteArray signal, username;

            signal = CONNECTED;
            signal += ' ';
            stream >> username;

            signal += username;
            send_message(ALL, "default", signal);
        }

        else if (message_code == LOG){
            QByteArray signal, username, destination;
            stream >> username >> destination;
            QByteArray log = get_log(username, destination);
            if (log != "")
            {
                signal = LOG;
                signal += ' ' + destination + ' ' + log;
                send_message(USER, username, signal);
            }
            else
            {
                signal = LOG_FINISH;
                signal += ' ' + destination;
                send_message(USER, username, signal);
            }

        }

        else if (message_code == LOGPHOTO)
        {
            QByteArray signal, username, destination, id;
            stream >> username >> id;

            QByteArray log_entry = get_log_by_id(id.toInt());
            signal = LOGPHOTO;
            signal += ' ' + log_entry;
            send_message(USER, username, signal);
        }

        else if (message_code == UPDATE_LOG)
        {
            QByteArray signal, count, username, destination;
            stream >> username >> destination >> count;

            unsigned int old_count = count.toInt();
            unsigned int new_count = log_line_count(destination, destination);

            if (old_count == new_count)
            {
                signal = LOG_FINISH;
                signal += ' ' + destination;
                send_message(USER, username, signal);
            }

            else
            {
                QByteArray log = get_log_part(username, destination, old_count, new_count);

                signal = LOG;
                signal += ' ' + destination + ' ' + log;
                send_message(USER, username, signal);
            }
        }

        else if (message_code == UPDATE_LOG_TS)
        {
            QByteArray signal, ts, username, destination;
            stream >> username >> destination >> ts;

            unsigned int client_last_ts = ts.toInt();
            unsigned int server_last_ts = log_last_ts(username, destination);

            qDebug() << "UPDATE_LOG_TS: client_last_ts=" << client_last_ts << "; server_last_ts=" << server_last_ts;

            if (client_last_ts >= server_last_ts)
            {
                signal = LOG_FINISH;
                signal += ' ' + destination;
                send_message(USER, username, signal);
            }

            else
            {
                QByteArray log = get_log_part_ts(username, destination, client_last_ts+1, server_last_ts);

                signal = LOG;
                signal += ' ' + destination + ' ' + log;
                send_message(USER, username, signal);
            }
        }

        else if (message_code == LOG_CONTACTS)
        {
            QByteArray signal, username;

            signal = LOG_CONTACTS;
            signal += ' ';
            stream >> username;

            QByteArray contact_list = find_contacts_of_user(username);

            signal += contact_list;
            send_message(USER, username, signal);
        }

        else if (message_code == ADD_CONTACT)
        {
            QByteArray signal, username, contact;

            signal = ADD_CONTACT;
            signal += ' ';
            stream >> username >> contact;

            QByteArray contact_list = find_contact(contact);

            signal += contact_list;
            send_message(USER, username, signal);
        }

        else if (message_code == ADD_GROUP)
        {
            QByteArray signal, username, contact;

            signal = ADD_GROUP;
            signal += ' ';
            stream >> username >> contact;

            QByteArray group_list = find_group(contact);

            signal += group_list;
            send_message(USER, username, signal);
        }

        else if (message_code == JOIN_GROUP)
        {
            QByteArray signal, username, group;

            signal = JOIN_GROUP;
            signal += ' ';
            stream >> username >> group;

            if(join_group(username, group)){
                signal += GOOD;
            } else {
                signal += BAD;
            }
            send_message(USER, username, signal);
        }

        else if (message_code == CREATE_GROUP)
        {
            QByteArray signal, username, group;

            signal = CREATE_GROUP;
            signal += ' ';
            stream >> username >> group;
            signal += group;
            signal += ' ';

            if(create_group(username, group)){
                signal += GOOD;
            } else {
                signal += BAD;
            }
            send_message(USER, username, signal);
        }

        else if (message_code == IS_ONLINE)
        {
            QByteArray signal, username, contact;

            signal = IS_ONLINE;
            signal += ' ';

            stream >> contact >> username;

            if (find_user(contact))
            {
                signal += contact + ' ';
                signal += GOOD;
                send_message(USER, username, signal);
            }

            else
            {
                signal += contact + ' ';
                signal += BAD;
                send_message(USER, username, signal);
            }

        }

        else if (message_code == USER_COUNT)
        {
            QByteArray username;
            stream >> username;
            int user_count = get_user_count();

            QByteArray signal = QString("%1 %2").arg(USER_COUNT).arg(user_count).toUtf8();

            send_message(USER, username, signal);

        }

        else if (message_code == EXIT)
        {
            QByteArray signal, user_id, username;

            signal = DISCONNECTED;
            signal += ' ';
            stream >> user_id >> username;

            if (find_user(username))
            {
                delete_user(username);
                signal += username;
                send_message(EXIT, user_id, signal);
                disconnect();
            }
        }
    }

    else if (message_code == SEND)
    {
        QByteArray username, destination;

        stream >> username >> destination;

        QString s = stream.readAll();
        s.remove(0, 1);
        QByteArray fixed_message = s.toUtf8();
        //QByteArray fixed_message = message.remove(0,1);

        update_log(username,destination,fixed_message);
        if(is_destination_group(destination)){
            QList<QByteArray> users = get_group_users(destination);
            foreach (QByteArray group_user, users) {
                if(username.compare(group_user)!=0)
                    send_message(USER, group_user, message);
            }
        } else {
            send_message(USER, destination, message);
        }
    }
    else if (message_code == SENDPHOTO)
    {
        QByteArray username, destination;

        stream >> username >> destination;

        //QString s = stream.readAll();
        //s.remove(0, 1);
        //QByteArray fixed_message = s.toUtf8();

        QByteArray pix(message);
        update_log_photo(username, destination, pix.remove(0, QString(SENDPHOTO).length()+QString(username).length()+QString(destination).length()+3));
        //send_message(USER, destination, message);
        if(is_destination_group(destination)){
            QList<QByteArray> users = get_group_users(destination);
            foreach (QByteArray group_user, users) {
                if(username.compare(group_user)!=0)
                    send_message(USER, group_user, message);
            }
        } else {
            send_message(USER, destination, message);
        }
    }
}

/*
 * Возвращает true, если destination - это название группы, в противном случае - false
 */
bool Task_manager::is_destination_group(QByteArray destination) {
    QSqlQuery query;
    query.exec(QString("SELECT * FROM groups WHERE title='%1';").arg(destination));
    return query.next();
}

QList<QByteArray> Task_manager::get_group_users(QByteArray destination) {
    QList<QByteArray> result;
    QSqlQuery query;
    query.exec(QString("SELECT user FROM groups WHERE title='%1';").arg(destination));
    int user_column_num = query.record().indexOf("user");
    while(query.next()){
        result.append(query.value(user_column_num).toByteArray());
    }
    return result;
}

// validation
bool Task_manager::validate_user(QByteArray username, QByteArray password)
{
    QSqlQuery query;

    query.exec("select * from users where username='" + username + "' and password='" + password + "';");

    if (query.next())
        return true;

    return false;
}

bool Task_manager::validate_signup(QByteArray username, QByteArray password)
{
    QSqlQuery query;

    if (query.exec("insert into users (username, password) values ('" + username + "', '" + password + "');"))
        return true;
    qDebug() << query.lastError().text();
    return false;
}

// log
QByteArray Task_manager::get_log(QByteArray username, QByteArray destination)
{
    QSqlQuery query;

    if(is_destination_group(destination))
        query.exec("SELECT * FROM messages WHERE first_participant = '" + destination + "' OR second_participant = '" + destination + "' ORDER BY timestamp ASC;");
    else
        query.exec("select * from messages where(first_participant = '" + username + "' and second_participant = '" + destination + "')"
               "or (first_participant = '" + destination + "' and second_participant = '" + username + "') order by timestamp asc;");

    int id = query.record().indexOf("id");
    int first = query.record().indexOf("first_participant");
    int second = query.record().indexOf("second_participant");
    int message = query.record().indexOf("message");
    int timestamp = query.record().indexOf("timestamp");
    QByteArray log;

    while (query.next())
        if(!query.value(message).toString().startsWith(PHOTO))
            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + '\n';
        else
            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + ':' + query.value(id).toByteArray() + '\n';

    return log;
}

QList<int> Task_manager::get_log_ids(QByteArray username, QByteArray destination)
{
    QSqlQuery query;
    QList<int> id_list;

    query.exec("select * from messages where(first_participant = '" + username + "' and second_participant = '" + destination + "')"
               "or (first_participant = '" + destination + "' and second_participant = '" + username + "') order by timestamp asc;");

    int idN = query.record().indexOf("id");
    //int first = query.record().indexOf("first_participant");
    //int second = query.record().indexOf("second_participant");
    //int message = query.record().indexOf("message");
    //int timestamp = query.record().indexOf("timestamp");
    //QByteArray log;

    while (query.next())
        id_list.append(query.value(idN).toInt());
        //log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
        //query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + '\n';

    return id_list;
}

QByteArray Task_manager::get_log_by_id(int id)
{
    QSqlQuery query;

    query.exec(QString("SELECT * FROM messages WHERE id = %1;").arg(id));

    int first = query.record().indexOf("first_participant");
    int second = query.record().indexOf("second_participant");
    int message = query.record().indexOf("message");
    int timestamp = query.record().indexOf("timestamp");
    int raw = query.record().indexOf("raw");
    QByteArray log;

    while (query.next()){
        QString msg = query.value(message).toString();
        log.append(query.value(first).toByteArray());
        log.append(" ");
        log.append(query.value(second).toByteArray());
        log.append(" ");
        log.append(query.value(timestamp).toByteArray());
        //log.append(" ");
        //log.append(query.value(message).toByteArray());
        //if(msg.contains(PHOTO)){
            //photo
            log.append(" ");
            log.append(query.value(raw).toByteArray());
        //}
    }
    return log;
}


bool Task_manager::update_log(QByteArray username, QByteArray destination, QByteArray message)
{
    QSqlQuery query;
    //int timestamp = QDateTime::currentDateTime().toTime_t();
    int timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();

    if(message.startsWith(QByteArrayView(PHOTO))){
        query.prepare("INSERT INTO messages (first_participant, second_participant, message, timestamp, raw) values (:fp, :dest, :msg, :ts, :raw);");
        query.bindValue(":fp",username);
        query.bindValue(":dest",destination);
        query.bindValue(":msg",PHOTO);
        query.bindValue(":ts",timestamp);
        query.bindValue(":raw",message.remove(0,6));
        query.exec();
    } else

    if (query.exec("insert into messages (first_participant, second_participant, message, timestamp) values ('" + username + "', '" + destination + "', '" + message + "', " + QByteArray::number(timestamp) + ");"))
        return true;

    return false;
}

bool Task_manager::update_log_photo(QByteArray username, QByteArray destination, QByteArray message)
{
    QSqlQuery query;
    int timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();

    query.prepare("INSERT INTO messages (first_participant, second_participant, message, timestamp, raw) values (:fp, :dest, :msg, :ts, :raw);");
    query.bindValue(":fp",QString(username));
    query.bindValue(":dest",QString(destination));
    query.bindValue(":msg",PHOTO);
    query.bindValue(":ts",timestamp);
    query.bindValue(":raw",message);
    if(query.exec()) return true;
    else return false;
}

QByteArray Task_manager::get_log_part_ts(QByteArray username, QByteArray destination, unsigned int from_ts, unsigned int to_ts)
{
    if (from_ts >= to_ts)
        return ERROR;

    QSqlQuery query;

    if(is_destination_group(destination)){
        query.exec(QString("SELECT * FROM messages WHERE (first_participant = '%1' OR second_participant = '%1') AND timestamp BETWEEN %2 AND %3 ORDER BY timestamp DESC").arg(destination).arg(from_ts).arg(to_ts));
    } else {
        query.exec(QString("SELECT * FROM messages WHERE ((first_participant = '%1' AND second_participant = '%2')"
                           "OR (first_participant = '%2' AND second_participant = '%1')) AND timestamp BETWEEN %3 AND %4 ORDER BY timestamp DESC;")
                   .arg(username).arg(destination).arg(from_ts).arg(to_ts));
//        query.exec("select * from messages where (first_participant = '" + username + "' and second_participant = '" + destination + "')"
//            "or (first_participant = '" + destination + "' and second_participant = '" + username + "') order by timestamp desc;");
    }

    int id = query.record().indexOf("id");
    int first = query.record().indexOf("first_participant");
    int second = query.record().indexOf("second_participant");
    int message = query.record().indexOf("message");
    int timestamp = query.record().indexOf("timestamp");
    QByteArray log;

    while(query.next()){
        if(!query.value(message).toString().startsWith(PHOTO))
            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + '\n';
        else
            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + ':' + query.value(id).toByteArray() + '\n';

    }

//    for (unsigned int i = 0; i < (new_count - old_count); i++)
//    {
//        if (!query.next())
//            break;

//        if(!query.value(message).toString().startsWith(PHOTO))
//            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
//            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + '\n';
//        else
//            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
//            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + ':' + query.value(id).toByteArray() + '\n';

//        //log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
//        //    query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + '\n';
//    }

    return log;
}

QByteArray Task_manager::get_log_part(QByteArray username, QByteArray destination, unsigned int old_count, unsigned int new_count)
{
    if (old_count >= new_count)
        return ERROR;

    QSqlQuery query;

    if(is_destination_group(destination)){
        query.exec(QString("SELECT * FROM messages WHERE first_participant = '%1' or second_participant = '%1' ORDER BY timestamp DESC").arg(destination));
        //query.exec("SELECT * FROM messages WHERE first_participant = '" + destination + "' or second_participant = '" + destination + "'");
    } else {
        query.exec("select * from messages where (first_participant = '" + username + "' and second_participant = '" + destination + "')"
            "or (first_participant = '" + destination + "' and second_participant = '" + username + "') order by timestamp desc;");
    }

    int id = query.record().indexOf("id");
    int first = query.record().indexOf("first_participant");
    int second = query.record().indexOf("second_participant");
    int message = query.record().indexOf("message");
    int timestamp = query.record().indexOf("timestamp");
    QByteArray log;

    for (unsigned int i = 0; i < (new_count - old_count); i++)
    {
        if (!query.next())
            break;

        if(!query.value(message).toString().startsWith(PHOTO))
            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + '\n';
        else
            log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
            query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + ':' + query.value(id).toByteArray() + '\n';

        //log += query.value(first).toByteArray() + ' ' + query.value(second).toByteArray() + ' ' +
        //    query.value(timestamp).toByteArray() + ' ' + query.value(message).toByteArray() + '\n';
    }

    return log;
}

unsigned int Task_manager::log_last_ts(QByteArray username, QByteArray destination)
{
    QSqlQuery query;

    if(is_destination_group(destination)){
        query.exec(QString("SELECT COALESCE(MAX(timestamp),0) FROM messages WHERE first_participant = '%1' OR second_participant = '%1';").arg(destination));
    } else {
        query.exec(QString("SELECT COALESCE(MAX(timestamp),0) FROM messages WHERE(first_participant = '%1' AND second_participant = '%2')"
            "OR (first_participant = '%2' AND second_participant = '%1');").arg(username).arg(destination));
    }

    query.next();

    return query.value(0).toInt();
}

unsigned int Task_manager::log_line_count(QByteArray username, QByteArray destination)
{
    QSqlQuery query;

    if(is_destination_group(destination)){
        query.exec("SELECT COUNT(*) FROM messages WHERE first_participant = '" + destination + "' or second_participant = '" + destination + "';");
    } else {
        query.exec("select count(*) from messages where(first_participant = '" + username + "' and second_participant = '" + destination + "')"
            "or (first_participant = '" + destination + "' and second_participant = '" + username + "');");
    }

    query.next();

    return query.value(0).toInt();
}

//find contacts of user
QByteArray Task_manager::find_contacts_of_user(QByteArray username){
    qDebug() << "find_contacts_of_user(" << username << ")";
    QStringList result;
    QSqlQuery query;
    query.exec(QString("SELECT DISTINCT first_participant FROM messages WHERE second_participant = '%1'").arg(username));
    while(query.next()) result.append(query.value(0).toString() + ":0");
    query.exec(QString("SELECT DISTINCT second_participant FROM messages WHERE first_participant = '%1'").arg(username));
    while(query.next()) result.append(query.value(0).toString() + ":0");
    query.exec(QString("SELECT DISTINCT title FROM groups WHERE user = '%1'").arg(username));
    while(query.next()) result.append(query.value(0).toString() + ":1");
    qDebug() << result;
    result.removeDuplicates();
    return result.join(' ').toUtf8();
}

// find contact
QByteArray Task_manager::find_contact(QByteArray username, int count)
{
    QSqlQuery query;

    query.exec("select * from users where username like '%" + username + "%' order by length(username)");

    int index = query.record().indexOf("username");
    QByteArray log;

    while (query.next())
    {
        log += query.value(index).toByteArray() + ' ';

        if (--count == 0)
            break;
    }

    log.remove(log.length() - 1, 1);

    return log;
}

// find group
QByteArray Task_manager::find_group(QByteArray username, int count)
{
    QSqlQuery query;

    query.exec("SELECT DISTINCT title FROM groups WHERE title LIKE '%" + username + "%' ORDER BY LENGTH(title)");

    int index = query.record().indexOf("title");
    QByteArray log;

    while (query.next())
    {
        log += query.value(index).toByteArray() + ' ';

        if (--count == 0)
            break;
    }

    log.remove(log.length() - 1, 1);

    return log;
}

// create group
bool Task_manager::create_group(QByteArray username, QByteArray group)
{
    QSqlQuery query;

    query.exec("SELECT COUNT(title) FROM groups WHERE title = '" + group + "'");

    query.next();
    if(query.value(0).toInt()!=0){
        return false;
    } else {
        query.exec(QString("INSERT INTO groups (title, user) VALUES('%1', '%2')").arg(group).arg(username));
        return true;
    }
}

// join group
bool Task_manager::join_group(QByteArray username, QByteArray group)
{
    QSqlQuery query;

    query.exec("SELECT COUNT(title) FROM groups WHERE title = '" + group + "' AND user = '" + username + "'");

    query.next();
    if(query.value(0).toInt()!=0){
        return false;
    } else {
        query.exec(QString("INSERT INTO groups (title, user) VALUES('%1', '%2')").arg(group).arg(username));
        return true;
    }
}

int Task_manager::get_user_count()
{
    QSqlQuery query;

    query.exec("select COUNT(username) AS user_count from users;");

    int user_count = query.value(0).toInt();
    return user_count;
}

QString Task_manager::get_user_info(int user_no)
{
    QSqlQuery query;

    query.exec(QString("select username from users LIMIT 1 OFFSET %1;").arg(user_no));

    QString user_info = query.value(0).toString();
    return user_info;
}

// other
bool Task_manager::find_user(QByteArray username)
{
    QSqlQuery query;

    query.exec("select * from session where username='" + username + "';");

    if (query.next())
        return true;

    return false;
}

void Task_manager::add_user(QByteArray username)
{
    QSqlQuery query;

    query.exec("insert into session (username) values ('" + username + "');");
}

void Task_manager::delete_user(QByteArray username)
{
    QSqlQuery query;
    query.exec("delete from session where username='" + username + "';");
}
