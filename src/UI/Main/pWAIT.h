#pragma once
#include <Nextion.h>

#include "UI/Page.h"

class pWAIT: public Page {
    public:
    
      static pWAIT& getInstance() { static pWAIT instance; return instance; }
      //void process() override { Page::process(); nexLoop(nexT); };
      void show() override { Page::show();
        tText1.setText("");
        tText2.setText("");
        tText3.setText("");
      };
    
      //#####################################################################
      NexText tText1     = NexText(20, 1, "tText1");
      NexText tText2     = NexText(20, 2, "tText2");
      NexText tText3     = NexText(20, 3, "tText3");
      //#####################################################################
      static void wait(String text1,String text2,String text3, int time, T::THandlerFunction callback, bool toBack = true){
        Log::D(__func__);
        pWAIT& WAIT = getInstance();
        WAIT.show();
        WAIT.tText1.setText(text1.c_str());
        WAIT.tText2.setText(text2.c_str());
        WAIT.tText3.setText(text3.c_str());
        delay(time);

        if (callback) callback();
        if (toBack) WAIT.back();
      }

      private:
      // Массив указателей на объекты NexTouch
      //NexTouch *nexT[1];
      pWAIT() : Page(20, 0, "p04_WAIT") {}

};
