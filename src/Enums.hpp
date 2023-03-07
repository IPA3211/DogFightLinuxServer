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
