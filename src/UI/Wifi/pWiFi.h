#pragma once
#include <Nextion.h>

#include "Service/WiFiConfig.h"
#include <WiFi.h>

#include "../Page.h"

class pWiFi: public Page {
    public:

      static pWiFi& getInstance() { static pWiFi instance; return instance; }

    void loop() override { Page::loop(); nexLoop(nexT);
      switch(App::state()->type()) {
        case State::Type::IDLE: n_Idle(); break;
      }
    };

      void show() override;

      //#####################################################################

      NexText tRSSIvalue   = NexText(14, 6, "tRSSIvalue");
      NexText tRSSI   = NexText(14, 9, "tRSSI");
      NexText tIP     = NexText(14, 10, "tIP");
      NexText tIPvalue     = NexText(14, 11, "tIPvalue");

      // Combo Box
      NexObject cbSSID       = NexObject(14, 12, "cbSSID");

      // Toggle
      NexText tAutoConnect = NexText(14, 14, "tAutoConnect");
      NexPicture rAutoConnect = NexPicture(14, 15, "rAutoConnect");

      //#####################################################################

private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[7];
      bool autoConnectDirty = false;
      bool autoConnectPending = false;
      bool autoConnectUiState = false;

      NexButton bSetWifi     = NexButton(14, 4, "bSetWifi");
      NexButton bBack        = NexButton(14, 5, "bBack");
      NexButton bConnect     = NexButton(14, 6, "bConnect");
      NexButton bAddItem     = NexButton(14, 8, "bAddItem");
      NexButton bDel     = NexButton(14, 13, "bDel");

      //#####################################################################
      pWiFi() : Page(14, 0, "p4_Wifi") {
          nexT[0] = &bBack;
          nexT[1] = &bSetWifi;
          nexT[2] = &bConnect;
          nexT[3] = &bAddItem;
          nexT[4] = &bDel;
          nexT[5] = &tAutoConnect;
          nexT[6] = NULL;  // Завершающий NULL

          bBack.attachPop(pop_bBack, &bBack);
          bSetWifi.attachPop(pop_bSetWifi, &bSetWifi);
          bConnect.attachPop(pop_bConnect, &bConnect);
          bAddItem.attachPop(pop_bAddItem, &bAddItem);
          bDel.attachPop(pop_bDel, &bDel);
          tAutoConnect.attachPop(pop_tAutoConnect, &tAutoConnect);
      }

  //#####################################################################

  static void pop_bBack(void* ptr);
  static void pop_bSetWifi(void* ptr);
  static void pop_bConnect(void* ptr);
  static void pop_bAddItem(void* ptr);
  static void pop_bDel(void* ptr);
  static void pop_tAutoConnect(void* ptr);

  //#####################################################################

  void n_Idle();
  void refreshList();
  void updateConnectButton();
  void updateConnectionInfo();
  void updateAutoConnect();
  uint32_t getSelectedIndex();
  String getSelectedSSID();
  void setConnectButtonState(bool connected);

};
