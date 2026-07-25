#ifndef ESP_NOW_MANAGER_H
#define ESP_NOW_MANAGER_H

#include <Arduino.h>
#include <functional>
#include <esp_now.h>
#include <esp_wifi.h>
#include "GRBLParser.h"

// ========== DICHIARAZIONI VARIABILI GLOBALI USATE ==========
extern bool brakeActive;
extern bool gcodeTrackingActive;
extern int grblCurrentLineNumber;

class ESPNowManager {
public:
    using PositionCallback = std::function<void(const Position&)>;
    using StateChangeCallback = std::function<void(MachineState oldState, MachineState newState)>;
    using ErrorCallback = std::function<void(const String& error)>;

    ESPNowManager();
    ~ESPNowManager();

    bool begin();
    bool isReady() const { return paired && bridgePeerAdded; }

    bool sendCommand(const String& cmd);
    bool sendGCodeStream(const String &gcode);
    bool sendCommandAsync(const String& cmd) { return sendCommand(cmd); }
    bool sendRealtime(uint8_t cmd);

    void update();

    // Getters
    MachineState getMachineState() const { return currentState; }
    Position getWorkPosition() const { return workPosition; }
    float getWorkPositionX() const { return workPosition.x; }
    float getWorkPositionY() const { return workPosition.y; }
    float getWorkPositionZ() const { return workPosition.z; }
    float getWorkPositionA() const { return workPosition.a; }
    Position getMachinePosition() const { return machinePosition; }
    Position getWorkCoordinateOffset() const { return workCoordinateOffset; }
    float getSpindleSpeed() const { return spindleSpeed; }
    float getRealSpindleSpeed() const { return realSpindleSpeed; }
    String getSpindleDirection() const { return spindleDirection.c_str(); }
    int getPlannerBuffer() const { return plannerBuffer; }
    int getSerialBuffer() const { return serialBuffer; }
    String getStatus() const;

    // Jog
    void jog(float x, float y, float z, float feedrate);
    void stopJog() { sendRealtime(0x85); }

    // Utility query
    String queryReply(const String& cmd, unsigned long timeout = 3000);

    // Callback registration
    void onPosition(PositionCallback cb) { positionCallback = cb; }
    void onStateChange(StateChangeCallback cb) { stateChangeCallback = cb; }
    void onError(ErrorCallback cb) { errorCallback = cb; }

    // Metodi aggiunti per compatibilità
    void resetController() { sendRealtime(0x18); }
    void getBufferState(int& planner, int& serial) const { planner = plannerBuffer; serial = serialBuffer; }
    String getFullStatus() const;
    float getLastSpindleSpeed() const { return spindleSpeed; }
    const GRBLParser::ParsedStatus& getParsedStatus() const { return lastParsedStatus; }

    // Compatibilità legacy
    void processFullDuplexEvents() {}
    void updatePosition() { update(); }
    bool isConnected() const { return isReady(); }

private:
    uint8_t broadcastMac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    uint8_t bridgeMac[6] = {0};
    bool paired = false;
    bool bridgePeerAdded = false;
    bool pairingInProgress = false;
    uint16_t sequence = 0;
    volatile uint16_t lastAckSeq = 0xFFFF;
    unsigned long lastDataTime = 0;
    unsigned long pairStartTime = 0;
    int currentChannel = 11;

    static constexpr int RX_QUEUE_SIZE = 64;
    static constexpr unsigned long HEARTBEAT_TIMEOUT_MS = 10000;
    struct RxPacket {
        uint8_t data[256];
        int len;
    };
    RxPacket rxQueue[RX_QUEUE_SIZE];
    volatile int rxHead = 0, rxTail = 0;
    volatile bool rxAvailable = false;

    std::string lineBuffer;
    std::string waitingForCmd;
    std::string pendingReply;

    MachineState currentState = STATE_UNKNOWN;
    Position machinePosition;
    Position workPosition;
    Position workCoordinateOffset;
    float spindleSpeed = 0, realSpindleSpeed = 0;
    std::string spindleDirection;
    int plannerBuffer = 0, serialBuffer = 0;

    GRBLParser::ParsedStatus lastParsedStatus;

    PositionCallback positionCallback = nullptr;
    StateChangeCallback stateChangeCallback = nullptr;
    ErrorCallback errorCallback = nullptr;

    bool scanChannelFromKnownNetworks();
    bool waitForPairing(unsigned long timeout);
    bool sendWithAck(const uint8_t* data, size_t len, uint16_t seq);
    void processReceivedData(const uint8_t* data, int len);
    void processLine(const std::string& line);
    void updateFromParsedStatus(const GRBLParser::ParsedStatus& parsed);
    void resetPairing();

    static void onSend(const uint8_t* mac_addr, esp_now_send_status_t status);
    static void onRecv(const uint8_t* mac, const uint8_t* data, int len);
    static ESPNowManager* instance;
};

#endif