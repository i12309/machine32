#include "pTaskList.h"
#include "pTaskRun.h"
#include "pTask.h"
#include "Catalog.h"

void pTaskList::display() {
    Log::D("pTaskList::display");
    Data::work.task.clear();
    pTaskList& UI = pTaskList::getInstance();
    Data::tasks.countPages();
    Page::setVisible(UI.bNextPage, (Data::tasks.curentPage + 1) < Data::tasks.totalPages);
    const int ITEMS_COUNT = sizeof(UI.ButtonsPressed) / sizeof(UI.ButtonsPressed[0]);
    // Очистка всех элементов
    for (int i = 0; i < ITEMS_COUNT; i++) {UI.ButtonsPressed[i]->setText("");
    }

    // Получение данных страницы
    int itemCount;
    Data::tasks.ListItems = Data::tasks.getPage(itemCount);
    // Обновление видимых элементов
    char buffer[32];
    for (int i = 0; i < itemCount && i < ITEMS_COUNT; i++) {
        // Обновление кнопки с названием задачи
        snprintf(buffer, sizeof(buffer), "  %d. %s",Data::tasks.curentPage * Data::tasks.pageSize + i + 1, Data::tasks.ListItems[i].NAME.c_str());
        UI.ButtonsPressed[i]->setText(buffer);
        // Обновление текстового поля с дополнительной информацией
        
    }
  }

void pTaskList::pop_bBackPage(void* ptr){
    Log::D(__func__);
    if (Data::tasks.curentPage==0){
                pTaskRun::getInstance().show();
    }
    else {
        Data::tasks.curentPage--;
        pTaskList::getInstance().display();
    }
}

void pTaskList::pop_bNextPage(void* ptr){
    Log::D(__func__);
    if ((Data::tasks.curentPage+1)<Data::tasks.totalPages){
      Data::tasks.curentPage++;
      pTaskList::getInstance().display();
    }
}

void pTaskList::pop_bAddItem(void* ptr){
    Log::D(__func__);
    pTask& UI = pTask::getInstance();

    Data::work.clear();
    UI.setFormMode(Catalog::FormMode::CREATE);
    UI.show();
    UI.tTaskName.setText("");
    //UI.tPaperLenghtMM.setText("");
    UI.tProductMM.setText("0");
    UI.tOverMM.setText("0");
    UI.tLastCutMM.setText("0");
    UI.tFirstCutMM.setText("0");
    UI.bListProfile.setText("Выберите профиль");
    UI.setMarkToggle(false);

    UI.bSave.attachPop(UI.pop_bNewTask,&UI.bSave);

    pTaskList::getInstance().display();
}



void pTaskList::pop_bPressed(void* ptr){
    Log::D(__func__);
    pTaskList& UI = pTaskList::getInstance();
    // Количество кнопок
    const int BUTTON_COUNT = sizeof(UI.ButtonsPressed) / sizeof(UI.ButtonsPressed[0]);
    // Определяем, какая кнопка была нажата
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (ptr == UI.ButtonsPressed[i]) {
            Data::tasks.getByPage(i,Data::work.task);
            Data::profiles.getByID(Data::work.task.PROFILE_ID,Data::work.profile);
            pTaskRun::getInstance().show();
            break;
        }
    }
}

void pTaskList::pop_bEdit(void* ptr){
    Log::D(__func__);
    pTaskList& UI = pTaskList::getInstance();
    // Количество кнопок
    const int BUTTON_COUNT = sizeof(UI.ButtonsEdit) / sizeof(UI.ButtonsEdit[0]);
    // Определяем, какая кнопка была нажата
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (ptr == UI.ButtonsEdit[i]) {
            // Получаем профиль по текущей странице и позиции кнопки
            //Log::D(i);
            Data::tasks.getByPage(i,Data::work.task); 

            Data::work.task.print();

            if (Data::work.task.valid()) {

                pTask& UI = pTask::getInstance();
                UI.setFormMode(Catalog::FormMode::EDIT);
                UI.show();

                UI.tTaskName.setText(Data::work.task.NAME.c_str());
                UI.tProductMM.setText(String(Data::work.task.PRODUCT_mm).c_str());
                UI.tOverMM.setText(String(Data::work.task.OVER_mm).c_str());
                UI.tLastCutMM.setText(String(Data::work.task.LASTCUT_mm).c_str());
                UI.setMarkToggle(Data::work.task.MARK == 0);

                Data::profiles.getByID(Data::work.task.PROFILE_ID,Data::work.profile);

                if (Data::work.profile.valid()) UI.bListProfile.setText(Data::work.profile.NAME.c_str()); else UI.bListProfile.setText("Выберите профиль!");

                UI.bSave.attachPop(UI.pop_bEditTask,&UI.bSave);
                
            }
            break;
        }
    }
}
