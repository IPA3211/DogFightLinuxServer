#pragma once

#include <iostream>
#include <string>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <mysql_error.h>

#include "./json/json.h"
#include "./Client.hpp"

using namespace std;

enum Table
{
    User
};

enum UserTableColumn
{
    IDpk,
    UserId,
    PassHash,
    NickName,
    Email,
};

class MySqlManager
{
private:
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con;

    sql::PreparedStatement *statement_id_duplication;
    sql::PreparedStatement *statement_nick_duplication;
    sql::PreparedStatement *statement_email_duplication;
    sql::PreparedStatement *statement_sign_in;
    sql::PreparedStatement *statement_sign_up;
    sql::PreparedStatement *statement_get_user_info;

public:
    static MySqlManager &instance;
    MySqlManager(/* args */);
    ~MySqlManager();
    sql::ResultSet *send_query(std::string qurry);

    Json::Value check_duplication(int column, string check);
    Json::Value signup_user(string id, string pass, string nick, string email);
    Json::Value signin_user(string id, string pass, Client **client);
    Json::Value get_user_info(Client *client);
};