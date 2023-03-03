#include "Room.hpp"

Room::Room(/* args */)
{
}

Room::~Room()
{
    for (auto &&i : member_list)
    {
        i->set_room(nullptr);
    }
}

void Room::join_client(Client *client)
{
    member_list.push_back(client);
}

void Room::exit_client(Client *client)
{
    member_list.erase(remove(member_list.begin(), member_list.end(), client), member_list.end());
}

std::vector<Client *> Room::get_client_list()
{
    return member_list;
}
