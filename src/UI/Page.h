#pragma once
#include <Nextion.h>

#include "App/App.h"
#include "State/State.h"
#include "Controller/Registry.h"
#include "State/Scene.h"

class Page {

public:
    Page(uint8_t pid, uint8_t cid, const char* name) : nexPage(pid, cid, name) {}

    static Page* activePage; // на активную старницу 
    static Page* previousPage; // Указатель на предыдущую страницу
    static const char* _func; bool _setFunc(const char* func) { if (_func != func) {_func = func; return true; } return false; }
    static bool nextionInit(){activePage = nullptr; return nexInit();}
    virtual void loop(){/*В каждой странице UI есть loop() который, в нужном состоянии, вызывает нужный метод для обновления*/};
    static void process();
    // Register the active page listener list before delegating to Nextion.
    static void processTouch(NexTouch* touchList[]) {
        nexLoop(touchList);
    }

    NexPage nexPage;
    bool isShow = false;

    virtual void show(){
        if (activePage != nullptr) activePage->hide();
        isShow = nexPage.show(); 
        previousPage = activePage; // сохраняем 
        activePage = this; // обновляем 
    }

    virtual void hide(){isShow = false;}

    virtual void back(){
        if (activePage != nullptr) activePage->hide();
        activePage = previousPage; // обновляем
        previousPage = nullptr; 
        activePage->show(); 
    }

    virtual void checkButton(String nameButton, T::THandlerPageFunction func){
        // проверяем нажата ли кнопка 
        if (App::ctx().reg.getButton(nameButton)->isTrigger()) func(nullptr);
    }

public:
    String getText(NexText& object,const int len){
        char text[len] = {0};
        memset(text, 0, sizeof(text));
        object.getText(text, sizeof(text));
        return String(text);
    }

    struct TextPicToggleStyle {
        uint16_t bcoOn;
        uint16_t bcoOff;
        uint16_t pcoOn;
        uint16_t pcoOff;
        uint32_t picOn;
        uint32_t picOff;
    };

    static void setTextPicToggle(NexText& text, NexPicture& pic, bool selected, const TextPicToggleStyle& style) {
        if (selected) {
            text.Set_background_color_bco(style.bcoOn);
            text.Set_font_color_pco(style.pcoOn);
            pic.Set_background_image_pic(style.picOn);
        } else {
            text.Set_background_color_bco(style.bcoOff);
            text.Set_font_color_pco(style.pcoOff);
            pic.Set_background_image_pic(style.picOff);
        }
    }

    static bool isTextSelectedByColor(NexText& text, uint16_t selectedBco) {
        uint32_t bco = 0;
        text.Get_background_color_bco(&bco);
        return bco == selectedBco;
    }

    static bool toggleTextPicByColor(NexText& text, NexPicture& pic, const TextPicToggleStyle& style) {
        bool selected = isTextSelectedByColor(text, style.bcoOn);
        bool next = !selected;
        setTextPicToggle(text, pic, next, style);
        return next;
    }

    static void setObjectY(const char* objName, int y) {
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "%s.y=%d", objName, y);
        sendCommand(cmd);
    }

    static void setVisible(NexObject& obj, bool visible) {
        String cmd = "vis " + String(obj.getObjName()) + "," + String(visible ? 1 : 0);
        sendCommand(cmd.c_str());
    }

    static void setVisibleY(const char* objName, int visibleY, bool visible) {
        setObjectY(objName, visible ? visibleY : 1000);
    }

    int getCBvalue(NexCheckbox& object){
        uint32_t tempValue;
        object.getValue(&tempValue);
        return static_cast<int>(tempValue);
    }

    void printNexT(NexTouch* elements[], size_t count) {
        Serial.printf("count Element %d\n",count);
        for (size_t i = 0; i < count; i++) {
            if (elements[i] != nullptr) {
                // Получаем имя объекта через его атрибуты
                uint8_t pid, cid;
                //char name[16] = {0}; // Буфер для имени
                
                // Для NexButton/NexText и других наследников NexTouch
                pid = elements[i]->getObjPid();
                cid = elements[i]->getObjCid();
                const char* name = elements[i]->getObjName();

                if (pid && cid && name ) {
                    
                    Serial.printf("Element %d: Page=%d, ID=%d, Name='%s'\n",i, pid, cid, name);
                } else {
                    Serial.printf("Element %d: Unknown type\n", i);
                }
            } else {
                Serial.printf("Element %d: NULL\n", i);
            }
        }
    }

private:
    static void processWarnings();
};

#define nexLoop(nex_listen_list) Page::processTouch(nex_listen_list)


