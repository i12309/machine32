#pragma once
#include <functional>
#include <Nextion.h>

#include "UI/Page.h"

class pINFO: public Page {
    public:

      static pINFO& getInstance() { static pINFO instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};

      static void showInfo(String text1 = "", String text2 = "", String text3 = "",
                           std::function<void()> onOk = nullptr,
                           std::function<void()> onCancel = nullptr,
                           bool showCancel = false,
                           String okText = "",
                           String cancelText = "") {
          pINFO& page = getInstance();
          page._okCallback = onOk;
          page._cancelCallback = onCancel;
          const bool cancelVisible = showCancel || (onCancel != nullptr);
          page.show();
          page.tInfo1.setText(text1.c_str());
          page.tInfo2.setText(text2.c_str());
          page.tInfo3.setText(text3.c_str());
          page.setButtonTexts(okText, cancelText, cancelVisible);
          page.setCancelVisible(cancelVisible);
      }

      //#####################################################################
      // Text
      NexText t0     = NexText(23, 2, "t0");
      NexText tInfo1 = NexText(23, 4, "tInfo1");
      NexText tInfo2 = NexText(23, 5, "tInfo2");
      NexText tInfo3 = NexText(23, 6, "tInfo3");

      // Button
      NexButton bOK    = NexButton(23, 3, "bOK");
      NexButton bCancel    = NexButton(23, 7, "bCancel");
      
      //#####################################################################

    private:
      std::function<void()> _okCallback = nullptr;
      std::function<void()> _cancelCallback = nullptr;
      bool _cancelColorsInit = false;
      uint16_t _cancelBcoNormal = 0;
      uint16_t _cancelPcoNormal = 0;
      String _okDefaultText = "OK";
      String _cancelDefaultText = "Отмена";
      NexTouch *nexT[3];

      //#####################################################################

      pINFO() : Page(23, 0, "p04_INFO") {
          nexT[0] = &bOK;
          nexT[1] = &bCancel;
          nexT[2] = NULL;

          bOK.attachPop(pop_bOK, &bOK);
      }

      //#####################################################################

      static void pop_bOK(void* ptr){
          Log::D(__func__);
          pINFO& page = getInstance();
          std::function<void()> callback = page._okCallback;
          page._okCallback = nullptr;
          page._cancelCallback = nullptr;
          if (callback) callback();
          else page.back();
      }

      static void pop_bCancel(void* ptr){
          Log::D(__func__);
          pINFO& page = getInstance();
          std::function<void()> callback = page._cancelCallback;
          page._okCallback = nullptr;
          page._cancelCallback = nullptr;
          if (callback) callback();
          else page.back();
      }

      void setCancelVisible(bool visible) {
          initCancelColors();
          if (visible) {
              bCancel.Set_background_color_bco(_cancelBcoNormal);
              bCancel.Set_font_color_pco(_cancelPcoNormal);
              bCancel.attachPop(pop_bCancel, &bCancel);
          } else {
              bCancel.Set_background_color_bco(Catalog::Color::lighter);
              bCancel.Set_font_color_pco(Catalog::Color::lighter);
              bCancel.detachPop();
          }
      }

      void initCancelColors() {
          if (_cancelColorsInit) return;
          uint32_t bco = 0;
          uint32_t pco = 0;
          bCancel.Get_background_color_bco(&bco);
          bCancel.Get_font_color_pco(&pco);
          _cancelBcoNormal = static_cast<uint16_t>(bco);
          _cancelPcoNormal = static_cast<uint16_t>(pco);
          _cancelColorsInit = true;
      }

      void setButtonTexts(const String& okText, const String& cancelText, bool cancelVisible) {
          const String okLabel = okText.length() > 0 ? okText : _okDefaultText;
          bOK.setText(okLabel.c_str());

          if (cancelVisible) {
              const String cancelLabel = cancelText.length() > 0 ? cancelText : _cancelDefaultText;
              bCancel.setText(cancelLabel.c_str());
          }
      }

};
