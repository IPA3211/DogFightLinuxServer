#pragma once

enum TcpPacketType{
    Answer = 0,
    Msg,
    Chat,
    SecretChat,

    DuplicationCheck = 100,

    SignUp = 200,
    SignIn,
    GetUserInfo,

    RoomCreate = 300,
    RoomChangeHost,
    RoomChangeInfo,
    RoomJoin,
    RoomExit,
    GetRoomList,
    GetRoomInfo,

    Quit = 999
};

enum DFErrorType
{
    ERR_GENERIC,
    ERR_JSON_PARSE,

    ERR_ROOM_PW = 100,
    ERR_ROOM_FULL,
    ERR_ROOM_INFO_FULL,

    ERR_SQL_QUERY = 200,
};
