#pragma once
#include <Nextion.h>

#include "../Page.h"

class PinTest: public Page {
    public:
      static PinTest& getInstance() { static PinTest instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();
        PinTest::pop_radio(&esp);
      };

      //#####################################################################
      // Text
      NexText t1    = NexText(27, 4, "t1");
      NexText t2    = NexText(27, 5, "t2");
      NexText t3    = NexText(27, 7, "t3");
      NexText t4    = NexText(27, 9, "t4");
      NexText t5    = NexText(27, 11, "t5");
      NexText t6    = NexText(27, 13, "t6");
      NexText t7    = NexText(27, 15, "t7");
      NexText t8    = NexText(27, 18, "t8");
      NexText t9    = NexText(27, 19, "t9");
      NexText t10   = NexText(27, 21, "t10");
      NexText t11   = NexText(27, 23, "t11");
      NexText t12   = NexText(27, 25, "t12");
      NexText t13   = NexText(27, 27, "t13");
      NexText t14   = NexText(27, 29, "t14");
      NexText t15   = NexText(27, 32, "t15");
      NexText t16   = NexText(27, 33, "t16");
      NexText t17   = NexText(27, 35, "t17");
      NexText t18   = NexText(27, 37, "t18");
      NexText t19   = NexText(27, 39, "t19");
      NexText t20   = NexText(27, 41, "t20");
      NexText t21   = NexText(27, 43, "t21");

      NexText *Text[21];
      // Button
      NexButton bBack = NexButton(27, 2, "bBack");

      NexDSButton *Switch[21];

      // Switch
      NexDSButton p1    = NexDSButton(27, 3, "p1");
      NexDSButton p2    = NexDSButton(27, 6, "p2");
      NexDSButton p3    = NexDSButton(27, 8, "p3");
      NexDSButton p4    = NexDSButton(27, 10, "p4");
      NexDSButton p5    = NexDSButton(27, 12, "p5");
      NexDSButton p6    = NexDSButton(27, 14, "p6");
      NexDSButton p7    = NexDSButton(27, 16, "p7");
      NexDSButton p8    = NexDSButton(27, 17, "p8");
      NexDSButton p9    = NexDSButton(27, 20, "p9");
      NexDSButton p10   = NexDSButton(27, 22, "p10");
      NexDSButton p11   = NexDSButton(27, 24, "p11");
      NexDSButton p12   = NexDSButton(27, 26, "p12");
      NexDSButton p13   = NexDSButton(27, 28, "p13");
      NexDSButton p14   = NexDSButton(27, 30, "p14");
      NexDSButton p15   = NexDSButton(27, 31, "p15");
      NexDSButton p16   = NexDSButton(27, 34, "p16");
      NexDSButton p17   = NexDSButton(27, 36, "p17");
      NexDSButton p18   = NexDSButton(27, 38, "p18");
      NexDSButton p19   = NexDSButton(27, 40, "p19");
      NexDSButton p20   = NexDSButton(27, 42, "p20");
      NexDSButton p21   = NexDSButton(27, 44, "p21");

      // Radio
      NexRadio esp   = NexRadio(27, 45, "esp");
      NexRadio mcp1  = NexRadio(27, 47, "mcp1");
      NexRadio mcp2  = NexRadio(27, 49, "mcp2");
      NexRadio mcp3  = NexRadio(27, 51, "mcp3");
      //#####################################################################

    private:
      enum Device {
        DEV_NONE,
        DEV_ESP32,
        DEV_MCP1,
        DEV_MCP2,
        DEV_MCP3
      };
      Device currentDevice;

      NexTouch *nexT[27];

      PinTest() : Page(27, 0, "p5_PinTest") {
          Text[0]=&t1;
          Text[1]=&t2;
          Text[2]=&t3;
          Text[3]=&t4;
          Text[4]=&t5;
          Text[5]=&t6;
          Text[6]=&t7;
          Text[7]=&t8;
          Text[8]=&t9;
          Text[9]=&t10;
          Text[10]=&t11;
          Text[11]=&t12;
          Text[12]=&t13;
          Text[13]=&t14;
          Text[14]=&t15;
          Text[15]=&t16;
          Text[16]=&t17;
          Text[17]=&t18;
          Text[18]=&t19;
          Text[19]=&t20;
          Text[20]=&t21;
          
          currentDevice = DEV_NONE;
          nexT[0]=&bBack;
          nexT[1]=&p1;
          nexT[2]=&p2;
          nexT[3]=&p3;
          nexT[4]=&p4;
          nexT[5]=&p5;
          nexT[6]=&p6;
          nexT[7]=&p7;
          nexT[8]=&p8;
          nexT[9]=&p9;
          nexT[10]=&p10;
          nexT[11]=&p11;
          nexT[12]=&p12;
          nexT[13]=&p13;
          nexT[14]=&p14;
          nexT[15]=&p15;
          nexT[16]=&p16;
          nexT[17]=&p17;
          nexT[18]=&p18;
          nexT[19]=&p19;
          nexT[20]=&p20;
          nexT[21]=&p21;
          nexT[22]=&esp;
          nexT[23]=&mcp1;
          nexT[24]=&mcp2;
          nexT[25]=&mcp3;
          nexT[26] = NULL;

          Switch[0]=&p1;
          Switch[1]=&p2;
          Switch[2]=&p3;
          Switch[3]=&p4;
          Switch[4]=&p5;
          Switch[5]=&p6;
          Switch[6]=&p7;
          Switch[7]=&p8;
          Switch[8]=&p9;
          Switch[9]=&p10;
          Switch[10]=&p11;
          Switch[11]=&p12;
          Switch[12]=&p13;
          Switch[13]=&p14;
          Switch[14]=&p15;
          Switch[15]=&p16;
          Switch[16]=&p17;
          Switch[17]=&p18;
          Switch[18]=&p19;
          Switch[19]=&p20;
          Switch[20]=&p21;

          bBack.attachPop(pop_bBack, &bBack);

          for (int i = 0; i < (sizeof(Switch) / sizeof(Switch[0])); i++) { Switch[i]->attachPop(pop_switch, Switch[i]);  }

          esp.attachPop(pop_radio, &esp);
          mcp1.attachPop(pop_radio, &mcp1);
          mcp2.attachPop(pop_radio, &mcp2);
          mcp3.attachPop(pop_radio, &mcp3);
      }

      static void pop_bBack(void* ptr);
      static void pop_switch(void* ptr);
      static void pop_radio(void* ptr);


};
