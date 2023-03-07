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
            throw DFError((char *)"pw incorrect");
        }
    }

    if (member_list.size() >= info.max_player)
    {
        throw DFError((char *)"room is full");
    }

    member_list.push_back(client);
}

void Room::exit_client(Client *client)
{
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
    }
}

std::vector<Client *> Room::get_client_list()
{
    return member_list;
}