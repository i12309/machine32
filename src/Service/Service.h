#pragma once

#include "State/State.h"
#include "Service/HServer.h"

class Service {

public:
    Service(){};

    // Только в этих состояниях работает HTTP сервер и другие сервисы
    static void process(){
        switch(App::state()->type()) {
            case State::Type::BOOT:               Work();   break;
            case State::Type::IDLE:               Work();   break;
            case State::Type::ERROR:              Work();   break;
            case State::Type::SERVICE:            Work();   break;
            case State::Type::NULL_STATE:         Work();   break;
        }
    };

    static void Work(){
        HServer::process();
        MQTTc::process();
    };

};