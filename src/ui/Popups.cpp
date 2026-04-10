#include "Popups.hpp"

bool JoinSessionPopup::init(){
    if (!geode::Popup::init(300.f, 200.f)) return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    this->setTitle("Join Session");

    return true;
}

void JoinSessionPopup::onConnect(CCObject *){
    this->onClose(nullptr);
}

JoinSessionPopup* JoinSessionPopup::create(){
    auto ret = new JoinSessionPopup();
    if (ret && ret->init()){
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}