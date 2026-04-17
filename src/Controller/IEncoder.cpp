#include "IEncoder.h"

#include "Service/Log.h"

IEncoder::IEncoder(int pinA, int pinB, int threshold, MCP* mcp, bool enabled, int pcntUnit)
    : _pinA(pinA)
    , _pinB(pinB)
    , _threshold(threshold)
    , _mcp(mcp)
    , _enabled(enabled)
    , _pcntUnit(kDefaultPcntUnit)
    , _encoder(nullptr)
    , _lastPosition(0)
    , _capturedCount(0)
{
    if (!_enabled) return;

    if (pcntUnit >= 0 && pcntUnit < MAX_ESP32_ENCODERS) {
        _pcntUnit = pcntUnit;
    } else {
        Log::L("[IEncoder] invalid PCNT unit=%d, fallback to %d", pcntUnit, kDefaultPcntUnit);
        _pcntUnit = kDefaultPcntUnit;
    }

    // ESP32Encoder работает только с локальными GPIO (не MCP23017).
    if (_mcp != nullptr) {
        _enabled = false;
        Log::L("[IEncoder] disabled: MCP-based encoder is not supported by ESP32 PCNT");
        return;
    }

    _encoder = new ESP32Encoder();
    if (_encoder == nullptr) {
        _enabled = false;
        Log::L("[IEncoder] failed to allocate ESP32Encoder");
        return;
    }

    // Сохраняем поведение INPUT_PULLUP как в старой реализации.
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    attachFullQuad(_pcntUnit);
    _lastPosition = getCount();
    resumeCount(); 
}

IEncoder::~IEncoder() {
    if (_encoder == nullptr) return;
    if (_encoder->isAttached()) _encoder->detach();
    delete _encoder;
    _encoder = nullptr;
}

bool IEncoder::ready() const {
    return _enabled && _encoder != nullptr;
}

void IEncoder::attachHalfQuad(int pcnt_unit) {
    if (!ready()) return;
    if (_encoder->isAttached()) _encoder->detach();
    _encoder->attachHalfQuad(_pinA, _pinB, pcnt_unit);
    _pcntUnit = pcnt_unit;
}

void IEncoder::attachFullQuad(int pcnt_unit) {
    if (!ready()) return;
    if (_encoder->isAttached()) _encoder->detach();
    _encoder->attachFullQuad(_pinA, _pinB, pcnt_unit);
    _pcntUnit = pcnt_unit;
}

void IEncoder::attachSingleEdge(int pcnt_unit) {
    if (!ready()) return;
    if (_encoder->isAttached()) _encoder->detach();
    _encoder->attachSingleEdge(_pinA, _pinB, pcnt_unit);
    _pcntUnit = pcnt_unit;
}

int64_t IEncoder::getCount() {
    if (!ready() || !_encoder->isAttached()) return _lastPosition;
    _lastPosition = _encoder->getCount() * -1; // инвертируем потому что энкодер стоит так что относительно мотора вращается в противоположную сторону
    return _lastPosition;
}

int64_t IEncoder::clearCount() {
    if (!ready() || !_encoder->isAttached()) {
        _lastPosition = 0;
        return 0;
    }
    const int64_t result = _encoder->clearCount();
    _lastPosition = _encoder->getCount();
    return result;
}

int64_t IEncoder::pauseCount() {
    if (!ready() || !_encoder->isAttached()) return 0;
    return _encoder->pauseCount();
}

int64_t IEncoder::resumeCount() {
    if (!ready() || !_encoder->isAttached()) return 0;
    return _encoder->resumeCount();
}

void IRAM_ATTR IEncoder::setCapture(int64_t value) {
    _capturedCount = value;
}

int64_t IEncoder::getCapture() const {
    return _capturedCount;
}

void IEncoder::resetCapture() {
    _capturedCount = 0;
}

void IEncoder::detach() {
    if (!ready() || !_encoder->isAttached()) return;
    _encoder->detach();
}

bool IEncoder::isAttached() const {
    if (!ready()) return false;
    return _encoder->isAttached();
}

void IEncoder::setCount(int64_t value) {
    if (!ready() || !_encoder->isAttached()) {
        _lastPosition = value;
        return;
    }
    _encoder->setCount(value);
    _lastPosition = value;
}

void IEncoder::setFilter(uint16_t value) {
    if (!ready() || !_encoder->isAttached()) return;
    _encoder->setFilter(value);
}

void IEncoder::setUseInternalWeakPullResistors(puType type) {
    ESP32Encoder::useInternalWeakPullResistors = type;
}

puType IEncoder::getUseInternalWeakPullResistors() {
    return ESP32Encoder::useInternalWeakPullResistors;
}

void IEncoder::setIsrServiceCpuCore(uint32_t core) {
    ESP32Encoder::isrServiceCpuCore = core;
}

uint32_t IEncoder::getIsrServiceCpuCore() {
    return ESP32Encoder::isrServiceCpuCore;
}
