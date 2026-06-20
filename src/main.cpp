#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <chrono>

using namespace geode::prelude;

class $modify(ModLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        cocos2d::CCPoint m_cameraVelocity = cocos2d::CCPoint(0.0f, 0.0f);
        bool m_isPanning = false;
        std::chrono::steady_clock::time_point m_lastMoveTime;
        float m_damping = Mod::get()->getSettingValue<float>("damping-force");
    };

    void updateEditor(float dt) {
        LevelEditorLayer::updateEditor(dt);
        if (!m_fields->m_isPanning) {
            if (std::abs(m_fields->m_cameraVelocity.x) > 0.1f ||
                std::abs(m_fields->m_cameraVelocity.y) > 0.1f) {

                if (this->m_objectLayer) {
                    auto currentPos = this->m_objectLayer->getPosition();

                    currentPos.x += m_fields->m_cameraVelocity.x * dt;
                    currentPos.y += m_fields->m_cameraVelocity.y * dt;
                    this->m_objectLayer->setPosition(currentPos);

                    if (this->m_editorUI) {
                        this->m_editorUI->constrainGameLayerPosition();
                        this->m_editorUI->updateSlider();
                    }

                    m_fields->m_cameraVelocity.x *=
                        std::pow(m_fields->m_damping, dt * 100.f);
                    m_fields->m_cameraVelocity.y *=
                        std::pow(m_fields->m_damping, dt * 100.f);
                }
            } else {
                m_fields->m_cameraVelocity = cocos2d::CCPoint(0.0f, 0.0f);
            }
        }
    }
};

class $modify(ModEditorUI, EditorUI) {
    bool ccTouchBegan(CCTouch *touch, CCEvent *event) {
        if (!EditorUI::ccTouchBegan(touch, event))
            return false;

        if (this->m_editorLayer) {
            auto layer =
                static_cast<ModLevelEditorLayer *>(this->m_editorLayer);
            layer->m_fields->m_isPanning = true;
            layer->m_fields->m_cameraVelocity = cocos2d::CCPoint(0.0f, 0.0f);
            layer->m_fields->m_lastMoveTime = std::chrono::steady_clock::now();
        }
        return true;
    }

    void ccTouchMoved(CCTouch *touch, CCEvent *event) {
        cocos2d::CCPoint posBefore = cocos2d::CCPoint(0, 0);
        ModLevelEditorLayer *layer = nullptr;

        if (this->m_editorLayer && this->m_editorLayer->m_objectLayer) {
            layer = static_cast<ModLevelEditorLayer *>(this->m_editorLayer);
            posBefore = layer->m_objectLayer->getPosition();
        }

        EditorUI::ccTouchMoved(touch, event);

        if (layer && layer->m_objectLayer) {
            cocos2d::CCPoint posAfter = layer->m_objectLayer->getPosition();

            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed =
                now - layer->m_fields->m_lastMoveTime;
            float dt = elapsed.count();

            if (posBefore != posAfter) {
                if (dt > 0.001f) {
                    float instantVelX = (posAfter.x - posBefore.x) / dt;
                    float instantVelY = (posAfter.y - posBefore.y) / dt;

                    layer->m_fields->m_cameraVelocity.x =
                        (layer->m_fields->m_cameraVelocity.x * 0.4f) +
                        (instantVelX * 0.6f);
                    layer->m_fields->m_cameraVelocity.y =
                        (layer->m_fields->m_cameraVelocity.y * 0.4f) +
                        (instantVelY * 0.6f);
                }
            } else {
                layer->m_fields->m_cameraVelocity =
                    cocos2d::CCPoint(0.0f, 0.0f);
            }

            layer->m_fields->m_lastMoveTime = now;
        }
    }

    void ccTouchEnded(CCTouch *touch, CCEvent *event) {
        if (this->m_editorLayer) {
            auto layer =
                static_cast<ModLevelEditorLayer *>(this->m_editorLayer);
            layer->m_fields->m_isPanning = false;

            auto now = std::chrono::steady_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - layer->m_fields->m_lastMoveTime)
                    .count();

            if (duration > 50) {
                layer->m_fields->m_cameraVelocity =
                    cocos2d::CCPoint(0.0f, 0.0f);
            }
        }

        EditorUI::ccTouchEnded(touch, event);
    }
};