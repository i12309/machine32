#pragma once
#include <Nextion.h>
#include <vector>

#include "../Page.h"

class pShowStat: public Page {
    public:

      static pShowStat& getInstance() { static pShowStat instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};

      void showDeviceMotors();
      void showTasks();
      void showProfiles();

      //#####################################################################
      // Text
      NexText tTitle        = NexText(29, 1, "tTitle");
      NexText tField1       = NexText(29, 3, "tField1");
      NexText tField2       = NexText(29, 4, "tField2");
      NexText tParam1       = NexText(29, 5, "tParam1");
      NexText tValue1       = NexText(29, 6, "tValue1");
      NexText tParam2       = NexText(29, 7, "tParam2");
      NexText tValue2       = NexText(29, 8, "tValue2");
      NexText tParam3       = NexText(29, 9, "tParam3");
      NexText tValue3       = NexText(29, 10, "tValue3");
      NexText tParam4       = NexText(29, 11, "tParam4");
      NexText tValue4       = NexText(29, 12, "tValue4");
      NexText tParam5       = NexText(29, 13, "tParam5");
      NexText tValue5       = NexText(29, 14, "tValue5");
      NexText tParam6       = NexText(29, 15, "tParam6");
      NexText tValue6       = NexText(29, 16, "tValue6");

      // Button
      NexButton bBackMainMenu = NexButton(29, 2, "bBackMainMenu");
      NexButton bNextPage     = NexButton(29, 17, "bNextPage");
      //#####################################################################

    private:
      enum class ViewMode : uint8_t {
          DeviceMotors,
          Tasks,
          TaskDetail,
          TaskProfiles,
          TaskMotors,
          Profiles,
          ProfileDetailGlobal,
          ProfileDetailTask,
          ProfileMotorsGlobal,
          ProfileMotorsTask
      };

      enum class ActionType : uint8_t {
          None,
          OpenTaskDetail,
          OpenTaskProfiles,
          OpenTaskMotors,
          OpenProfileDetailGlobal,
          OpenProfileDetailTask,
          OpenProfileMotorsGlobal,
          OpenProfileMotorsTask
      };

      struct RowAction {
          ActionType type = ActionType::None;
          long taskId = -1;
          long profileId = -1;
      };

      struct Row {
          String label;
          String value;
          RowAction action;
      };

      struct ViewState {
          ViewMode view;
          long taskId;
          long profileId;
          uint8_t page;
      };

      static const uint8_t kPageSize = 6;
      static const int kNextPageY = 2;

      NexTouch *nexT[15];
      NexText *paramFields[kPageSize];
      NexText *valueFields[kPageSize];

      std::vector<Row> rows;
      std::vector<ViewState> nav;
      ViewMode view = ViewMode::DeviceMotors;
      long viewTaskId = -1;
      long viewProfileId = -1;
      uint8_t page = 0;
      uint8_t totalPages = 0;
      String title;
      String field1;
      String field2;

      //#####################################################################

      pShowStat() : Page(29, 0, "p6_ShowStat") {
          paramFields[0] = &tParam1;
          paramFields[1] = &tParam2;
          paramFields[2] = &tParam3;
          paramFields[3] = &tParam4;
          paramFields[4] = &tParam5;
          paramFields[5] = &tParam6;

          valueFields[0] = &tValue1;
          valueFields[1] = &tValue2;
          valueFields[2] = &tValue3;
          valueFields[3] = &tValue4;
          valueFields[4] = &tValue5;
          valueFields[5] = &tValue6;

          int idx = 0;
          nexT[idx++] = &bBackMainMenu;
          nexT[idx++] = &bNextPage;
          for (int i = 0; i < kPageSize; i++) {
              nexT[idx++] = paramFields[i];
              nexT[idx++] = valueFields[i];
          }
          nexT[idx] = NULL;

          bBackMainMenu.attachPop(pop_bBackMainMenu, &bBackMainMenu);
          bNextPage.attachPop(pop_bNextPage, &bNextPage);
          for (int i = 0; i < kPageSize; i++) {
              paramFields[i]->attachPop(pop_row, paramFields[i]);
              valueFields[i]->attachPop(pop_row, valueFields[i]);
          }
      }

      //#####################################################################

      static void pop_bBackMainMenu(void* ptr);
      static void pop_bNextPage(void* ptr);
      static void pop_row(void* ptr);

      void setView(ViewMode newView, long taskId, long profileId, bool push);
      void buildRows();
      void renderPage();
      void clearRows();
      void updateNextButton();
      void handleAction(const RowAction& action);
      int rowIndexFromPtr(void* ptr) const;
      void nextPage();
      void goBack();

};
