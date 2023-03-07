#pragma once

#include <iostream>
#include "./Enums.hpp"

class DFError
{
public:
    DFErrorType type;
    std::string Label;

    DFError(DFErrorType t)
    {
        type = t;
        switch (type)
        {
        case ERR_JSON_PARSE:
            Label = (char *)"json parse error";
            break;
        case ERR_ROOM_PW:
            Label = (char *)"room pw incorrect";
            break;
        case ERR_ROOM_FULL:
            Label = (char *)"room is full";
            break;
        case ERR_ROOM_INFO_FULL:
            Label = (char *)"client amount > room max_clinet to change";
            break;
        case ERR_SQL_QUERY :
            Label = (char *)"Query Error";
            break;
        default:
            Label = (char *)"Generic Error";
            break;
        }
    }
    ~DFError() {}
    inline const Json::Value get_packet(void)
    {
        Json::Value packet;
        packet["type"] = type;
        packet["msg"] = Label;
        return packet;
    }
    inline const DFErrorType get_type(void) { return type; }
    inline const char *get_message(void) { return Label.c_str(); }
};