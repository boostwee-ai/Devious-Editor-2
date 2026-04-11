#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

#include "../sync/SyncManager.hpp"

using namespace geode::prelude;

extern SyncManager* g_sync;
extern bool g_isInSession;
extern bool g_isHost;
extern CCPoint lastMousePos;

void objectModified(GameObject* object){
    if (g_isInSession){
        g_sync->onLocalObjectModified(object);
    }
}

void settingsUpdate(){
    if (g_isInSession && g_sync && g_isHost && !g_sync->isApplyingRemoteChanges()) {
        g_sync->onLocalLevelSettingsChanged();
    }
}

class $modify(MyLevelEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool p1) {
        if (!LevelEditorLayer::init(level, p1)) return false;
        
        if (g_isInSession && !g_isHost && g_sync) {
            log::info("Editor initialized as guest, delaying full state request.");
            this->scheduleOnce(schedule_selector(MyLevelEditorLayer::requestFullStateDelayed), 0.5f);
        }
        
        return true;
    }

    void requestFullStateDelayed(float dt) {
        if (g_sync) {
            g_sync->requestFullState();
        }
    }

    void updateOptions() {
        LevelEditorLayer::updateOptions();
        
        if (g_isInSession && g_isHost && g_sync && !g_sync->isApplyingRemoteChanges()) {
            g_sync->onLocalLevelSettingsChanged();
        }
    }
    struct Fields {
        bool m_playtesting = false;
    };

    /* -- add obj -- */
    void addKeyframe(KeyframeGameObject* p0) {
        // todo
        LevelEditorLayer::addKeyframe(p0);
    }

    void addDelayedSpawn(EffectGameObject* p0, float p1) {
        // todo
        LevelEditorLayer::addDelayedSpawn(p0, p1);
    }

    void addSpecial(GameObject* p0) {
        LevelEditorLayer::addSpecial(p0);

        if (g_isInSession && g_sync && !g_sync->isApplyingRemoteChanges()){
            // HACK: Objects in custom objects tab are detected as objects and are being sent
            // even tho they are part of the ui. We dont want to send these!!
            auto editorUI = this->m_editorUI;
            if (editorUI) {
                CCNode* parent = p0->getParent();
                bool isUIElement = false;
                
                while (parent) {
                    if (parent == editorUI) {
                        isUIElement = true;
                        break;
                    }
                    parent = parent->getParent();
                }
                
                if (!isUIElement) {
                    g_sync->onLocalObjectAdded(p0);
                }
            }
        }
    }

    void onPlaytest() {
        log::info("start playtest");
        m_fields->m_playtesting = true;
        
        LevelEditorLayer::onPlaytest();
    }

    void removeObject(GameObject* p0, bool p1) {
        if (g_isInSession && g_sync && !g_sync->isApplyingRemoteChanges()) {
            std::string uid = g_sync->getObjectUid(p0);
            if (!uid.empty()) {
                g_sync->onLocalObjectDestroyed(p0);
            }
        }
        LevelEditorLayer::removeObject(p0, p1);
    }

    void onStopPlaytest() {
        log::info("stop playtest");

        m_fields->m_playtesting = false;
        if (g_sync) {
            g_sync->cleanUpPlayers();
        }
        
        LevelEditorLayer::onStopPlaytest();
    }

    virtual ~MyLevelEditorLayer() {
        if (g_sync) {
            g_sync->clearAll();
        }
    }
};

class $modify(MyEditorUI, EditorUI){
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;
        
        if (g_isInSession && !g_isHost){
            auto settingsbtn = this->getChildByID("settings-button");
            if (settingsbtn){
                settingsbtn->setVisible(false);
            }
        }

        auto settingsBtn = this->getChildByID("settings-button");
        if (settingsBtn) {
            auto menu = settingsBtn->getParent();
            if (menu) {
                auto inspectorSprite = CCSprite::createWithSpriteFrameName("edit_eSelectionFilterBtn_001.png");
                if (!inspectorSprite) {
                    inspectorSprite = CCSprite::createWithSpriteFrameName("GJ_viewLightBtn_001.png");
                }

                auto inspectorBtn = CCMenuItemSpriteExtra::create(
                    inspectorSprite,
                    this,
                    menu_selector(MyEditorUI::onInspector)
                );
                inspectorBtn->setID("inspector-button");
                
                // Add to the same menu as settings button
                menu->addChild(inspectorBtn);
                
                // Position it next to settings button
                inspectorBtn->setPosition({
                    settingsBtn->getPositionX() - 40.f,
                    settingsBtn->getPositionY()
                });
            }
        }

        return true;
    }

    void onInspector(CCObject* sender) {
        if (g_sync) {
            g_sync->toggleInspector();
            
            auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
            if (g_sync->isInspectorEnabled()) {
                btn->setColor({0, 255, 0});
            } else {
                btn->setColor({255, 255, 255});
            }
        }
    }

    /* -- TRANSFORM -- */
    void transformObjectCall(EditCommand cmd) {
        EditorUI::transformObjectCall(cmd);

        auto selected = this->getSelectedObjects();
        for (auto obj : CCArrayExt<GameObject*>(selected)){
            objectModified(obj);
        }
    }

    void scaleObjects(CCArray* p0, float p1, float p2, CCPoint p3, ObjectScaleType p4, bool p5) {
        for (auto obj : CCArrayExt<GameObject*>(p0)){
            objectModified(obj);
        }
        EditorUI::scaleObjects(p0, p1, p2, p3, p4, p5);
    }

    void rotateObjects(CCArray* p0, float p1, CCPoint p2) {
        for (auto obj : CCArrayExt<GameObject*>(p0)){
            objectModified(obj);
        }
        EditorUI::rotateObjects(p0, p1, p2);
    }

    // idk if this is really related to transform things, but ill leave it just in case
    void updateButtons() {
        if (g_isInSession && g_sync && !g_sync->isApplyingRemoteChanges()) {
            auto selected = this->getSelectedObjects();
            if (selected && selected->count() > 0) {
                for (auto obj : CCArrayExt<GameObject*>(selected)) {
                    g_sync->onLocalObjectModified(obj);
                }
            }
        }

        EditorUI::updateButtons();
    }
    
    /* -- delete --*/
    void onDeleteSelected(CCObject* sender) {
        EditorUI::onDeleteSelected(sender);
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) {
        EditorUI::ccTouchMoved(touch, event);

        if (g_isInSession && g_sync){
            CCPoint glPos = touch->getLocation();
            lastMousePos = glPos;

            auto editorLayer = LevelEditorLayer::get();
            if (!editorLayer) return;

            auto objLayer = editorLayer->m_objectLayer;
            if (!objLayer) return;

            CCPoint worldPos = objLayer->convertToNodeSpace(glPos);

            g_sync->onLocalCursorUpdate(worldPos);
        }
    }

    // possible settings modified
    void selectBuildTab(int p0) {
        EditorUI::selectBuildTab(p0);
        
        if (p0 == 4){
            log::info("change to tab {}",p0);
            settingsUpdate();
        }
    }

    void updateSlider() {
        EditorUI::updateSlider();
        settingsUpdate();
    }

    void toggleMode(CCObject* p0) {
        EditorUI::toggleMode(p0);
        settingsUpdate();
    }

    void songStateChanged() {
        EditorUI::songStateChanged();
        settingsUpdate();
    }

    void selectObjects(CCArray* objects, bool idk) {
        EditorUI::selectObjects(objects, idk);
        
        if (g_isInSession && g_sync && !g_sync->isApplyingRemoteChanges()){
            g_sync->onLocalSelectionChanged(objects);
        }
    }

    void deselectAll() {
        if (g_isInSession && g_sync && !g_sync->isApplyingRemoteChanges()){
            auto emptyArray = CCArray::create();
            g_sync->onLocalSelectionChanged(emptyArray);
        }
        
        EditorUI::deselectAll();
    }
};