#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <Geode/Geode.hpp>
#include "../../libs/enet/include/enet.h"

struct LocalSession {
    std::string username;
    std::string ip;
    uint16_t port;
    uint32_t lastSeen;
};

class DiscoveryManager {
public:
    static DiscoveryManager& get();

    // Host side
    void startBroadcasting(const std::string& username, uint16_t port);
    void stopBroadcasting();

    // Client side
    void startListening();
    void stopListening();
    
    std::vector<LocalSession> getAvailableSessions();
    void clearSessions();

    static std::string getLocalIP();

private:
    DiscoveryManager();
    ~DiscoveryManager();

    std::atomic<bool> m_isBroadcasting;
    std::atomic<bool> m_isListening;
    
    std::string m_hostUsername;
    uint16_t m_hostPort;
    
    std::thread m_broadcastThread;
    std::thread m_listenThread;
    
    std::mutex m_sessionsMutex;
    std::map<std::string, LocalSession> m_availableSessions; // IP -> Session

    void broadcastLoop();
    void listenLoop();
};
