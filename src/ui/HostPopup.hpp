#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class HostPopup : public geode::Popup<>{
    protected:
        CCLabelBMFont* m_ipLabel;
        CCMenuItemSpriteExtra* hostBtn;

        bool init() override;
        void onStartHost(CCObject*);
    public:
        static HostPopup* create();
};