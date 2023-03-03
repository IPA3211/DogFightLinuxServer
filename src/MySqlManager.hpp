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

public:
    static MySqlManager &instance;
    MySqlManager(/* args */);
    sql::ResultSet *SendQuery(std::string qurry);

    Json::Value CheckDuplication(int column, string check);
    Json::Value SignUpUser(string id, string pass, string nick, string email);
    Json::Value SignInUser(string id, string pass, Client **client);

    ~MySqlManager();
};