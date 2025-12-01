#include <windows.h>
#include <string>
#include <thread>
#include "scs_sdk_1_12/include/scssdk_telemetry.h"
#include "scs_sdk_1_12/include/scssdk_telemetry_common_channels.h"
#include "scs_sdk_1_12/include/scssdk_telemetry_truck_common_channels.h"

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>

SOCKET clientSocket = INVALID_SOCKET;
SOCKET serverSocket = INVALID_SOCKET;

void startServer() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(25555);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    listen(serverSocket, 1);

    clientSocket = accept(serverSocket, nullptr, nullptr);
}

void sendData(const std::string& text) {
    if (clientSocket != INVALID_SOCKET) {
        send(clientSocket, text.c_str(), (int)text.size(), 0);
    }
}

float speed = 0;
float fuel  = 0;
float posX = 0;
float posY = 0;
float posZ = 0;
std::string truck_model;
std::string truck_brand;
std::string truck_name;

scs_result onTelemetry(const scs_event_t event, const void* eventInfo, const scs_context_t context) {
    const scs_telemetry_data_t* data = (const scs_telemetry_data_t*)eventInfo;

    if (!data || !data->attributes) return SCS_RESULT_ok;

    for (int i = 0; i < data->count; i++) {
        const auto& attr = data->attributes[i];
        if (!attr.value) continue;

        switch (attr.id) {
        case SCS_TELEMETRY_TRUCK_CHANNEL_speed:
            speed = *(const float*)attr.value;
            break;
        case SCS_TELEMETRY_TRUCK_CHANNEL_fuel:
            fuel = *(const float*)attr.value;
            break;
        case SCS_TELEMETRY_TRUCK_CHANNEL_local_position:
        {
            const float* p = (const float*)attr.value;
            posX = p[0];
            posY = p[1];
            posZ = p[2];
        }
        break;
        #ifdef SCS_TELEMETRY_TRUCK_CHANNEL_model
        case SCS_TELEMETRY_TRUCK_CHANNEL_model:
            if (attr.value) truck_model = std::string((const char*)attr.value);
            break;
        #endif
        #ifdef SCS_TELEMETRY_TRUCK_CHANNEL_brand
        case SCS_TELEMETRY_TRUCK_CHANNEL_brand:
            if (attr.value) truck_brand = std::string((const char*)attr.value);
            break;
        #endif
        #ifdef SCS_TELEMETRY_TRUCK_CHANNEL_name
        case SCS_TELEMETRY_TRUCK_CHANNEL_name:
            if (attr.value) truck_name = std::string((const char*)attr.value);
            break;
        #endif
        }
    }

    // TODO
    // 所有数据待整理
    std::string json = "{"
        "\"speed\":" + std::to_string(speed) + ","
        "\"fuel\":" + std::to_string(fuel) + ","
        "\"x\":" + std::to_string(posX) + ","
        "\"y\":" + std::to_string(posY) + ","
        "\"z\":" + std::to_string(posZ) +
        ",\"truck_model\":\"" + truck_model + "\"," \
        "\"truck_brand\":\"" + truck_brand + "\"," \
        "\"truck_name\":\"" + truck_name + "\"" \
        "}";

    sendData(json + "\n");

    return SCS_RESULT_ok;
}

extern "C" __declspec(dllexport) scs_result scs_telemetry_init(
    const scs_telemetry_init_t* initInfo,
    scs_context_t context)
{
    std::thread(startServer).detach();
    initInfo->register_for_event(SCS_TELEMETRY_EVENT_frame_end, onTelemetry, nullptr);
    return SCS_RESULT_ok;
}

extern "C" __declspec(dllexport) void scs_telemetry_shutdown(scs_context_t context) {
    closesocket(clientSocket);
    closesocket(serverSocket);
}
