#include <Geode/Geode.hpp>

#include "../sync/SyncManager.hpp"
#include "JoinPopup.hpp"
#include "../network/NetworkManager.hpp"
#include "../network/DiscoveryManager.hpp"

extern NetworkManager* g_network;
extern SyncManager* g_sync;

extern bool g_isHost;
extern bool g_isInSession;

JoinPopup* JoinPopup::create(){
    auto ret = new JoinPopup();
    if (ret && ret->init()){
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool JoinPopup::init(){
    if (!geode::Popup::init(320.f, 280.f)) return false;

    this->setTitle("Join Session");

    auto winSize = this->m_mainLayer->getContentSize();

    // IP input
    auto ipLabel = CCLabelBMFont::create("Host IP:","bigFont.fnt");
    ipLabel->setScale(0.5f);
    ipLabel->setPosition(ccp(
        winSize.width/2,
        winSize.height/2 + 30
    ));
    this->m_mainLayer->addChild(ipLabel);

    m_ipInput = TextInput::create(200.0f, "127.0.0.1", "chatFont.fnt");
    m_ipInput->setFilter("1234567890.:");
    m_ipInput->setPosition(ccp(
        winSize.width/2,
        winSize.height/2
    ));
    this->m_mainLayer->addChild(m_ipInput);

    // Join button
    auto joinBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Join","goldFont.fnt","GJ_button_01.png",0.8f),
        this,
        menu_selector(JoinPopup::OnJoin)
    );
    joinBtn->setPosition(ccp(
        winSize.width/2,
        winSize.height/2 -40
    ));

    auto menu = CCMenu::create();
    menu->addChild(joinBtn);
    menu->setPosition(0,0);
    this->m_mainLayer->addChild(menu);

    // discovery UI
    auto discLabel = CCLabelBMFont::create("Local Sessions:", "goldFont.fnt");
    discLabel->setScale(0.5f);
    discLabel->setPosition(ccp(winSize.width / 2, winSize.height / 2 - 70));
    this->m_mainLayer->addChild(discLabel);

    m_sessionMenu = CCMenu::create();
    m_sessionMenu->setPosition(ccp(winSize.width / 2, winSize.height / 2 - 100));
    this->m_mainLayer->addChild(m_sessionMenu);

    // start listening
    DiscoveryManager::get().startListening();
    this->schedule(schedule_selector(JoinPopup::onUpdateSessions), 1.0f);

    return true;
}

void JoinPopup::onUpdateSessions(float dt) {
    m_sessionMenu->removeAllChildren();

    auto sessions = DiscoveryManager::get().getAvailableSessions();
    float yOffset = 0;

    if (sessions.empty()) {
        auto label = CCLabelBMFont::create("Scanning...", "chatFont.fnt");
        label->setScale(0.5f);
        label->setOpacity(100);
        m_sessionMenu->addChild(label);
    } else {
        for (const auto& session : sessions) {
            auto fillBtn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create(fmt::format("{} ({})", session.username, session.ip).c_str(), "chatFont.fnt", "GJ_button_04.png", 0.5f),
                this,
                menu_selector(JoinPopup::onSelectSession)
            );

            fillBtn->setID(fmt::format("{}:{}", session.ip, session.port));
            fillBtn->setPositionY(yOffset);
            m_sessionMenu->addChild(fillBtn);
            yOffset -= 25.0f;
        }
    }
}

void JoinPopup::onSelectSession(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    m_ipInput->setString(btn->getID().c_str());
}

void JoinPopup::onClose(CCObject* sender) {
    DiscoveryManager::get().stopListening();
    Popup::onClose(sender);
}

void JoinPopup::OnJoin(CCObject*){
    std::string ip = m_ipInput->getString();

    if (ip.empty()){
        FLAlertLayer::create("Error","Please enter a valid IP Adress!!", "OK")->show();
        return;
    }

    log::info("Attempting to join: {}", ip);

    // parse ip and port
    std::string address = ip;
    uint16_t port = 7777;

    size_t colonPos = ip.find(':');
    if (colonPos != std::string::npos){
        address = ip.substr(0, colonPos);
        try {
            port = (uint16_t)std::stoi(ip.substr(colonPos + 1));
        } catch (...) {
            port = 7777;
        }
    }

    // connect to address
    if (g_network->connect(address, port)){
        g_isHost = false;
        g_isInSession = true;

        g_sync->setUserID(g_network->getPeerID());

        this->onClose(nullptr);

        // go to editor
        auto level = GJGameLevel::create();
        level->m_levelName = "Collab Session";

        auto scene = CCScene::create();
        auto editorLayer = LevelEditorLayer::create(level, false);
        scene->addChild(editorLayer);

        CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f,scene));
        
    } else {
        FLAlertLayer::create("Connection Failed", "Couldn't connect to host!", "OK")->show();
    }
}