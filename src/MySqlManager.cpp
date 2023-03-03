#include "./MySqlManager.hpp"

MySqlManager::MySqlManager()
{
    try
    {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "dogfight_user", "980706");

        statement_id_duplication =
            con->prepareStatement("SELECT count(*) FROM dogfight.user WHERE UserId = ?;");
        statement_nick_duplication =
            con->prepareStatement("SELECT count(*) FROM dogfight.user WHERE NickName = ?;");
        statement_email_duplication =
            con->prepareStatement("SELECT count(*) FROM dogfight.user WHERE Email = ?;");
        statement_sign_up =
            con->prepareStatement("INSERT INTO dogfight.user(userId, passHash, nickName, email) VALUES(?, ?, ?, ?)");
        statement_sign_in =
            con->prepareStatement("SELECT * FROM dogfight.user WHERE UserId= ? and passHash= ?;");
    }
    catch (sql::SQLException &e)
    {
        printf("%s\n", e.what());
        throw e;
    }
}

MySqlManager::~MySqlManager()
{
    delete driver;
    delete con;
}

sql::ResultSet *MySqlManager::SendQuery(std::string qurry)
{
    sql::Statement *stmt;
    sql::ResultSet *res;

    stmt = con->createStatement();
    res = stmt->executeQuery(qurry);

    delete stmt;
    return res;
}

Json::Value MySqlManager::CheckDuplication(int column, string check)
{
    Json::Value ans_value;

    int temp_ans = 0;
    string msg = "";

    try
    {
        sql::ResultSet *ans;
        switch (column)
        {
            case UserTableColumn::UserId : 
                statement_id_duplication->setString(1, check);
                ans = statement_id_duplication->executeQuery();
            break;
            case UserTableColumn::NickName : 
                statement_nick_duplication->setString(1, check);
                ans = statement_id_duplication->executeQuery();
            break;
            case UserTableColumn::Email : 
                statement_email_duplication->setString(1, check);
                ans = statement_id_duplication->executeQuery();
            break;
        }

        ans->next();

        if (ans->getInt("count(*)") == 0)
        {
            temp_ans = 1; // true
            msg = "true";
        }
        else
        {
            temp_ans = 0; // false
            msg = "false";
        }

        delete ans;
    }
    catch (sql::SQLException &e)
    {
        temp_ans = -1; // error
        msg = e.what();
    }

    ans_value["result"] = temp_ans;
    ans_value["msg"] = msg;

    return ans_value;
}

Json::Value MySqlManager::SignUpUser(string id, string pass, string nick, string email)
{
    Json::Value ans_value;
    int temp_ans = 0;
    string msg = "";
    try
    {
        statement_sign_up->setString(1, id);
        statement_sign_up->setString(2, pass);
        statement_sign_up->setString(3, nick);
        statement_sign_up->setString(4, email);
        statement_sign_up->executeUpdate();

        temp_ans = 0;
        msg = "Sign Up success";
    }
    catch (sql::SQLException &e)
    {
        temp_ans = -1; // error
        msg = e.what();
    }

    ans_value["result"] = temp_ans;
    ans_value["msg"] = msg;

    return ans_value;
}

Json::Value MySqlManager::SignInUser(string id, string pass, Client **client)
{
    Json::Value ans_value;
    int temp_ans = 0;
    string msg = "";
    try
    {
        statement_sign_in->setString(1, id);
        statement_sign_in->setString(2, pass);
        auto ans = statement_sign_in->executeQuery();

        
        if (ans->rowsCount() != 0)
        {
            temp_ans = 1; // true
            ans->next();
            *(client) = new Client(ans->getInt("idpk"), ans->getString("nickname"));
            
            msg = "Sign in success";
        }
        else
        {
            temp_ans = 0; // false
            msg = "Sign in fail";
        }
        delete ans;
    }
    catch (sql::SQLException &e)
    {
        temp_ans = -1; // error
        msg = e.what();
    }

    ans_value["result"] = temp_ans;
    ans_value["msg"] = msg;

    return ans_value;
}