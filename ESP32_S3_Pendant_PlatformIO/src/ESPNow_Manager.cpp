#include "ESPNow_Manager.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <GlobalVars.h>
#include <vector>
#include <cstring>

ESPNowManager *ESPNowManager::instance = nullptr;

ESPNowManager::ESPNowManager()
{
    instance = this;
    waitingForCmd = "";
    pendingReply = "";
}

ESPNowManager::~ESPNowManager()
{
    if (instance == this)
        instance = nullptr;
}

// ---------- Scansione canale ----------
bool ESPNowManager::scanChannelFromKnownNetworks()
{
    const char *knownNetworks[] = {
        "TISCALI-5311",
        "TISCALI-07EE7E",
        "HUAWEI P30 lite"};
    const int knownCount = 3;

    Serial.println("\n🔍 Scansione WiFi per canale...");
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        Serial.println("❌ Nessuna rete trovata");
        WiFi.scanDelete();
        return false;
    }

    int bestChannel = -1;
    int bestRSSI = -127;
    String bestSSID;

    for (int i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        int channel = WiFi.channel(i);
        int rssi = WiFi.RSSI(i);
        for (int j = 0; j < knownCount; j++)
        {
            if (ssid == knownNetworks[j] && rssi > bestRSSI)
            {
                bestRSSI = rssi;
                bestChannel = channel;
                bestSSID = ssid;
                break;
            }
        }
    }
    WiFi.scanDelete();

    if (bestChannel > 0)
    {
        currentChannel = bestChannel;
        Serial.printf("✅ Canale %d (rete: %s, RSSI: %d)\n", currentChannel, bestSSID.c_str(), bestRSSI);
        return true;
    }
    Serial.println("⚠️ Nessuna rete conosciuta, canale 11");
    currentChannel = 11;
    return false;
}

// ---------- Inizializzazione ----------
bool ESPNowManager::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (!scanChannelFromKnownNetworks())
    { /* fallback */ }

    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("❌ ESP-NOW init failed!");
        return false;
    }

    esp_now_register_send_cb(onSend);
    esp_now_register_recv_cb(onRecv);

    esp_now_peer_info_t broadcastPeer = {};
    memcpy(broadcastPeer.peer_addr, broadcastMac, 6);
    broadcastPeer.channel = currentChannel;
    broadcastPeer.encrypt = false;
    esp_now_add_peer(&broadcastPeer);

    Serial.println("📡 Invio PAIR...");
    esp_now_send(broadcastMac, (const uint8_t *)"PAIR", 4);
    pairStartTime = millis();
    lastDataTime = millis();

    return waitForPairing(10000);
}

bool ESPNowManager::waitForPairing(unsigned long timeout)
{
    unsigned long start = millis();
    while (!paired && millis() - start < timeout)
    {
        delay(10);
        update();
    }
    if (paired)
    {
        Serial.println("✅ Bridge accoppiato!");
        return true;
    }
    Serial.println("❌ Pairing fallito");
    return false;
}

void ESPNowManager::resetPairing()
{
    Serial.println("\n🔄 Reset pairing");

    pairingInProgress = true;
    paired = false;
    bridgePeerAdded = false;

    if (bridgeMac[0] || bridgeMac[1] || bridgeMac[2] ||
        bridgeMac[3] || bridgeMac[4] || bridgeMac[5])
    {
        esp_now_del_peer(bridgeMac);
    }

    memset(bridgeMac, 0, 6);
    sequence = 0;
    lastAckSeq = 0xFFFF;
    rxHead = 0;
    rxTail = 0;
    rxAvailable = false;

    esp_now_send(broadcastMac, (const uint8_t *)"PAIR", 4);
    pairStartTime = millis();
    lastDataTime = millis();
}

// ---------- Invio comandi ----------
bool ESPNowManager::sendCommand(const String &cmd)
{
    if (!paired || !bridgePeerAdded)
    {
        if (errorCallback)
            errorCallback("Not paired");
        return false;
    }

    String command = cmd;
    bool isRealtime = (command.length() == 1 && (command[0] == '!' || command[0] == '~' || command[0] == 0x18));
    if (!isRealtime)
    {
        if (!command.endsWith("\n"))
            command += "\n";
        command.toUpperCase();
    }

    size_t len = command.length();
    if (len > 246)
        len = 246;

    uint8_t packet[250];
    packet[0] = sequence & 0xFF;
    packet[1] = (sequence >> 8) & 0xFF;
    packet[2] = len & 0xFF;
    packet[3] = (len >> 8) & 0xFF;
    memcpy(&packet[4], command.c_str(), len);

    bool ok = sendWithAck(packet, len + 4, sequence);
    if (ok)
        sequence++;
    return ok;
}

bool ESPNowManager::sendGCodeStream(const String &gcode)
{
    if (!paired || !bridgePeerAdded)
    {
        Serial.println("⏳ Attendi pairing...");
        return false;
    }

    std::vector<String> lines;
    String cur = "";
    for (size_t i = 0; i < gcode.length(); i++)
    {
        char c = gcode[i];
        if (c == '\n' || c == '\r')
        {
            if (cur.length() > 0)
                lines.push_back(cur + "\n");
            cur = "";
        }
        else
        {
            cur += c;
        }
    }
    if (cur.length() > 0)
        lines.push_back(cur + "\n");

    Serial.printf("📦 Righe da inviare: %d\n", lines.size());
    int ok = 0;
    for (size_t i = 0; i < lines.size(); i++)
    {
        Serial.printf("➡️ [%d/%d] %s", i + 1, lines.size(), lines[i].c_str());
        if (sendCommand(lines[i]))
            ok++;
        else
            Serial.println(" ❌ Errore invio riga");
    }
    Serial.printf("🎉 COMPLETATO: %d/%d righe OK\n", ok, lines.size());
    return ok == (int)lines.size();
}

bool ESPNowManager::sendRealtime(uint8_t cmd)
{
    if (!paired || !bridgePeerAdded)
        return false;
    uint8_t packet[5];
    packet[0] = sequence & 0xFF;
    packet[1] = (sequence >> 8) & 0xFF;
    packet[2] = 1;
    packet[3] = 0;
    packet[4] = cmd;
    bool ok = sendWithAck(packet, 5, sequence);
    if (ok)
        sequence++;
    return ok;
}

bool ESPNowManager::sendWithAck(const uint8_t *data, size_t len, uint16_t seq)
{
    esp_err_t err = esp_now_send(bridgeMac, data, len);
    if (err != ESP_OK)
        return false;

    unsigned long t0 = millis();
    while (lastAckSeq != seq)
    {
        if (millis() - t0 > 3000)
        {
            Serial.println("⚠️ Timeout ACK");
            return false;
        }
        delay(1);
    }
    return true;
}

// ---------- Ricezione ----------
void ESPNowManager::update()
{
    if (paired && (millis() - lastDataTime) > HEARTBEAT_TIMEOUT_MS)
    {
        Serial.println("\n⚠️ Nessun dato per 3s, reset pairing");
        resetPairing();
    }

    if (!paired)
    {
        if (millis() - pairStartTime > 2000)
        {
            Serial.println("🔁 Ritento PAIR...");
            esp_now_send(broadcastMac, (const uint8_t *)"PAIR", 4);
            pairStartTime = millis();
        }
    }

    if (rxAvailable)
    {
        noInterrupts();
        int idx = rxTail;

        if (idx != rxHead)
        {
            RxPacket pkt = rxQueue[idx];
            rxTail = (idx + 1) % RX_QUEUE_SIZE;
            rxAvailable = (rxTail != rxHead);
            interrupts();

            processReceivedData(pkt.data, pkt.len);
        }
        else
        {
            interrupts();
        }
    }
}

void ESPNowManager::processReceivedData(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        char c = (char)data[i];
        if (c == '\n')
        {
            if (lineBuffer.length() > 0)
            {
                processLine(lineBuffer);
                lineBuffer.clear();
            }
        }
        else if (c != '\r')
        {
            lineBuffer += c;
        }
    }
}

void ESPNowManager::processLine(const std::string& line)
{
    lastDataTime = millis();

    if (waitingForCmd.length() > 0)
    {
        if (line[0] == '<')
            return;
        if (line.find(waitingForCmd) != std::string::npos || line.find('=') != std::string::npos)
        {
            pendingReply = line;
            waitingForCmd.clear();
        }
        return;
    }

    if (line == "ok")
        return;
    if (line.rfind("error:", 0) == 0)
    {
        if (errorCallback)
            errorCallback(String(line.c_str()));
        return;
    }
    if (line.rfind("[MSG:", 0) == 0)
    {
        Serial.println(line.c_str());
        return;
    }
    if (!line.empty() && line[0] == '<')
    {
        if (!gcodeTrackingActive)
        {
            //Serial.printf("📡 AutoReport: %s\n", line.c_str());
        }

        GRBLParser::ParsedStatus parsed = GRBLParser::parseCompleteStatus(line);
        grblCurrentLineNumber = parsed.lineNumber;
        updateFromParsedStatus(parsed);
    }
}

void ESPNowManager::updateFromParsedStatus(const GRBLParser::ParsedStatus &parsed)
{
    MachineState oldState = currentState;
    currentState = parsed.state;
    machinePosition = parsed.machinePosition;

    if (parsed.wcoFound)
    {
        workCoordinateOffset = parsed.workCoordinateOffset;
    }
    workPosition = machinePosition.subtract(workCoordinateOffset);

    spindleSpeed = parsed.spindleSpeed;
    realSpindleSpeed = parsed.realSpindleSpeed;
    spindleDirection = parsed.spindleDirection;
    plannerBuffer = parsed.plannerBuffer;
    serialBuffer = parsed.serialBuffer;

    if (positionCallback)
        positionCallback(workPosition);
    if (stateChangeCallback && oldState != currentState)
        stateChangeCallback(oldState, currentState);
}

String ESPNowManager::getStatus() const
{
    switch (currentState)
    {
    case STATE_IDLE:   return "Idle";
    case STATE_RUN:    return "Run";
    case STATE_HOLD:   return "Hold";
    case STATE_ALARM:  return "Alarm";
    default:           return "Unknown";
    }
}

String ESPNowManager::getFullStatus() const
{
    return String(lastParsedStatus.lastStatusLine.c_str());
}

void ESPNowManager::jog(float x, float y, float z, float feedrate)
{
    String cmd = "$J=G91";
    if (x != 0)
        cmd += " X" + String(x, 3);
    if (y != 0)
        cmd += " Y" + String(y, 3);
    if (z != 0)
        cmd += " Z" + String(z, 3);
    cmd += " F" + String(feedrate, 1);
    sendCommand(cmd);
}

String ESPNowManager::queryReply(const String &cmd, unsigned long timeout)
{
    if (!paired || !bridgePeerAdded)
        return "";
    waitingForCmd = cmd.c_str();
    pendingReply.clear();
    if (!sendCommand(cmd))
    {
        waitingForCmd.clear();
        return "";
    }
    unsigned long start = millis();
    while (!waitingForCmd.empty() && (millis() - start) < timeout)
    {
        update();
        delay(5);
    }
    String reply = pendingReply.c_str();
    pendingReply.clear();
    waitingForCmd.clear();
    return reply;
}

// ---------- Callback statiche ----------
void ESPNowManager::onSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // unused
}

void ESPNowManager::onRecv(const uint8_t *mac, const uint8_t *data, int len)
{
    if (!instance) return;
    instance->lastDataTime = millis();

    // ACK
    if (len == 3 && data[2] == 0x01) {
        uint16_t seq = data[0] | (data[1] << 8);
        instance->lastAckSeq = seq;
        return;
    }

    // PAIR_OK
    if (len == 7 && memcmp(data, "PAIR_OK", 7) == 0) {
        memcpy(instance->bridgeMac, mac, 6);
        esp_now_peer_info_t bridgePeer = {};
        memcpy(bridgePeer.peer_addr, instance->bridgeMac, 6);
        bridgePeer.channel = instance->currentChannel;
        bridgePeer.encrypt = false;
        if (esp_now_add_peer(&bridgePeer) == ESP_OK) {
            instance->bridgePeerAdded = true;
            instance->paired = true;
            Serial.println("\n✅ BRIDGE TROVATO!");
            Serial.print("📡 MAC Bridge: ");
            for (int i = 0; i < 6; i++) {
                Serial.printf("%02X", instance->bridgeMac[i]);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
        }
        return;
    }

    // PING → PONG
    if (len == 4 && memcmp(data, "PING", 4) == 0) {
        esp_now_send(mac, (const uint8_t *)"PONG", 4);
        return;
    }

    // 🔥 RIMOSSO il controllo su g_encoderActive - non più necessario con PCNT

    // Dati normali
    int nextHead = (instance->rxHead + 1) % RX_QUEUE_SIZE;
    if (nextHead != instance->rxTail) {
        RxPacket &pkt = instance->rxQueue[instance->rxHead];
        pkt.len = len;
        if (len > 256) len = 256;
        memcpy(pkt.data, data, len);
        instance->rxHead = nextHead;
        instance->rxAvailable = true;
    } else {
        // Coda piena: scarta il pacchetto più vecchio
        instance->rxTail = (instance->rxTail + 1) % RX_QUEUE_SIZE;
        nextHead = (instance->rxHead + 1) % RX_QUEUE_SIZE;
        if (nextHead != instance->rxTail) {
            RxPacket &pkt = instance->rxQueue[instance->rxHead];
            pkt.len = len;
            if (len > 256) len = 256;
            memcpy(pkt.data, data, len);
            instance->rxHead = nextHead;
            instance->rxAvailable = true;
        }
    }
}