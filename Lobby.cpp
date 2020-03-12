#include "Lobby.hpp"

int Lobby::lobbyID = 0;

Lobby::Lobby(int lobbyOwner) {
    this->id = lobbyID++;
    this->lobbyOwner = lobbyOwner;
    this->status = "inactive";
}

int Lobby::getCurrentPlayers() 
{
    return this->currentPlayers;
}

string Lobby::getStatus()
{
    return this->status;
}


void Lobby::setStatus(string status)
{
    this->status = status;
}


std::vector<Client*> Lobby::getClientList()
{
    return this->clientList;
}


int Lobby::getLobbyOwner()
{
    return this->lobbyOwner;
}

void Lobby::setLobbyOwner(int lobbyOwner)
{
    this->lobbyOwner = lobbyOwner;
}

int Lobby::getId()
{
    return this->id;
}

void Lobby::setId(int id)
{
    this->id = id;
}

void Lobby::addClient(Client * client) {
    clientList.push_back(client);
    client->setLobby_Id(lobbyID);
    if (currentPlayers == 0) {
        this->lobbyOwner = client->getPlayer_Id();
    }
    this->currentPlayers++;
}

void Lobby::removeClient(Client *client)
{
    for (auto it = clientList.begin(); it != clientList.end(); it++) {
        if ((*it)->getPlayer_Id() == client->getPlayer_Id()) {
            clientList.erase(it);
            break;
        }
    }
    client->setCharacterClass("default");
    client->setLobby_Id(-1);
    client->setTeam(0);
    currentPlayers--;
    if (currentPlayers == 0) {
        this->setLobbyOwner(-1);
    }
}

void Lobby::removeAllClients()
{
    auto iter = clientList.begin();
    for ( ; iter != clientList.end(); iter++)
        (*iter)->setLobby_Id(-1);
    clientList.clear();
    this->currentPlayers = 0;
}