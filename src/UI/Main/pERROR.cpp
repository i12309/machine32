#include "pERROR.h"
#include "UI/Service/pService.h"
#include "pMain.h"

void pERROR::pop_bService(void* ptr) {
  Log::D(__func__);
  App::diag().clear();
  App::state()->setFactory(State::Type::SERVICE);
  pService::getInstance().show();
}

void pERROR::pop_bDropError(void* ptr) {
  Log::D(__func__);
  App::diag().clear();
  App::state()->setFactory(State::Type::IDLE);
  pMain::getInstance().show();
}

void pERROR::pop_bNext(void* ptr) {
  Log::D(__func__);
  App::diag().next();
  pERROR::getInstance().renderError();
}

void pERROR::pop_bBack(void* ptr) {
  Log::D(__func__);
  App::diag().prev();
  pERROR::getInstance().renderError();
}

void pERROR::renderError() {
  auto& errors = App::diag();
  if (errors.hasMessages()) {
    const auto& err = errors.current();
    const auto& entry = errors.currentEntry();
    const size_t index = errors.currentIndex() + 1;
    const size_t total = errors.count();

    String title = "Ошибка";
    if (entry.kind == DeviceError::Kind::Warning) {
      title = "Предупреждение";
    } else if (entry.kind == DeviceError::Kind::Fatal) {
      title = "Критическая ошибка";
    }

    if (total > 1) {
      String header = title + String(" (") + String(index) + " из " + String(total) + "):";
      tErrorInfo1.setText(header.c_str());
    } else {
      String header = title + ":";
      tErrorInfo1.setText(header.c_str());
    }

    tErrorInfo2.setText(Catalog::errorName(err.code).c_str());

    if (err.value.length() > 0) {
      tErrorInfo3.setText(err.value.c_str());
    } else {
      tErrorInfo3.setText(err.description.c_str());
    }

    Page::setVisible(bNext, errors.canNext());
    Page::setVisible(bBack, errors.canPrev());
    return;
  }

  const auto& err = errors.currentOrNoError();
  tErrorInfo1.setText("Диагностика:");
  tErrorInfo2.setText(Catalog::errorName(err.code).c_str());
  tErrorInfo3.setText(err.value.c_str());
  Page::setVisible(bNext, false);
  Page::setVisible(bBack, false);
}

