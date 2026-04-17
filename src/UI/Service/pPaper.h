#pragma once
#include <Nextion.h>
#include "Catalog.h"

#include "../Page.h"
#include "Controller/Registry.h"

class pPaper: public Page {
    public:
    
      static pPaper& getInstance() { static pPaper instance; return instance; }
    
      void loop() override { Page::loop(); nexLoop(nexT); 
        switch(App::state()->type()) {
          case State::Type::SERVICE: n_Service(); break;
        }      
      };
    
      void show() override { Page::show();
        // При открытии формы всегда синхронизируем переключатель с фактическим состоянием муфты.
        if (App::ctx().swCatch != nullptr) {
          App::ctx().swCatch->off();
          clutch_switch = App::ctx().swCatch->power;
        } else {
          clutch_switch = false;
        }
        setClutchUi(clutch_switch);
        setShowThrowUi(App::ctx().swThrow != nullptr);
        tValue.setText("0");
        tEncoder.setText(String(App::ctx().mPaper->getCurrentPosition()).c_str());
        setDirUi();
        setOpticalUi();
        updateOpticalStateUi(true);
        updateEncoderUi();
        normalizeStepValueUi();
      };
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[16];

      bool throw_switch;
      bool clutch_switch = true;
      bool opticalStateInit = false;
      bool lastMarkWhite = false;
      bool lastEdgeWhite = false;
    
      //#####################################################################
      NexCheckbox cCheckMark   = NexCheckbox(11, 9, "cCheckMark");
      
      NexText tEncoder       = NexText(11, 13, "tEncoder");
      NexCheckbox cEncoder      = NexCheckbox(11, 14, "cEncoder");
      NexCheckbox cCheckEdge   = NexCheckbox(11, 21, "cCheckEdge");
      //#####################################################################
      NexButton bBack         = NexButton(11,7,"bBack");
      NexButton bDetectPaper  = NexButton(11,2,"bDetectPaper");
      NexButton bDetectMark   = NexButton(11,3,"bDetectMark");
      NexButton bClutch    = NexButton(11,4,"bClutch");
      NexButton bStop      = NexButton(11,8,"bStop");
      NexButton bPForward   = NexButton(11,5,"bPForward");
      NexButton bPBack      = NexButton(11,6,"bPBack");
      NexButton bThrow      = NexButton(11,10,"bThrow");

      NexPicture rClutch      = NexPicture(11,11,"rClutch");
      NexPicture rThrow      = NexPicture(11,12,"rThrow");

      NexButton bBwd      = NexButton(11,15,"bBwd");
      NexButton bFwd      = NexButton(11,16,"bFwd");
      NexButton bMark      = NexButton(11,17,"bMark");
      NexButton bEdge      = NexButton(11,18,"bEdge");

      NexButton bStepMM      = NexButton(11,19,"bStepMM");
      NexText tValue      = NexText(11,20,"tValue");
      
      //#####################################################################
      pPaper() : Page(11, 0, "p5_Paper") {
        nexT[0] = &bBack;
        nexT[1] = &bDetectPaper;
        nexT[2] = &bDetectMark;
        nexT[3] = &bClutch;
        nexT[4] = &bStop;
        nexT[5] = &bPForward;
        nexT[6] = &bPBack;
        nexT[7] = &bThrow;
        nexT[8] = &bBwd;
        nexT[9] = &bFwd;
        nexT[10] = &bMark;
        nexT[11] = &bEdge;
        nexT[12] = &bStepMM;
        nexT[13] = &tEncoder;
        nexT[14] = &cCheckMark;
        nexT[15] = NULL;  // Завершающий NULL
  
        bBack.attachPop(pop_bBack, &bBack);

        bDetectPaper.attachPop(pop_bDetectPaper, &bDetectPaper);
        bDetectMark.attachPop(pop_bDetectMark, &bDetectMark);
        bClutch.attachPop(pop_bClutch, &bClutch);
        bThrow.attachPop(pop_bThrow, &bThrow);

        bStop.attachPop(pop_bStop, &bStop);

        bPForward.attachPop(pop_bPForward, &bPForward);
        bPForward.attachPush(push_bPForward, &bPForward);

        bPBack.attachPop(pop_bPBack, &bPBack);
        bPBack.attachPush(push_bPBack, &bPBack);

        bBwd.attachPop(pop_bBwd, &bBwd);
        bFwd.attachPop(pop_bFwd, &bFwd);
        bMark.attachPop(pop_bMark, &bMark);
        bEdge.attachPop(pop_bEdge, &bEdge);
        bStepMM.attachPop(pop_bStepMM, &bStepMM);

        tEncoder.attachPop(pop_tEncoder, &tEncoder);
        cCheckMark.attachPop(pop_cCheck, &cCheckMark);

      }
    
  //#####################################################################

  static void pop_bBack(void* ptr);

  static void pop_bDetectPaper(void* ptr);
  static void pop_bDetectMark(void* ptr);
  static void pop_bClutch(void* ptr);
  static void pop_bThrow(void* ptr);
  
  static void pop_bStop(void* ptr);
  
  static void pop_bPForward(void* ptr);
  static void push_bPForward(void* ptr);
  
  static void pop_bPBack(void* ptr);
  static void push_bPBack(void* ptr);

  static void pop_bBwd(void* ptr);
  static void pop_bFwd(void* ptr);
  static void pop_bMark(void* ptr);
  static void pop_bEdge(void* ptr);
  static void pop_bStepMM(void* ptr);
  static void pop_cCheck(void* ptr);
  static void pop_tEncoder(void* ptr);

  //#####################################################################
  void setSensorUi(NexCheckbox& checkbox, bool white) {
      const uint16_t color = white ? Catalog::Color::green : Catalog::Color::red;
      checkbox.Set_background_color_bco(color);
      checkbox.Set_font_color_pco(color);
  }

  void updateOpticalStateUi(bool force = false) {
      bool markWhite = (App::ctx().oMark != nullptr) ? App::ctx().oMark->checkBlack() : false;
      bool edgeWhite = (App::ctx().oEdge != nullptr) ? App::ctx().oEdge->checkBlack() : false;

      if (force || !opticalStateInit || markWhite != lastMarkWhite) {
          setSensorUi(cCheckMark, markWhite);
          lastMarkWhite = markWhite;
      }

      if (force || !opticalStateInit || edgeWhite != lastEdgeWhite) {
          setSensorUi(cCheckEdge, edgeWhite);
          lastEdgeWhite = edgeWhite;
      }

      opticalStateInit = true;
  }

  void setClutchUi(bool enabled) {
      if (enabled) {
          bClutch.Set_background_color_bco(Catalog::Color::yellow); // Желтый
          rClutch.Set_background_image_pic(Catalog::UI::sw_pic2);
      } else {
          bClutch.Set_background_color_bco(Catalog::Color::grey); // Серый
          rClutch.Set_background_image_pic(Catalog::UI::sw_pic1);
      }
  }

  void setThrowUi(bool enabled) {
    if (enabled) {
        bThrow.Set_background_color_bco(Catalog::Color::yellow); // Желтый
        rThrow.Set_background_image_pic(Catalog::UI::sw_pic2);
    } else {
        bThrow.Set_background_color_bco(Catalog::Color::grey); // Серый
        rThrow.Set_background_image_pic(Catalog::UI::sw_pic1);
    }
  }

  void setShowThrowUi(bool hasMotor){
    if (hasMotor) {
        bThrow.setText("   Выброс");
        Page::setVisible(rThrow, true);
        throw_switch = App::ctx().swThrow->power;
    } else {
        bThrow.setText("");
        Page::setVisible(rThrow, false);
        throw_switch = false;
    }
    setThrowUi(throw_switch);
  }

  void setDirUi() {
      const uint16_t active = Catalog::Color::darkGrey;
      const uint16_t inactive = Catalog::Color::lightGrey;
      const uint16_t activeText = Catalog::Color::white;
      const uint16_t inactiveText = Catalog::Color::mediumGrey;

      if (detectDir == Catalog::DIR::Backward) {
          bBwd.Set_background_color_bco(active);
          bBwd.Set_font_color_pco(activeText);
          bFwd.Set_background_color_bco(inactive);
          bFwd.Set_font_color_pco(inactiveText);
      } else {
          bBwd.Set_background_color_bco(inactive);
          bBwd.Set_font_color_pco(inactiveText);
          bFwd.Set_background_color_bco(active);
          bFwd.Set_font_color_pco(activeText);
      }
  }

  void setOpticalUi() {
      const uint16_t active = Catalog::Color::darkGrey;
      const uint16_t inactive = Catalog::Color::lightGrey;
      const uint16_t activeText = Catalog::Color::white;
      const uint16_t inactiveText = Catalog::Color::mediumGrey;

      if (opticalSensor == Catalog::OpticalSensor::MARK) {
          bMark.Set_background_color_bco(active);
          bMark.Set_font_color_pco(activeText);
          bEdge.Set_background_color_bco(inactive);
          bEdge.Set_font_color_pco(inactiveText);
      } else {
          bMark.Set_background_color_bco(inactive);
          bMark.Set_font_color_pco(inactiveText);
          bEdge.Set_background_color_bco(active);
          bEdge.Set_font_color_pco(activeText);
      }
  }

  int normalizeStepValueUi() {
      String value = getText(tValue, 10);
      value.trim();

      int step = atoi(value.c_str());
      if (step < 0) {
          step = 0;
          tValue.setText("0");
      }

      const uint16_t background = (step == 0) ? Catalog::Color::lightGrey : Catalog::Color::grey;
      tValue.Set_background_color_bco(background);
      bStepMM.Set_background_color_bco(background);

      return step;
  }

  // Меняет цвет индикатора лимита энкодера
  void setEncoderLimitUi(bool limit) {
      uint16_t color = limit ? Catalog::Color::orange : Catalog::Color::grey; // Оранжевый / Серый
      cEncoder.Set_background_color_bco(color);
      cEncoder.Set_font_color_pco(color);
  }

  // Обновляет значение энкодера и индикатор лимита с кешированием
  void updateEncoderUi() {
      IEncoder* encoder = App::ctx().eCatch;
      if (!encoder) return;

      // Проверяем выход за порог и меняем цвет только при изменении.
          setEncoderLimitUi((encoder->getCount() > encoder->getThreshold()));
  }
   

  void n_Service() {
      updateOpticalStateUi();

      updateEncoderUi();

      static unsigned long previousMillis;  // Время последней печати TODO - удалить
      if (millis() - previousMillis >= 1000) { 
        previousMillis = millis(); 
        tEncoder.setText(String(App::ctx().ePaper->getCount()).c_str());
        normalizeStepValueUi();
    } 
  };

  Catalog::DIR detectDir = Catalog::DIR::Forward;
  Catalog::OpticalSensor opticalSensor = Catalog::OpticalSensor::EDGE;

};






