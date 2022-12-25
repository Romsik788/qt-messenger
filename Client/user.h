#ifndef USER_H
#define USER_H

#include <QByteArray>
#include <QVector>

class User
{
public:
    User();

private:

    struct TGContact{
        QByteArray title;
        bool is_group;
    };

    QByteArray username, user_id, password;
    QVector<TGContact> current_contacts;

public:
    void add_contact(QByteArray, bool is_group = false);
    void remove_contact(QByteArray);
    bool find_contact(QByteArray);
    bool is_group(QByteArray);
    void clear_contacts();
    void set_user_id(QByteArray);
    void set_username(QByteArray);
    void set_password(QByteArray);
    QByteArray get_username();
    QByteArray get_user_id();
    QByteArray get_password();
};

#endif

