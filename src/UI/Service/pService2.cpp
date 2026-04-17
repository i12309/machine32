#include "pService2.h"
#include "pService.h"
#include "pThrow.h"
#include "pKnife.h"
#include "pBigel.h"

  void pService2::pop_bBack(void* ptr) {
    Log::D(__func__);
    pService::getInstance().show();
  }

  void pService2::pop_bThrow(void* ptr) {
    Log::D(__func__);
    pThrow::getInstance().show();
  }
  void pService2::pop_bBig(void* ptr) {
    Log::D(__func__);
    pBigel::getInstance().show();
  }
  void pService2::pop_bKnife(void* ptr) {
    Log::D(__func__);
    pKnife::getInstance().show();
  }

  void pService2::pop_b1(void* ptr) {
    Log::D(__func__);
        App::state()->setNexTypeState(State::Type::T100B);// для того что бы после CHECK он смог перейти в PROFILING
        App::state()->setFactory(State::Type::CHECK);
  }
  void pService2::pop_b2(void* ptr) {
    Log::D(__func__);
        App::state()->setNexTypeState(State::Type::TDL);// для того что бы после CHECK он смог перейти в PROFILING
        App::state()->setFactory(State::Type::CHECK);
  }
  void pService2::pop_b3(void* ptr) {
    Log::D(__func__);
        App::state()->setNexTypeState(State::Type::TCUT);// для того что бы после CHECK он смог перейти в PROFILING
        App::state()->setFactory(State::Type::CHECK);
  }
