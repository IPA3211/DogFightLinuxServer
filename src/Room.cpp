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
    if (info.is_private)
    {
        if (pw != info.pw)
        {
            room_mutex.unlock();
            throw DFError(ERR_ROOM_PW);
        }
    }

    room_mutex.lock();
    if ((int)member_list.size() >= info.max_player)
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
            send_packet_all(0, TcpPacketType::RoomChangeHost, packet);
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
    if ((int)member_list.size() > roominfo.max_player)
    {
        room_mutex.unlock();
        throw DFError(ERR_ROOM_INFO_FULL);
    }
    room_mutex.unlock();
}

RoomInfo Room::get_room_info()
{
    return info;
}

int Room::get_member_count()
{
    return member_list.size();
}

Json::Value Room::send_packet_all(int index, TcpPacketType type, Json::Value msg)
{
    Json::Value ans_packet;
    try
    {
        for (auto &&c : member_list)
        {
            server->send_packet(c->get_ssl(), 0, TcpPacketType::Chat, msg);
        }
        ans_packet["result"] = 1;
        ans_packet["msg"] = "success send to all";
    }
    catch (const std::exception &e)
    {
        ans_packet["result"] = -1;
        ans_packet["msg"] = e.what();
    }
    return ans_packet;
}
