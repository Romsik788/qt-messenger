// path to common includes
#include "../common_includes/std.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QProcess>
#include <QSoundEffect>
#include <QFileDialog>
#include <QInputDialog>
#include <QtUiTools/QUiLoader>
#include <QMessageBox>

// path to common includes
#include "../common_includes/commands.h"
#include "window.h"
#include "task_manager.h"

// error messages
QString error[] = {
                    "Username and/or password is not correct. Please try again.",
                    "Cannot connect to server. Please try again later.",
                    "Please enter username and password.",
                    "Username already taken. Please try again.",
                    "Passwords do not match. Please try again.",
                    "Please confirm password.",
                    "Username cannot contain spaces. Please try again.",
                    "User already logged in.",
                    "Error occured. Please restart the program."};

QString info[] = { "Connecting to server..." };

// stylesheets
QString button_clicked = "QPushButton:enabled"
                         "{"
                         "  background: #585879;"
                         "  min-width: 40px;"
                         "  min-height:40px;"
                         "  border-radius: 0px;"
                         "  padding: 5px;"
                         "  outline: 0px;"
                         "}";

QString button_unclicked = "QPushButton:enabled"
                           "{"
                           "    background: #1c1c37;"
                           "    min-width: 40px;"
                           "    min-height:40px;"
                           "    border-radius: 0px;"
                           "    padding: 5px;"
                           "    outline: 0px;"
                           "}"
                           "QPushButton:pressed"
                           "{"
                           "    background: #585879;"
                           "}"
                           "QPushButton:hover"
                           "{"
                           "    background: #3D3D5A;"
                           "}";

QString user_style = "background: #73739b;"
                     "border-radius: 5px;"
                     "color: black;"
                     "font:10pt \"Open Sans\";";

QString contact_name_style = "background: #73739b;"
                     "border-top-right-radius: 5px;"
                     "border-top-left-radius: 5px;"
                     "border-bottom-right-radius: 0px;"
                     "border-bottom-left-radius: 0px;"
                     "color: black;"
                     "font:10pt \"Open Sans\";";

QString contact_style = "background: white;"
                        "border-radius: 5px;"
                        "color: black;"
                        "font:10pt \"Open Sans\";";

QString user_bubble_style = "background: white;"
                            "border-radius: 20;"
                            "color:  #73739b;"
                            "font: 20pt \"Open Sans\";";

QString contact_bubble_style = "background: #73739b;"
                               "border-radius: 20;"
                               "color: white;"
                               "font: 20pt \"Open Sans\";";

QString add_button_style = "QPushButton"
                           "{"
                           "    font: 11pt \"Open Sans Semibold\";"
                           "    background:  #73739b;"
                           "    border-radius: 0px;"
                           "    min-height:25px;"
                           "    padding: 5px;"
                           "    outline: 0 px;"
                           "    color: white;"
                           "}"
                           "QPushButton:hover"
                           "{"
                           "    background: #7a7aa1;"
                           "}"
                           "QPushButton:pressed"
                           "{"
                           "    background: #8f8fb3;"
                           "}";


Window::Window(QWidget *parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.stackedWidget->setCurrentWidget(ui.login);
    this->statusBar()->hide();
    this->menuBar()->hide();
    this->setMaximumSize(ui.login->minimumSize());

    // database
    db.setDatabaseName(client_database_path);

    if (!QFile::exists(client_database_path))
    {
        bool ok;
        QString text = QInputDialog::getText(this, tr("Server host init"),
                                                 tr("Host:"), QLineEdit::Normal,
                                                 "127.0.0.1", &ok);
        if(!ok){
            exit(1);
        }
        if (db.open())
        {
            QSqlQuery query;

            query.exec("CREATE TABLE messages (id INTEGER PRIMARY KEY AUTOINCREMENT, first_participant TEXT, second_participant TEXT, message TEXT, timestamp INTEGER, raw BLOB);");
            query.exec("CREATE TABLE connection (ip_address TEXT, port INTEGER);");
            query.exec(QString("INSERT INTO connection (ip_address, port) VALUES ('%1', 8084)").arg(text));
        }
    }

    if (!db.isOpen())
    {
        if (!db.open())
        {
            ui.login_error->setText(error[8]);
            ui.signup_error->setText(error[8]);
        }
    }

    // signal/slot connection
    // context menu
    ui.contacts->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.contacts, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    //qrc
    QString fileName(":/text/Resources/form.txt");

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug()<<"filenot opened"<<Qt::endl;
    }
    else
    {
        qDebug()<<"file opened"<<Qt::endl;
        layout_template = file.readAll();
    }

    file.close();

    // scrollbars
    //QScrollBar* scrollbar = ui.scrollArea->verticalScrollBar();
    //QScrollBar* scrollbar_1 = ui.scrollArea_2->verticalScrollBar();
    //QScrollBar* scrollbar_2 = ui.scrollArea_4->verticalScrollBar();

    //connect(scrollbar, SIGNAL(rangeChanged(int, int)), this, SLOT(auto_scroll(int, int)));
    //connect(scrollbar_1, SIGNAL(rangeChanged(int, int)), this, SLOT(auto_scroll(int, int)));
    //connect(scrollbar_2, SIGNAL(rangeChanged(int, int)), this, SLOT(auto_scroll(int, int)));

    // connection
    connection = new Connection();
    connection->set_user(&user);

    connect(connection, SIGNAL(connected()), this, SLOT(on_socket_connected()));
    connect(connection, SIGNAL(error()), this, SLOT(on_socket_error()));

    // info message
    ui.login_error->setText(info[0]);
    ui.signup_error->setText(info[0]);

    // task manager
    task_manager = new Task_manager();
    task_manager->start();

    connect(this, SIGNAL(lock()), task_manager, SLOT(on_lock()));
    connect(this, SIGNAL(unlock()), task_manager, SLOT(on_unlock()));
    connect(connection, SIGNAL(add_task(QByteArray)), task_manager, SLOT(on_add_task(QByteArray)));
    connect(task_manager, SIGNAL(login_true()), this, SLOT(on_login_true()));
    connect(task_manager, &Task_manager::contacts_received, this, &Window::on_contacts_received);
    connect(task_manager, SIGNAL(login_false()), this, SLOT(on_login_false()));
    connect(task_manager, SIGNAL(login_duplicate()), this, SLOT(on_login_duplicate()));
    connect(task_manager, SIGNAL(signup_true()), this, SLOT(on_signup_true()));
    connect(task_manager, SIGNAL(signup_false()), this, SLOT(on_signup_false()));
    connect(task_manager, SIGNAL(contact_online(QByteArray)), this, SLOT(on_contact_online(QByteArray)));
    connect(task_manager, SIGNAL(contact_offline(QByteArray)), this, SLOT(on_contact_offline(QByteArray)));
    connect(task_manager, SIGNAL(update_log(QString)), this, SLOT(on_update_log(QString)));
    connect(task_manager, SIGNAL(create_log(QByteArray, QByteArray)), this, SLOT(on_create_log(QByteArray, QByteArray)));
    connect(task_manager, SIGNAL(create_log_photo(QByteArray, QByteArray)), this, SLOT(on_create_log_photo(QByteArray, QByteArray)));
    connect(task_manager, SIGNAL(someone_connected(QByteArray)), this, SLOT(on_someone_connected(QByteArray)));
    connect(task_manager, SIGNAL(someone_disconnected(QByteArray)), this, SLOT(on_someone_disconnected(QByteArray)));
    connect(task_manager, SIGNAL(receive_message(QByteArray, QByteArray, QString)), this, SLOT(on_receive_message(QByteArray, QByteArray, QString)));
    connect(task_manager, SIGNAL(receive_message_photo(QByteArray, QByteArray, QByteArray)), this, SLOT(on_receive_message_photo(QByteArray, QByteArray, QByteArray)));
    connect(task_manager, SIGNAL(add_contacts(QStringList)), this, SLOT(on_add_contacts(QStringList)));
    connect(task_manager, &Task_manager::add_groups, this, &Window::on_add_groups);
    connect(task_manager, &Task_manager::create_group, this, &Window::on_create_group);
    connect(task_manager, SIGNAL(set_username(QByteArray)), connection, SLOT(on_set_username(QByteArray)));
    connect(task_manager, SIGNAL(set_user_id(QByteArray)), connection, SLOT(on_set_user_id(QByteArray)));
    connect(task_manager, SIGNAL(reconnect()), this, SLOT(on_login_button_clicked()));
    connect(task_manager, SIGNAL(create_log_finish(QByteArray)), this, SLOT(on_create_log_finish(QByteArray)));

    // Contact queue fill
    /*
    data.replace("%1","8");
    QByteArray ba = data.toUtf8();
    QBuffer buf(&ba);
    QUiLoader loader;
    loader.load(&buf,ui.chat_windows);
    QWidget *page = ui.chat_windows->findChild<QWidget *>("page_8",Qt::FindChildrenRecursively);
    QVBoxLayout *output = ui.chat_windows->findChild<QVBoxLayout *>("output_8",Qt::FindChildrenRecursively);
    qDebug() << page;
    qDebug() << output;
    Contact *contact = new Contact(ui.page, ui.output, ui.contact_label, ui.contact_status);
    Contact *contact_1 = new Contact(ui.page_1, ui.output_2, ui.contact_label_2, ui.contact_status_2);
    Contact *contact_2 = new Contact(ui.page_2, ui.output_3, ui.contact_label_4, ui.contact_status_4);

    contact_queue.enqueue(contact);
    contact_queue.enqueue(contact_1);
    contact_queue.enqueue(contact_2);
    */

    layout_counter = 4;
    add_layout(1);
    add_layout(2);
    add_layout(3);
    add_layout(4);

    // status gif
    ui.status_label->setMovie(status_gif);
    ui.login_status_label->setMovie(status_gif);
    ui.signup_status_label->setMovie(status_gif);
    ui.status_label->hide();
    ui.login_status_label->hide();
    ui.signup_status_label->hide();
}

void Window::add_layout(int index){
    QString cur_layout = layout_template;
    cur_layout.replace("%1", QString("%1").arg(index));

    QByteArray ba = cur_layout.toUtf8();
    QBuffer buf(&ba);
    QUiLoader loader;
    loader.load(&buf,ui.chat_windows);
    QWidget *page = ui.chat_windows->findChild<QWidget *>(QString("page_%1").arg(index),Qt::FindChildrenRecursively);
    QVBoxLayout *output = ui.chat_windows->findChild<QVBoxLayout *>(QString("output_%1").arg(index),Qt::FindChildrenRecursively);
    QLabel *contact_label = ui.chat_windows->findChild<QLabel *>(QString("contact_label_%1").arg(index),Qt::FindChildrenRecursively);
    QLabel *contact_status = ui.chat_windows->findChild<QLabel *>(QString("contact_status_%1").arg(index),Qt::FindChildrenRecursively);
    Contact *contact = new Contact(page,output,contact_label,contact_status);

    contact->status_label->setText(" ");

    // scrollbars
    QScrollBar* scrollbar = ui.chat_windows->findChild<QScrollArea *>(QString("scrollArea_%1").arg(index),Qt::FindChildrenRecursively)->verticalScrollBar();
    connect(scrollbar, &QScrollBar::rangeChanged, this, &Window::auto_scroll);

    contact_queue.enqueue(contact);
}

// connection
void Window::on_socket_connected()
{
    emit(lock());

    connection->send_message(PASSWORD);

    if ((ui.stackedWidget->currentWidget() == ui.login) || (ui.stackedWidget->currentWidget() == ui.signup))
    {
        ui.login_error->setText("");
        ui.signup_error->setText("");
    }

    if (status_gif->Running)
    {
        status_gif->stop();
        ui.status_label->hide();
        ui.login_status_label->hide();
        ui.signup_status_label->hide();
        ui.status_text_label->hide();
    }

    emit(unlock());
}

void Window::on_socket_error()
{
    emit(lock());

    if ((ui.stackedWidget->currentWidget() == ui.login) || (ui.stackedWidget->currentWidget() == ui.signup))
    {
        ui.login_error->setText(error[1]);
        ui.signup_error->setText(error[1]);
    }

    ui.status_label->show();
    ui.status_text_label->show();
    ui.login_status_label->show();
    ui.signup_status_label->show();
    status_gif->start();
    emit(unlock());
}

// login, signup
void Window::on_login_button_clicked()
{
    emit(lock());
    if (connection->get_status())
    {
        QString username_check = ui.login_username->text();
        QString password_check = ui.login_password->text();

        if ((username_check == "") || (password_check == ""))
            ui.login_error->setText(error[2]);

        else
        {
            QByteArray signal = VALIDATION;
            signal.append( ' ' + user.get_user_id() + ' ' + username_check.toUtf8() + ' ' + password_check.toUtf8());
            qDebug() << "singal: " << signal;
            connection->send_message(signal);
        }

        ui.login_username->setFocus();
    }

    emit(unlock());
}

void Window::on_signup_button_clicked()
{
    emit(lock());

    if (connection->get_status())
    {
        QString username_check = ui.signup_username->text();
        QString password_check = ui.signup_password->text();
        QString password_confirm = ui.signup_confirm->text();

        if ((username_check == "") || (password_check == ""))
            ui.signup_error->setText(error[2]);

        else if ((username_check != "") && (password_check != "") && (password_confirm == ""))
            ui.signup_error->setText(error[5]);

        else if (password_check != password_confirm)
            ui.signup_error->setText(error[4]);

        else if (username_check.contains(' '))
            ui.signup_error->setText(error[6]);

        else
        {
            QByteArray signal = SIGNUP;
            signal.append( ' ' + user.get_user_id() + ' ' + username_check.toUtf8() + ' ' + password_check.toUtf8());
            connection->send_message(signal);
        }
    }
    emit(unlock());
}

void Window::on_login_signup_clicked()
{
    emit(lock());
    ui.stackedWidget->setCurrentWidget(ui.signup);
    emit(unlock());
}

void Window::on_signup_login_clicked()
{
    emit(lock());
    ui.stackedWidget->setCurrentWidget(ui.login);
    emit(unlock());
}

void Window::on_signup_true()
{
    emit(lock());

    ui.login_username->setText(ui.signup_username->text());
    ui.login_password->setText(ui.signup_password->text());
    on_login_button_clicked();

    emit(unlock());
}

void Window::on_signup_false()
{
    emit(lock());
    ui.signup_error->setText(error[3]);
    emit(unlock());
}

void Window::on_login_true()
{
    QByteArray signal = LOG_CONTACTS;
    signal.append(' ' + user.get_username());
    connection->send_message(signal);

//    emit(lock());

//    QSqlQuery query;

//    query.exec("SELECT * FROM sqlite_master WHERE name ='" + user.get_username() + "' and type='table'");

//    if (!query.next())
//        query.exec("CREATE TABLE " + user.get_username() + " (username TEXT PRIMARY KEY, type INTEGER DEFAULT (0));");

//    ui.status_text_label->hide();
//    ui.file_button->setEnabled(true);
//    user.set_password(ui.login_password->text().toUtf8());
//    ui.stackedWidget->setCurrentWidget(ui.main);
//    ui.chat_windows->setCurrentWidget(ui.blank);
//    this->setFixedWidth(600);
//    this->setMaximumSize(QSize(16777215, 16777215));
//    this->menuBar()->show();
//    ui.send_button->setEnabled(false);
//    ui.username_label->setText(ui.login_username->text());
//    ui.input->setFocus();
//    refresh_contacts();
//    on_contact_button_clicked();

//    emit(unlock());
}

void Window::on_login_false()
{
    emit(lock());
    ui.login_error->setText(error[0]);
    emit(unlock());
}

void Window::on_login_duplicate()
{
    emit(lock());
    ui.login_error->setText(error[7]);
    emit(unlock());
}

void Window::on_contacts_received(QByteArray contacts){
    emit lock();

    QSqlQuery query;

    query.exec("SELECT * FROM sqlite_master WHERE name ='" + user.get_username() + "' and type='table'");

    if (!query.next())
        query.exec("CREATE TABLE " + user.get_username() + " (username TEXT PRIMARY KEY, type INTEGER DEFAULT (0));");

    qDebug() << "on_contacts_received(" << contacts << ")";
    QString table_name = user.get_username();
    QString contacts_str = QString(contacts);
    QStringList contact_list = contacts_str.split(' ', Qt::SkipEmptyParts);
    //if(contact_list.size()==0 && !contacts_str.isEmpty()) contact_list.append(QString(contacts_str));
    qDebug() << "size=" << contact_list.size();
    foreach (QString contact, contact_list) {
        QStringList contact_item = contact.split(':');
        query.exec(QString("SELECT COUNT(username) FROM %1 WHERE username = '%2' AND type = %3").arg(table_name).arg(contact_item[0]).arg(contact_item[1].toInt()));
        if(query.next() && query.value(0).toInt() == 0){
            query.exec(QString("INSERT INTO %1 (username,type) VALUES ('%2',%3)").arg(table_name).arg(contact_item[0]).arg(contact_item[1].toInt()));
        }
    }

    ui.status_text_label->hide();
    ui.file_button->setEnabled(true);
    user.set_password(ui.login_password->text().toUtf8());
    ui.stackedWidget->setCurrentWidget(ui.main);
    ui.chat_windows->setCurrentWidget(ui.blank);
    this->setFixedWidth(600);
    this->setMaximumSize(QSize(16777215, 16777215));
    this->menuBar()->show();
    ui.send_button->setEnabled(false);
    ui.username_label->setText(ui.login_username->text());
    ui.input->setFocus();
    refresh_contacts();
    on_contact_button_clicked();

    emit unlock();
}

// chat
void Window::on_input_textChanged()
{
    emit(lock());

    if (ui.contacts->selectedItems().count() != 0)
    {
        int message_length = ui.input->toPlainText().length();

        if (message_length > 0)
        {
            ui.send_button->setEnabled(true);

            if (ui.input->toPlainText()[ui.input->toPlainText().length() - 1] == '\n')
            {
                if (message_length != 1)
                {
                    ui.input->textCursor().deletePreviousChar();
                    on_send_button_clicked();
                }

                else
                    ui.input->textCursor().deletePreviousChar();
            }
        }

        else
            ui.send_button->setEnabled(false);
    }

    int line_count = ui.input->document()->lineCount();
    int input_frame_box_size = 48;
    int input_size = 48;
    int input_frame_size = 75;
    int inc = 18;

    if (line_count == 1)
    {
        ui.input->setFixedHeight(input_size);
        ui.input_frame_box->setFixedHeight(input_frame_box_size);
        ui.input_frame->setFixedHeight(input_frame_size);
        ui.horizontalLayout_3->setContentsMargins(0, 11, 0, 0);
    }

    else if (line_count == 2)
    {
        ui.input->setFixedHeight(input_size);
        ui.input_frame_box->setFixedHeight(input_frame_box_size);
        ui.input_frame->setFixedHeight(input_frame_size);
        ui.horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
    }

    else if (line_count == 3)
    {
        ui.input->setFixedHeight(input_size + inc);
        ui.input_frame_box->setFixedHeight(input_frame_box_size + inc);
        ui.input_frame->setFixedHeight(input_frame_size + inc);
        ui.horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
    }

    else if (line_count == 4)
    {
        ui.input->setFixedHeight(input_size + 2 * inc);
        ui.input_frame_box->setFixedHeight(input_frame_box_size + 2 * inc);
        ui.input_frame->setFixedHeight(input_frame_size + 2 * inc);
        ui.horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
    }

    emit(unlock());
}

void Window::on_send_button_clicked()
{
    emit(lock());

    if (connection->get_status())
    {
        QString message = ui.input->toPlainText();
        QString new_message;
        bool is_message_correct = false;

        for (int i = 0; i < message.length(); i++)
        {
            qDebug() << i << ":" << message[i];
            //if (message[i] > QChar(31))
            if (message[i].isPrint())
            {
                new_message += message[i];

                //if (message[i] > QChar(32))
                if (!message[i].isSpace())
                    is_message_correct = true;
            }
        }

        if (is_message_correct)
        {
            QByteArray username = user.get_username();
            QByteArray destination = connection->get_destination();

            ui.input->clear();
            ui.input->setFocus();
            ui.send_button->setEnabled(false);

            QByteArray signal = SEND;
            signal.append( ' ' + username + ' ' + destination + ' ' + new_message.toUtf8());

            if (connection->send_message(signal))
            {
                format_output("message", destination, username, new_message);
                update_log_history(username, destination, new_message);
            }
        }
    }
    emit(unlock());
}

void Window::on_receive_message(QByteArray sender, QByteArray destination, QString message)
{
    emit lock();

    if(is_contact_group(destination)){
        if(user.get_username() != sender ){
            if (message.contains(PHOTO))
                format_output("photo", destination, sender, message);
            else
                format_output("message", destination, sender, message);

            update_log_history(sender, destination, message);
        }
    } else {
        if (connection->get_destination() != sender)
            color_notify(sender);

        if (message.contains(PHOTO))
            format_output("photo", sender, sender, message);
        else
            format_output("message", sender, sender, message);

        update_log_history(sender, destination, message);

    }
    /* // Sounds
    if (user.get_username() == name)
        QSound::play("sounds/outgoing.wav");

    if ((this->isMinimized()) && (user.get_username() != name))
        QSound::play("sounds/incoming.wav");
    */


    emit unlock();
}

void Window::on_receive_message_photo(QByteArray sender, QByteArray destination, QByteArray message)
{
    emit lock();

    if(is_contact_group(destination)){
        if (connection->get_destination() != sender)
            color_notify(sender);

        format_output("photo", destination, sender, message);
        update_log_history(sender, destination, message);
    } else {

        format_output("photo", sender, sender, message);
        update_log_history(sender, destination, message);
    }

    emit unlock();
}

void Window::color_notify(QByteArray sender)
{
    for (int i = 0; i < ui.contacts->count(); i++)
    {
        if (ui.contacts->item(i)->text() == sender)
        {
            ui.contacts->item(i)->setBackground(QColor(42, 42, 76));
            ui.contacts->item(i)->setForeground(QColor(255, 255, 255));
        }
    }
}

void Window::format_output(QString mode, QString contact, QByteArray name, QString message)
{
    qDebug() << "format_output: mode=" << mode << ", contact=" << contact << ", name=" << name << ", message=" << message;

    if(!chat_map.contains(contact)){
        if(contact_queue.empty()){
            layout_counter++;
            add_layout(layout_counter);
        }
        chat_map.insert(contact, contact_queue.dequeue());
        save_contact(contact);
        refresh_contacts();
        qDebug() << "fo-00";
    }
    qDebug() << "fo-01";
    QVBoxLayout *current_window = chat_map[contact]->layout;
    qDebug() << "fo-02";
    QString *last_message = chat_map[contact]->last_message;
    qDebug() << "fo-03";
    QLabel *message_label = new QLabel();
    qDebug() << "fo-04";
    QByteArray new_message;
    qDebug() << "fo-05";

    if (mode == "photo")
        message.replace(PHOTO, "");

    if (mode == "message")
    {
        int line_break_count = 0;
        int max_line_length = 30;

        for (int i = 0; i < message.length(); i++)
        {
            //if (message.at(i) > QChar(32))
            if (message.at(i).isPrint())
            {
                new_message.append(QString("%1").arg(message.at(i)).toUtf8());
                line_break_count++;
            }

            //if (message.at(i) == QChar(32))
            if (message.at(i).isSpace())
            {
                //new_message += message.at(i);
                new_message.append(QString("%1").arg(message.at(i)).toUtf8());
                line_break_count = 0;
            }

            if (line_break_count == max_line_length)
            {
                new_message += '\n';
                line_break_count = 0;
            }
        }

        new_message.replace("&", "&amp;");
        new_message.replace("<", "&lt;");
        new_message.replace(">", "&gt;");

        new_message.replace("(smile)", "<img src=:/small_emoticons/Resources/small_emoticons/smile.png>");
        new_message.replace("(wink)", "<img src=:/small_emoticons/Resources/small_emoticons/wink.png>");
        new_message.replace("(laugh)", "<img src=:/small_emoticons/Resources/small_emoticons/laugh.png>");
        new_message.replace("(happy)", "<img src=:/small_emoticons/Resources/small_emoticons/happy.png>");
        new_message.replace("(shy)", "<img src=:/small_emoticons/Resources/small_emoticons/shy.png>");
        new_message.replace("(angry)", "<img src=:/small_emoticons/Resources/small_emoticons/angry.png>");
    }


    if (name == user.get_username())
    {
        message_label->setStyleSheet(user_style);

        QHBoxLayout *layout = new QHBoxLayout;
        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding);

        layout->setSpacing(10);
        message_label->setMargin(10);
        message_label->setWordWrap(true);

        if (mode == "photo")
        {
            QPixmap p(message);

            message_label->setMaximumSize(300, 200);
            int w = message_label->width();
            int h = message_label->height();

            message_label->setPixmap(p.scaled(w, h, Qt::KeepAspectRatio));
        }

        else
        {
            message_label->setMaximumWidth(400);
            message_label->setText(new_message);
        }

        QLabel * bubble = new QLabel();

        if (*last_message != "user")
        {
            bubble->setAlignment(Qt::AlignCenter);
            bubble->setStyleSheet(user_bubble_style);
            //QString bubble_letter(toupper(user.get_username()[0]));
            QString uname = user.get_username();
            QString bubble_letter(uname.at(0));
            bubble->setText(bubble_letter);
        }

        bubble->setFixedSize(40, 40);

        layout->addSpacerItem(spacer);
        layout->addWidget(message_label);
        layout->addWidget(bubble);
        current_window->addLayout(layout);
        current_window->addStretch();
        *last_message = "user";
    }

    else
    {
        message_label->setStyleSheet(contact_style);

        QHBoxLayout *layout = new QHBoxLayout;
        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding);
        QLabel *contact_label = new QLabel();
        contact_label->setText(name);
        contact_label->setStyleSheet(contact_name_style);

        layout->setSpacing(10);
        message_label->setMargin(10);
        message_label->setWordWrap(true);

        if (mode == "photo")
        {
            QPixmap p(message);

            message_label->setMaximumSize(300, 200);
            int w = message_label->width();
            int h = message_label->height();

            message_label->setPixmap(p.scaled(w, h, Qt::KeepAspectRatio));
        }

        else
        {
            message_label->setMaximumWidth(400);
            message_label->setText(new_message);
        }

        QLabel * bubble = new QLabel();

        if (*last_message != "contact")
        {
            bubble->setAlignment(Qt::AlignCenter);
            bubble->setStyleSheet(contact_bubble_style);
            //QString bubble_letter(contact.at(0).toUpper());
            QString bubble_letter(name.first(1).toUpper());
            bubble->setText(bubble_letter);
        }

        bubble->setFixedSize(40, 40);
        layout->addWidget(bubble);

        QVBoxLayout *msg_layout = new QVBoxLayout();
        msg_layout->setSpacing(0);
        msg_layout->addWidget(contact_label);
        msg_layout->addWidget(message_label);
        //layout->addWidget(contact_label);
        //layout->addWidget(message_label);
        layout->addLayout(msg_layout);
        layout->addSpacerItem(spacer);
        current_window->addLayout(layout);
        qDebug() << layout->minimumSize().height();

        *last_message = "contact";
    }
}

void Window::format_output(QString mode, QString contact, QByteArray name, QByteArray photo_ba)
{
    //qDebug() << "format_output: " << message;

    if(!chat_map.contains(contact)){
        if(contact_queue.empty()){
            layout_counter++;
            add_layout(layout_counter);
        }
        chat_map.insert(contact, contact_queue.dequeue());
        save_contact(contact);
        refresh_contacts();
        qDebug() << "fo-00";
    }
    QVBoxLayout *current_window = chat_map[contact]->layout;
    QString *last_message = chat_map[contact]->last_message;
    QLabel *message_label = new QLabel();
    QByteArray new_message;

    /*
    if (mode == "photo")
        message.replace(PHOTO, "");

    if (mode == "message")
    {
        int line_break_count = 0;
        int max_line_length = 30;

        for (int i = 0; i < message.length(); i++)
        {
            //if (message.at(i) > QChar(32))
            if (message.at(i).isPrint())
            {
                new_message.append(QString("%1").arg(message.at(i)).toUtf8());
                line_break_count++;
            }

            //if (message.at(i) == QChar(32))
            if (message.at(i).isSpace())
            {
                //new_message += message.at(i);
                new_message.append(QString("%1").arg(message.at(i)).toUtf8());
                line_break_count = 0;
            }

            if (line_break_count == max_line_length)
            {
                new_message += '\n';
                line_break_count = 0;
            }
        }

        new_message.replace("&", "&amp;");
        new_message.replace("<", "&lt;");
        new_message.replace(">", "&gt;");

        new_message.replace("(smile)", "<img src=:/small_emoticons/Resources/small_emoticons/smile.png>");
        new_message.replace("(wink)", "<img src=:/small_emoticons/Resources/small_emoticons/wink.png>");
        new_message.replace("(laugh)", "<img src=:/small_emoticons/Resources/small_emoticons/laugh.png>");
        new_message.replace("(happy)", "<img src=:/small_emoticons/Resources/small_emoticons/happy.png>");
        new_message.replace("(shy)", "<img src=:/small_emoticons/Resources/small_emoticons/shy.png>");
        new_message.replace("(angry)", "<img src=:/small_emoticons/Resources/small_emoticons/angry.png>");
    }

    */

    if (name == user.get_username())
    {
        message_label->setStyleSheet(user_style);

        QHBoxLayout *layout = new QHBoxLayout;
        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding);

        layout->setSpacing(10);
        message_label->setMargin(10);
        message_label->setWordWrap(true);

        if (mode == "photo")
        {
            QPixmap p;
            p.loadFromData(photo_ba);

            message_label->setMaximumSize(300, 200);
            int w = message_label->width();
            int h = message_label->height();

            message_label->setPixmap(p.scaled(w, h, Qt::KeepAspectRatio));
        }

        else
        {
            message_label->setMaximumWidth(400);
            message_label->setText(new_message);
        }

        QLabel * bubble = new QLabel();

        if (*last_message != "user")
        {
            bubble->setAlignment(Qt::AlignCenter);
            bubble->setStyleSheet(user_bubble_style);
            //QString bubble_letter(toupper(user.get_username()[0]));
            QString uname = user.get_username();
            QString bubble_letter(uname.at(0));
            bubble->setText(bubble_letter);
        }

        bubble->setFixedSize(40, 40);

        layout->addSpacerItem(spacer);
        layout->addWidget(message_label);
        layout->addWidget(bubble);
        current_window->addLayout(layout);
        *last_message = "user";
    }

    else
    {
        message_label->setStyleSheet(contact_style);

        QHBoxLayout *layout = new QHBoxLayout;
        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding);

        layout->setSpacing(10);
        message_label->setMargin(10);
        message_label->setWordWrap(true);

        if (mode == "photo")
        {
            QPixmap p;
            p.loadFromData(photo_ba);

            message_label->setMaximumSize(300, 200);
            int w = message_label->width();
            int h = message_label->height();

            message_label->setPixmap(p.scaled(w, h, Qt::KeepAspectRatio));
        }

        else
        {
            message_label->setMaximumWidth(400);
            message_label->setText(new_message);
        }

        QLabel * bubble = new QLabel();

        if (*last_message != "contact")
        {
            bubble->setAlignment(Qt::AlignCenter);
            bubble->setStyleSheet(contact_bubble_style);
            QString bubble_letter(contact.at(0).toUpper());
            bubble->setText(bubble_letter);
        }

        bubble->setFixedSize(40, 40);
        layout->addWidget(bubble);

        layout->addWidget(message_label);
        layout->addSpacerItem(spacer);
        current_window->addLayout(layout);
        *last_message = "contact";
    }
}
//photo
void Window::on_picture_button_clicked()
{
    emit(lock());

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));
    if( !fileName.isNull() ){
        qDebug() << fileName;
    }

    /*
    if (ui.frame->width() == 0)
    {
        window_width = this->width();
        ui.frame->setMinimumWidth(180);
        this->setMinimumWidth(780);
    }

    else
    {
        ui.frame->setMinimumWidth(0);
        this->setMinimumWidth(600);
        this->resize(window_width, this->height());
    }
    */

    emit(unlock());
}

// emoticons
void Window::on_smile_button_clicked()
{
    emit(lock());

    if (ui.frame->width() == 0)
    {
        window_width = this->width();
        ui.frame->setMinimumWidth(180);
        this->setMinimumWidth(780);
    }

    else
    {
        ui.frame->setMinimumWidth(0);
        this->setMinimumWidth(600);
        this->resize(window_width, this->height());
    }

    emit(unlock());
}

void Window::on_smile_button_2_clicked()
{
    emit(lock());
    add_emoticon("smile");
    emit(unlock());
}

void Window::on_smile_button_3_clicked()
{
    emit(lock());
    add_emoticon("wink");
    emit(unlock());
}

void Window::on_smile_button_4_clicked()
{
    emit(lock());
    add_emoticon("laugh");
    emit(unlock());
}

void Window::on_smile_button_5_clicked()
{
    emit(lock());
    add_emoticon("happy");
    emit(unlock());
}

void Window::on_smile_button_6_clicked()
{
    emit(lock());
    add_emoticon("shy");
    emit(unlock());
}

void Window::on_smile_button_7_clicked()
{
    emit(lock());
    add_emoticon("angry");
    emit(unlock());
}

void Window::add_emoticon(QString emoticon)
{
    ui.input->moveCursor(QTextCursor::End);
    // ui.input->append("<img src=:/small_emoticons/small_emoticons/" + emoticon + ".png>");
    ui.input->insertPlainText(" (" + emoticon + ") ");
    ui.input->moveCursor(QTextCursor::End);
    ui.input->setFocus();
}

// search contact menu
void Window::on_add_button_clicked()
{
    emit(lock());

    ui.add_button->setStyleSheet(button_clicked);
    ui.contact_button->setStyleSheet(button_unclicked);
    ui.add_group_button->setStyleSheet(button_unclicked);
    ui.contact_widget->setCurrentWidget(ui.add_page);
    ui.add_line->clear();
    ui.add_line->setFocus();

    emit(unlock());
}

// search group menu
void Window::on_add_group_button_clicked()
{
    emit(lock());

    ui.add_group_button->setStyleSheet(button_clicked);
    ui.contact_button->setStyleSheet(button_unclicked);
    ui.add_button->setStyleSheet(button_unclicked);
    ui.contact_widget->setCurrentWidget(ui.add_group_page);
    ui.add_line->clear();
    ui.add_line->setFocus();

    emit(unlock());
}

void Window::on_add_line_textChanged(const QString &arg1)
{
    emit(lock());

    if (connection->get_status())
    {
        if (ui.add_line->text().length() != 0)
        {
            QByteArray signal = ADD_CONTACT;

            signal.append( ' ' + user.get_username() + ' ' + ui.add_line->text().toUtf8());
            connection->send_message(signal);
        }

        else
            clear_layout(ui.add_layout);

    }

    emit(unlock());
}

void Window::on_add_group_line_textChanged(const QString &arg1)
{
    emit(lock());

    if (connection->get_status())
    {
        if (ui.add_group_line->text().length() != 0)
        {
            QByteArray signal = ADD_GROUP;

            signal.append( ' ' + user.get_username() + ' ' + ui.add_line->text().toUtf8());
            connection->send_message(signal);
        }

        else
            clear_layout(ui.add_layout);

    }

    emit(unlock());
}

void Window::on_add_contacts(QStringList list)
{
    emit(lock());

    clear_layout(ui.add_layout);
    contacts_found.clear();

    for (int i = 0, count = 0; i < list.size(); i++)
    {
        QByteArray contact = list.at(i).toUtf8();

        if (!user.find_contact(contact) && (contact != user.get_username()))
        {
            QPushButton * button = new QPushButton();

            button->setText(contact);
            button->setStyleSheet(add_button_style);

            contacts_found.push_back(contact);

            if (count == 0)
                connect(button, SIGNAL(clicked()), this, SLOT(on_0_clicked()));

            else if (count == 1)
                connect(button, SIGNAL(clicked()), this, SLOT(on_1_clicked()));

            else if (count == 2)
                connect(button, SIGNAL(clicked()), this, SLOT(on_2_clicked()));

            else if (count == 3)
                connect(button, SIGNAL(clicked()), this, SLOT(on_3_clicked()));

            else if (count == 4)
                connect(button, SIGNAL(clicked()), this, SLOT(on_4_clicked()));

            ui.add_layout->addWidget(button);
            count++;
        }
    }

    emit(unlock());
}

void Window::on_add_groups(QStringList list)
{
    qDebug() << "on_add_groups";

    emit(lock());

    clear_layout(ui.add_group_layout);
    groups_found.clear();

    for (int i = 0, count = 0; i < list.size(); i++)
    {
        QByteArray contact = list.at(i).toUtf8();

        if (!user.find_contact(contact) && (contact != user.get_username()))
        {
            QPushButton * button = new QPushButton();

            button->setText(contact);
            button->setStyleSheet(add_button_style);
            button->setProperty("group_id",QVariant(i));

            groups_found.push_back(contact);

            connect(button, SIGNAL(clicked()), this, SLOT(on_add_group_clicked()));

            ui.add_group_layout->addWidget(button);
            count++;
        }
    }

    emit(unlock());
}

void Window::on_create_group(QString group, bool is_success){
    if(is_success){
        save_group(group);
    } else {
        QMessageBox msg;
        msg.setText(QString("WARNING!\nUnable to create group %1.").arg(group));
        msg.setIcon(QMessageBox::Warning);
        msg.setWindowTitle("Caution");
        msg.exec();
    }
}

void Window::on_add_group_clicked()
{
    emit(lock());
    int id = sender()->property("groupd_id").toInt();
    qDebug() << "add group: " << id;
    save_group(groups_found[id]);
    emit(unlock());
}

void Window::on_0_clicked()
{
    emit(lock());
    save_contact(contacts_found[0]);
    emit(unlock());
}

void Window::on_1_clicked()
{
    emit(lock());
    save_contact(contacts_found[1]);
    emit(unlock());
}

void Window::on_2_clicked()
{
    emit(lock());
    save_contact(contacts_found[2]);
    emit(unlock());
}

void Window::on_3_clicked()
{
    emit(lock());
    save_contact(contacts_found[3]);
    emit(unlock());
}

void Window::on_4_clicked()
{
    emit(lock());
    save_contact(contacts_found[4]);
    emit(unlock());
}

void Window::save_contact(QString contact)
{
    qDebug() << "save_contact[" << contact << "]";
    if (ui.contacts->count() < MAX_CONTACTS)
    {
        QSqlQuery query;

        user.add_contact(contact.toUtf8());
        query.exec("insert into " + user.get_username() + " (username) values ('" + contact + "');");

        refresh_contacts();
        if(contacts_found.indexOf(contact)!=-1)
            contacts_found.removeAt(contacts_found.indexOf(contact));
        on_add_groups(contacts_found);
    }
}

void Window::save_group(QString contact)
{
    qDebug() << "save_group[" << contact << "]";
    if (ui.contacts->count() < MAX_CONTACTS)
    {
        QSqlQuery query;

        user.add_contact(contact.toUtf8(),true);
        query.exec(QString("INSERT INTO %1 (username, type) values ('%2', %3);").arg(user.get_username()).arg(contact).arg(1));
        //query.exec("INSERT INTO " + user.get_username() + " (username, type) values ('" + contact + "');");

        QByteArray signal = JOIN_GROUP;

        signal.append( ' ' + user.get_username() + ' ' + contact.toUtf8());
        connection->send_message(signal);
        connection->waitForReadyRead();

        refresh_contacts();
        if(contacts_found.indexOf(contact)!=-1)
            contacts_found.removeAt(contacts_found.indexOf(contact));
        on_add_contacts(contacts_found);
    }
}

// contact menu
void Window::on_contact_button_clicked()
{
    emit(lock());

    ui.contact_button->setStyleSheet(button_clicked);
    ui.add_button->setStyleSheet(button_unclicked);
    ui.add_group_button->setStyleSheet(button_unclicked);
    ui.contact_widget->setCurrentWidget(ui.view_page);
    ui.input->setFocus();

    emit(unlock());
}

void Window::on_contacts_itemSelectionChanged()
{
    emit(lock());

    if (!delete_mode)
    {
        setAcceptDrops(true);

        //QString selected = ui.contacts->currentItem()->text();
        QString selected = ui.contacts->currentItem()->data(Qt::UserRole).toString();
        qDebug() << "on_contacts_itemSelectionChanged(): contact = " << selected;
        QMap<QString, Contact*>::const_iterator itr = chat_map.find(selected);

        if (itr != chat_map.end())
        {
            ui.chat_windows->setCurrentWidget(chat_map[selected]->page);

            if (ui.input->toPlainText().length() != 0)
                ui.send_button->setEnabled(true);

            if (ui.contacts->currentItem()->background() != QColor(199, 199, 216))
            {
                ui.contacts->currentItem()->setBackground(QColor(199, 199, 216));
                ui.contacts->currentItem()->setForeground(QColor(0, 0, 0));
            }

            connection->set_destination(selected.toUtf8());
        }
    }

    else
    {
        setAcceptDrops(false);
        ui.chat_windows->setCurrentWidget(ui.blank);
        delete_mode = false;
    }

    emit(unlock());
}

void Window::load_log(QString contact)
{
    QMap<QString, Contact*>::const_iterator itr = chat_map.find(contact);

    if (itr == chat_map.end())
    {
        if(contact_queue.empty()){
            layout_counter++;
            add_layout(layout_counter);
        }
        chat_map.insert(contact, contact_queue.dequeue());
        chat_map[contact]->name_label->setText(contact);
        chat_map[contact]->status_label->setText("Offline");

        QSqlQuery query;
        QString username = user.get_username();

        if(user.is_group(contact.toUtf8())){
            chat_map[contact]->status_label->setText("GROUP");
//            query.exec("SELECT COUNT(*) FROM messages WHERE first_participant = '" + contact + "' or second_participant = '" + contact + "';");
            query.exec(QString("SELECT COALESCE(MAX(timestamp),0) FROM messages WHERE first_participant = '%1' OR second_participant = '%1';").arg(contact));
        } else {
//            query.exec("select count(*) from messages where(first_participant = '" + username + "' and second_participant = '" + contact + "')"
//                "or (first_participant = '" + contact + "' and second_participant = '" + username + "');");
            query.exec(QString("SELECT COALESCE(MAX(timestamp),0) FROM messages WHERE(first_participant = '%1' AND second_participant = '%2')"
                "OR (first_participant = '%2' AND second_participant = '%1');").arg(username).arg(contact));
        }

        query.next();

        if (query.value(0).toInt() == 0)
        {
            QByteArray signal = LOG;

            signal.append( ' ' + username.toUtf8() + ' ' + contact.toUtf8());

            if (!connection->send_message(signal))
                on_update_log(contact);
        }

        else
        {
            QByteArray signal = UPDATE_LOG_TS;

            signal.append( ' ' + username.toUtf8() + ' ' + contact.toUtf8() + ' ' + query.value(0).toByteArray());

            if (!connection->send_message(signal))
                on_update_log(contact);
        }
    }
}

bool Window::is_contact_group(QString contact){
    qDebug() << "is_contact_group: " << contact;
    QSqlQuery query;
    query.exec(QString("SELECT type FROM " + user.get_username() + " WHERE username = '%1';").arg(contact));
    if(query.next())
        return query.value(query.record().indexOf("type")).toBool();
    else
        return false;
}

void Window::refresh_contacts()
{
    qDebug() << "refresh_contacts";

    ui.contacts->clear();
    qDebug() << 1;
    user.clear_contacts();
    qDebug() << 2;

    QSqlQuery query;

    query.exec("select * from " + user.get_username() + ";");
    qDebug() << 3;

    int username = query.record().indexOf("username");
    int type_col = query.record().indexOf("type");
    qDebug() << 4;

    while (query.next())
    {
        qDebug() << 5;
        QString contact = query.value(username).toString();
        int type = query.value(type_col).toInt();
        qDebug() << 6;

        QString contact_title;
        if(type==0)
            contact_title = "user " + contact;
        else
            contact_title = "group " + contact;
        QListWidgetItem *contact_label = new QListWidgetItem(contact_title);
        contact_label->setData(Qt::UserRole, QVariant(contact));
        ui.contacts->addItem(contact_label);
        qDebug() << 7;
        user.add_contact(contact.toUtf8(),type == 0 ? false : true);
        qDebug() << 8;
    }
    qDebug() << 9;

    for (int i = 0; i < ui.contacts->count(); i++)
    {
        qDebug() << 10;
        QListWidgetItem *item = ui.contacts->item(i);
        qDebug() << 11;
        item->setSizeHint(QSize(item->sizeHint().width(), 40));
        qDebug() << 12;
    }

    qDebug() << 13;
    log_count = ui.contacts->count();

    qDebug() << 14;
    if (log_count != 0){
        QString contact = ui.contacts->item(--log_count)->data(Qt::UserRole).toString();
        load_log(contact);
    }
    qDebug() << 15;
}

void Window::on_contact_online(QByteArray contact)
{

    if(!is_contact_group(contact)){

        emit lock();

        QMap<QString, Contact*>::const_iterator itr = chat_map.find(contact);

        if (itr != chat_map.end())
            chat_map[contact]->status_label->setText("Online");

        if (log_count != 0)
        {
            QByteArray signal = IS_ONLINE;

            signal.append( ' ' + ui.contacts->item(--log_count)->data(Qt::UserRole).toByteArray() + ' ' + user.get_username());
            connection->send_message(signal);
        } else {
            QByteArray signal = CONNECTED;

            signal += ' ' + user.get_username();
            connection->send_message(signal);
        }

        emit unlock();

    }
}

void Window::on_contact_offline(QByteArray contact)
{
    if(!is_contact_group(contact)) {

        emit lock();

        QMap<QString, Contact*>::const_iterator itr = chat_map.find(contact);

        if (itr != chat_map.end())
            chat_map[contact]->status_label->setText("Offline");

        if (log_count != 0) {
            QByteArray signal = IS_ONLINE;

            signal.append( ' ' + ui.contacts->item(--log_count)->data(Qt::UserRole).toByteArray() + ' ' + user.get_username());
            connection->send_message(signal);
        } else {
            QByteArray signal = CONNECTED;

            signal += ' ' + user.get_username();
            connection->send_message(signal);
        }

        emit unlock();

    }

}

void Window::on_someone_disconnected(QByteArray name)
{
    if(!is_contact_group(name)){

        emit lock();

        QMap<QString, Contact*>::const_iterator itr = chat_map.find(name);

        if (itr != chat_map.end()) {
            if (chat_map[name]->status_label->text() != "Offline")
                chat_map[name]->status_label->setText("Offline");
        }

        emit unlock();

    }
}

void Window::on_someone_connected(QByteArray name)
{
    if(!is_contact_group(name)){

        emit lock();

        QMap<QString, Contact*>::const_iterator itr = chat_map.find(name);

        if (itr != chat_map.end()) {
            if (chat_map[name]->status_label->text() != "Online")
                chat_map[name]->status_label->setText("Online");
        }

        emit unlock();
    }
}

void Window::on_delete_contact()
{
    emit(lock());

    delete_mode = true;
    QSqlQuery query;
    QByteArray marked = ui.contacts->currentItem()->text().toUtf8();

    ui.contacts->currentItem()->setSelected(false);
    user.remove_contact(marked);
    query.exec("delete from " + user.get_username() + " where username ='" + marked + "';");

    clear_layout(chat_map[marked]->layout);
    contact_queue.enqueue(chat_map[marked]);
    chat_map.remove(marked);
    refresh_contacts();

    emit(unlock());
}

void Window::ShowContextMenu(const QPoint &pos)
{
    if (ui.contacts->selectedItems().count() != 0)
    {
        QMenu contextMenu(tr("Context menu"), this);

        QAction action1("Delete", this);
        connect(&action1, SIGNAL(triggered()), this, SLOT(on_delete_contact()));
        contextMenu.addAction(&action1);

        contextMenu.exec(QCursor::pos());
    }
}

// log

void Window::on_create_log_photo(QByteArray contact, QByteArray log)
{
    qDebug() << "on_create_log_photo: contact = " << contact << "; log = " << log.first(100);

    emit lock();

    QSqlQuery query;

    QTextStream stream(log);
    QString username, destination, timestamp;

    stream >> username >> destination >> timestamp;
    //QString message = stream.readAll();
    //message.remove(0, 1);
    query.prepare("INSERT INTO messages (first_participant, second_participant, message, timestamp, raw) VALUES (:fp, :sp, :message, :timestamp, :file)");
    query.bindValue(":fp", username);
    query.bindValue(":sp", destination);
    query.bindValue(":message", PHOTO);
    query.bindValue(":timestamp", timestamp);
    query.bindValue(":file", log.remove(0,username.length()+destination.length()+timestamp.length()+3));
    query.exec();
    //query.exec("insert into messages (first_participant, second_participant, message, timestamp) values ('" + username + "', '" + destination + "', '" + message + "', '" + timestamp + "');");

    if(!photo_request_next()){
        if(is_contact_group(destination)) on_update_log(destination);
        else on_update_log(contact);
    }
    emit unlock();
}

bool Window::photo_request_next(){
    qDebug() << "photo_request_next";
    if(photo_queue.isEmpty()){
        return false;
    } else {
        int id = photo_queue.dequeue();
        QByteArray signal = LOGPHOTO;

        signal.append(' ');
        signal.append(user.get_username());
        signal.append(' ');
        signal.append(QString("%1").arg(id).toUtf8());
        connection->send_message(signal);
        return true;
    }
}

void Window::on_create_log(QByteArray contact, QByteArray log)
{
    qDebug() << "on_create_log: contact = " << contact << "; log = " << log;

    emit(lock());

    QByteArrayList log_part = log.split('\n');
    QSqlQuery query;
    QList<int> photo_id_list;

    for (int i = 0; i < (log_part.size() - 1); i++)
    {
        QTextStream stream(log_part[i]);
        QString username, destination, timestamp;

        stream >> username >> destination >> timestamp;
        QString message = stream.readAll();
        if(message.contains(PHOTO)){

            QStringList photos = message.split(':');
            for(int m = 0; m < (photos.size()-1); m++){
                photo_queue.enqueue(photos[1].toInt());
                photo_contact = username.toUtf8();
            }
        } else {
            message.remove(0, 1);
            query.exec("insert into messages (first_participant, second_participant, message, timestamp) values ('" + username + "', '" + destination + "', '" + message + "', '" + timestamp + "');");
        }
    }
    if(!photo_request_next()){
        on_update_log(contact);
    }
    emit(unlock());
}

void Window::on_create_log_finish(QByteArray contact) {
    qDebug() << "on_create_log_finish: contact = " << contact;
    on_update_log(contact);
    emit(unlock());
}

void Window::on_update_log(QString contact)
{
    qDebug() << "on_update_log";
    emit(lock());

    qDebug() << contact;
    QString sqlQuery;
    QByteArray username = user.get_username();
    if(chat_map.contains(contact)){
        if(is_contact_group(contact)){
            sqlQuery = QString("SELECT id, first_participant, second_participant,"
                               " message, timestamp FROM messages WHERE first_participant = '%1' "
                               "OR second_participant = '%1' ORDER BY timestamp;").arg(contact);
        } else {
            sqlQuery = "select id, first_participant, second_participant,"
                       " message, timestamp from messages where (first_participant = '" + username + "' "
                       "and second_participant = '" + contact + "') or "
                       "(first_participant = '" + contact + "' and second_participant = '"
                       + username + "') order by timestamp;";
        }
    QVBoxLayout *current_window = chat_map[contact]->layout;

    clear_layout(current_window);

    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    current_window->addSpacerItem(spacer);

    QSqlQuery query;
    query.exec(sqlQuery);

    int id = query.record().indexOf("id");
    int first = query.record().indexOf("first_participant");
    int message = query.record().indexOf("message");

    while (query.next())
    {
        qDebug() << query.value(message).toString();
        if (query.value(message).toString().contains(PHOTO)){
            qDebug() << "PHOTO";
            QSqlQuery rawQuery;
            rawQuery.exec(QString("select raw from messages where id=%1;").arg(query.value(id).toInt()));
            qDebug() << rawQuery.lastError().text();
            rawQuery.next();
            format_output("photo", contact, query.value(first).toByteArray(), rawQuery.value(0).toByteArray());
        } else
            format_output("message", contact, query.value(first).toByteArray(), query.value(message).toString());
    }

    ui.input->setFocus();

    if (log_count != 0)
        load_log(ui.contacts->item(--log_count)->data(Qt::UserRole).toString());

    else
    {
        log_count = ui.contacts->count();

        if (log_count != 0)
        {
            QByteArray signal = IS_ONLINE;

            signal.append( ' ' + ui.contacts->item(--log_count)->data(Qt::UserRole).toByteArray() + ' ' + username);
            connection->send_message(signal);
        }
    }
    }

    emit unlock();
}

void Window::update_log_history(QByteArray username, QByteArray destination, QString message)
{
    qDebug() << "update_log_history: un = " << username << "; mess = " << message;
    QSqlQuery query;
    int timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();

    query.exec("insert into messages (first_participant, second_participant, message, timestamp) values ('" + username + "', '" + destination + "', '" + message + "', " + QByteArray::number(timestamp) + ");");
}

void Window::update_log_history(QByteArray username, QByteArray destination, QByteArray buffer)
{
    qDebug() << "update_log_history{P}: un = " << username;
    //QSqlQuery query;
    int timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();

    //query.exec("insert into messages (first_participant, second_participant, message, timestamp) values ('" + username + "', '" + destination + "', '" + buffer + "', " + QByteArray::number(timestamp) + ");");

    QSqlQuery query;
    query.prepare("INSERT INTO messages (first_participant, second_participant, message, timestamp, raw) VALUES (:fp, :sp, :message, :timestamp, :file)");
    query.bindValue(":fp", QString(username));
    query.bindValue(":sp", QString(destination));
    query.bindValue(":message", PHOTO);
    query.bindValue(":timestamp", timestamp);
    query.bindValue(":file", buffer);

    if (!query.exec()) {
        qWarning()<< Q_FUNC_INFO << query.lastError();
    }
}

// other
void Window::keyPressEvent(QKeyEvent * e)
{
    if ((e->key() == Qt::Key_Enter) || (e->key() == Qt::Key_Return))
    {

        if (ui.login->isVisible())
            on_login_button_clicked();

        else if (ui.signup->isVisible())
            on_signup_button_clicked();
    }

    else if ((e->key() == Qt::Key_Escape) || (e->key() == Qt::Key_Backspace))
    {
        if (ui.signup->isVisible())
            on_signup_login_clicked();
    }
}

void Window::auto_scroll(int min, int max)
{
    Q_UNUSED(min);

    QScrollBar* scrollbar = qobject_cast<QScrollBar*>(sender());
    qDebug() << scrollbar;
    if(scrollbar != NULL){
        scrollbar->setValue(max);
        //if (ui.chat_windows->currentIndex() == 0)
    }
}

void Window::on_actionLog_out_triggered()
{
    QProcess::startDetached(QApplication::applicationFilePath());
    exit(12);
}

void Window::clear_layout(QLayout* layout)
{

    //foreach (QWidget *item, layout->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively)) {
    //    qDebug() << item;
    //}
    //if (layout != NULL){
    //    qDeleteAll(layout->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively));
    //    qDeleteAll(layout->findChildren<QLayout *>(QString(), Qt::FindChildrenRecursively));
    //    qDeleteAll(layout->findChildren<QPushButton *>(QString(), Qt::FindChildrenRecursively));
    //}
    if (layout == NULL)
        return;
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clear_layout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
           delete item->widget();
        }
        if(QSpacerItem *SpacerItem = item->spacerItem()) delete SpacerItem;
        delete item;
    }
    /*
    {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr)
        {
            if (item->widget())
                delete item->widget();

            if (item->spacerItem())
                delete item->spacerItem();

            if (item->layout())
            {
                clear_layout(item->layout());
                delete item->layout();
            }

        }
    }
    */
}

int Window::picture_get_length(QPixmap pixmap){
    QByteArray bArray;
    QBuffer buffer(&bArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "JPEG");
    return bArray.length();
}

void Window::on_file_button_clicked()
{
    emit(lock());

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));
    if( !fileName.isNull() ){
        qDebug() << fileName;
        QPixmap pixmap = QPixmap(fileName);

        int length = picture_get_length(pixmap);
        int percent = 1;

        QPixmap new_pixmap(pixmap);
        while(length > 65000 && percent < 100) {
            QSize size = pixmap.size();
            new_pixmap = pixmap.scaled(size.width() - size.width() / 100 * percent, size.height() - size.height() / 100 * percent, Qt::KeepAspectRatio);
            percent++;
            length = picture_get_length(new_pixmap);
        }

        QByteArray bArray;
        QBuffer buffer(&bArray);
        buffer.open(QIODevice::WriteOnly);
        new_pixmap.save(&buffer, "JPEG");

        qDebug() << "pic length: " << bArray.length() << "; scale %: " << percent;


        QByteArray username = user.get_username();
        QByteArray destination = connection->get_destination();

        ui.input->clear();
        ui.input->setFocus();
        ui.send_button->setEnabled(false);

        QByteArray signal = SENDPHOTO;
        signal.append( ' ' + username + ' ' + destination + ' ' );
        signal.append(bArray);

        if (connection->send_message(signal))
        {
            format_output("photo", destination, username, bArray);
            QByteArray ph;
            ph.append(bArray);
            update_log_history(username, destination, ph);
        }


    }


    emit(unlock());

}


void Window::on_actionCreate_group_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Input new group name"),
                                             tr("Name:"), QLineEdit::Normal,
                                             "", &ok);
    if(ok){
        QByteArray signal = CREATE_GROUP;
        QByteArray username = user.get_username();
        signal.append( ' ' + username + ' ' + text.toUtf8());

        connection->send_message(signal);
    }

}

