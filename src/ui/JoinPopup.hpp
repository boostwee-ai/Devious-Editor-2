#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class JoinPopup : public geode::Popup {
    protected:
        TextInput* m_ipInput;
        
        bool init();
        void OnJoin(CCObject*);
    public:
        static JoinPopup* create();
};
