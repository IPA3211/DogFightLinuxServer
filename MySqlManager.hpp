#include <iostream>
#include <string>
#include "/usr/include/mysql/mysql.h"

class FFError
{
public:
    std::string Label;

    FFError() { Label = (char *)"Generic Error"; }
    FFError(char *message) { Label = message; }
    ~FFError() {}
    inline const char *GetMessage(void) { return Label.c_str(); }
};

class MySqlManager
{
private:
    MYSQL *MySQLConnection = NULL;

public:
    static MySqlManager &instance;
    MySqlManager(/* args */);
    MYSQL_RES *SendQuery(std::string qurry);
    ~MySqlManager();
};

MySqlManager::MySqlManager(/* args */)
{
    std::string hostName = "localhost";
    std::string userId = "dogfight_user";
    std::string password = "980706";
    std::string DB = "dogfight";

    MySQLConnection = mysql_init(NULL);

    try
    {
        MYSQL *MySQLConRet = mysql_real_connect(MySQLConnection,
                                                hostName.c_str(),
                                                userId.c_str(),
                                                password.c_str(),
                                                DB.c_str(),
                                                0,
                                                NULL,
                                                0);

        if (MySQLConRet == NULL)
        {
            throw FFError((char *)mysql_error(MySQLConnection));
        }
    }
    catch (FFError e)
    {
        printf("%s\n", e.Label.c_str());
        throw e;
    }
}

MYSQL_RES *MySqlManager::SendQuery(std::string qurry)
{
    MYSQL_RES *mysqlResult;
    int mysqlStatus = mysql_query(MySQLConnection, qurry.c_str());

    if (mysqlStatus)
    {
        throw FFError((char *)mysql_error(MySQLConnection));
    }
    else
    {
        mysqlResult = mysql_store_result(MySQLConnection); // Get the Result Set
    }

    return mysqlResult;
}

MySqlManager::~MySqlManager()
{
    mysql_close(MySQLConnection);
}
