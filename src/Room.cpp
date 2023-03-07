#include "Room.hpp"
#include "./Server.hpp"
#include "DFError.hpp"

Room::Room(RoomInfo roominfo, Server *ser)
{
    server = ser;
    info = roominfo;
}

Room::~Room()
{
    for (auto &&i : member_list)
    {
        i->set_room(nullptr, "");
    }
}

void Room::join_client(Client *client, std::string pw)
{
    room_mutex.lock();
    if (info.is_private)
    {
        if (pw != info.pw)
        {
            room_mutex.unlock();
            throw DFError(ERR_ROOM_PW);
        }
    }

    if (member_list.size() >= info.max_player)
    {
        room_mutex.unlock();
        throw DFError(ERR_ROOM_FULL);
    }

    member_list.push_back(client);
    room_mutex.unlock();
}

void Room::exit_client(Client *client)
{
    room_mutex.lock();
    member_list.erase(remove(member_list.begin(), member_list.end(), client), member_list.end());

    if (client == info.host)
    {
        Json::Value packet;
        packet["msg"] = info.host->get_nickname();
        if (member_list.size() != 0)
        {
            info.host = member_list.front();
            server->send_packet(info.host->get_ssl(), 0, TcpPacketType::RoomChangeHost, packet);
        }
        else
        {
            server->delete_room(this);
        }
    }
    room_mutex.unlock();
}

void Room::set_room_info(RoomInfo roominfo)
{
    room_mutex.lock();
    if (member_list.size() > roominfo.max_player)
    {
        room_mutex.unlock();
        throw DFError(ERR_ROOM_INFO_FULL);
    }
    room_mutex.unlock();
}

RoomInfo Room::get_room_info()
{
    return RoomInfo();
}

int Room::get_member_count()
{
    return 0;
}

std::vector<Client *> Room::get_client_list()
{
    return member_list;
}

void Room::send_packet_all(int index, TcpPacketType type, Json::Value msg)
{
}
