#include "DiscoveryManager.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

DiscoveryManager::DiscoveryManager() : m_isBroadcasting(false), m_isListening(false) {}

DiscoveryManager::~DiscoveryManager() {
    stopBroadcasting();
    stopListening();
}

DiscoveryManager& DiscoveryManager::get() {
    static DiscoveryManager instance;
    return instance;
}

void DiscoveryManager::startBroadcasting(const std::string& username, uint16_t port) {
    if (m_isBroadcasting) return;
    m_hostUsername = username;
    m_hostPort = port;
    m_isBroadcasting = true;
    m_broadcastThread = std::thread(&DiscoveryManager::broadcastLoop, this);
}

void DiscoveryManager::stopBroadcasting() {
    m_isBroadcasting = false;
    if (m_broadcastThread.joinable()) {
        m_broadcastThread.join();
    }
}

void DiscoveryManager::startListening() {
    if (m_isListening) return;
    m_isListening = true;
    m_listenThread = std::thread(&DiscoveryManager::listenLoop, this);
}

void DiscoveryManager::stopListening() {
    m_isListening = false;
    if (m_listenThread.joinable()) {
        m_listenThread.join();
    }
}

void DiscoveryManager::broadcastLoop() {
    ENetSocket socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if (socket == ENET_SOCKET_NULL) return;

    enet_socket_set_option(socket, ENET_SOCKOPT_BROADCAST, 1);

    ENetAddress address;
    address.host = ENET_HOST_BROADCAST;
    address.port = 7778;

    while (m_isBroadcasting) {
        std::string message = "DE2_DISCOVERY:" + m_hostUsername + ":" + std::to_string(m_hostPort);
        ENetBuffer buffer;
        buffer.data = (void*)message.c_str();
        buffer.dataLength = message.length();

        enet_socket_send(socket, &address, &buffer, 1);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    enet_socket_destroy(socket);
}

void DiscoveryManager::listenLoop() {
    ENetSocket socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if (socket == ENET_SOCKET_NULL) return;

    enet_socket_set_option(socket, ENET_SOCKOPT_NONBLOCK, 1);
    enet_socket_set_option(socket, ENET_SOCKOPT_REUSEADDR, 1);

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 7778;

    if (enet_socket_bind(socket, &address) < 0) {
        enet_socket_destroy(socket);
        return;
    }

    while (m_isListening) {
        ENetAddress senderAddress;
        char bufferData[256];
        ENetBuffer buffer;
        buffer.data = bufferData;
        buffer.dataLength = sizeof(bufferData);

        int receivedSize = enet_socket_receive(socket, &senderAddress, &buffer, 1);
        if (receivedSize > 0) {
            std::string data(bufferData, receivedSize);
            if (data.starts_with("DE2_DISCOVERY:")) {
                auto parts = geode::utils::string::split(data, ":");
                if (parts.size() >= 3) {
                    char ip[64];
                    enet_address_get_host_ip(&senderAddress, ip, sizeof(ip));
                    
                    std::lock_guard<std::mutex> lock(m_sessionsMutex);
                    LocalSession& session = m_availableSessions[ip];
                    session.username = parts[1];
                    session.port = (uint16_t)std::stoi(parts[2]);
                    session.ip = ip;
                    session.lastSeen = (uint32_t)time(nullptr);
                }
            }
        }

        // cleanup old sessions
        {
            std::lock_guard<std::mutex> lock(m_sessionsMutex);
            uint32_t now = (uint32_t)time(nullptr);
            for (auto it = m_availableSessions.begin(); it != m_availableSessions.end(); ) {
                if (now - it->second.lastSeen > 10) {
                    it = m_availableSessions.erase(it);
                } else {
                    ++it;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    enet_socket_destroy(socket);
}

std::vector<LocalSession> DiscoveryManager::getAvailableSessions() {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    std::vector<LocalSession> sessions;
    for (auto const& [ip, session] : m_availableSessions) {
        sessions.push_back(session);
    }
    return sessions;
}

void DiscoveryManager::clearSessions() {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    m_availableSessions.clear();
}

std::string DiscoveryManager::getLocalIP() {
    ENetSocket socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if (socket == ENET_SOCKET_NULL) return "127.0.0.1";

    ENetAddress targetAddress;
    enet_address_set_host(&targetAddress, "8.8.8.8");
    targetAddress.port = 53;

    if (enet_socket_connect(socket, &targetAddress) < 0) {
        enet_socket_destroy(socket);
        return "127.0.0.1";
    }

    ENetAddress localAddress;
    if (enet_socket_get_address(socket, &localAddress) < 0) {
        enet_socket_destroy(socket);
        return "127.0.0.1";
    }

    char ip[64];
    enet_address_get_host_ip(&localAddress, ip, sizeof(ip));
    enet_socket_destroy(socket);
    
    return std::string(ip);
}
