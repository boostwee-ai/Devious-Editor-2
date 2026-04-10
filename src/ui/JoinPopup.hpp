#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class JoinPopup : public geode::Popup {
    protected:
        TextInput* m_ipInput;
        CCMenu* m_sessionMenu;
        
        bool init();
        void OnJoin(CCObject*);
        void onUpdateSessions(float dt);
        void onSelectSession(CCObject*);
        void onClose(CCObject*) override;
    public:
        static JoinPopup* create();
};
