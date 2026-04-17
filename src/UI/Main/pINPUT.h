#pragma once
#include <Nextion.h>
#include <functional>

#include "UI/Page.h"

class pINPUT: public Page {
    public:

      using InputCallback = std::function<void(const String&)>;

      static pINPUT& getInstance() { static pINPUT instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};

      static void showInput(const String& title, const String& info1, const String& info2, const String& defaultValue = "", InputCallback onOk = nullptr, T::THandlerFunction onCancel = nullptr, int showField = 0, bool autoBack = true);

      //#####################################################################
      // Text
      NexText tTitle      = NexText(24, 1, "tTitle");
      NexText tInfo1  = NexText(24, 3, "tInfo1");
      NexText tInfo2  = NexText(24, 4, "tInfo2");
      NexText tInfo3  = NexText(24, 5, "tInfo3");
      NexText tField      = NexText(24, 7, "tField");

      NexVariable showField = NexVariable(24, 7, "showField");

      // Button
      NexButton bOK     = NexButton(24, 2, "bOK");
      NexButton bCancel = NexButton(24, 6, "bCancel");
      //#####################################################################

    private:
      NexTouch *nexT[3];

      //#####################################################################

      pINPUT() : Page(24, 0, "p05_INPUT") {
          nexT[0] = &bOK;
          nexT[1] = &bCancel;
          nexT[2] = NULL;

          bOK.attachPop(pop_bOK, &bOK);
          bCancel.attachPop(pop_bCancel, &bCancel);
      }

      //#####################################################################

      static void pop_bOK(void* ptr);
      static void pop_bCancel(void* ptr);
      static void resetHandlers();

      static InputCallback onOkHandler;
      static T::THandlerFunction onCancelHandler;
      static bool autoBackOnClose;

};
