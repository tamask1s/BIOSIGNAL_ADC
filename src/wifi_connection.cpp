#include <cstring>
#include <string>
#include <cstdio>
#include <stdlib.h>
#include <map>
#include <vector>
#include "ZaxJsonParser.h"
#include "wifi_connection.h"
#include "print.h"

#if defined(ARDUINO)

#include <WiFi.h>

IPAddress gateway;
IPAddress subnet;
IPAddress primaryDNS; /** optional */
IPAddress secondaryDNS; /** optional */

wifi_connection::wifi_connection()
{
    gateway = IPAddress(192, 168, 1, 1);
    subnet = IPAddress(255, 255, 0, 0);
    primaryDNS = IPAddress(8, 8, 8, 8);
    secondaryDNS = IPAddress(8, 8, 4, 4);
}

int wifi_connection::connect(const char* ip)
{
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    WiFi.softAP("ecg_ap");
    println(WiFi.softAPIP());
    delay(100);
    // Configures static IP address
    IPAddress local_IP;
    local_IP.fromString(ip);
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
        println("WiFi Failed to configure.");

    print("Connecting to WiFi ");
    println(ap_settings_.ssid.c_str());
    WiFi.begin(ap_settings_.ssid.c_str(), ap_settings_.password.c_str());
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        print(".");
        if (++counter > 20)
            break;
    }
    println("");
    if (WiFi.status() == WL_CONNECTED)
    {
        print("WiFi connected. IP address:");
        println(WiFi.localIP());
        return 0;
    }
    else
    {
        println("WiFi connection failed.");
        return 1;
    }
}

void wifi_connection::disconnect()
{
    WiFi.disconnect();
    int counter = 0;
    while (WiFi.status() == WL_CONNECTED)
    {
        delay(100);
        print(".");
        if (++counter > 20)
            break;
    }
    if (WiFi.status() == WL_CONNECTED)
        println("WiFi disconnection failed. Still connected.");
    else
        println("WiFi disconnected.");
    delay(300);
    ///WiFi.end();
    WiFi.mode(WIFI_OFF);
}

void wifi_connection::set_ap_settings(wifi_settings& ap_settings)
{
    ap_settings_ = ap_settings;
}

bool wifi_connection::is_connected()
{
    return WiFi.status() == WL_CONNECTED;
}

#else /// ARDUINO

wifi_connection::wifi_connection()
{}

int wifi_connection::connect(const char* ip)
{
    return 0;
}

void wifi_connection::set_ap_settings(wifi_settings& ap_settings)
{
    ap_settings_ = ap_settings;
}

bool wifi_connection::is_connected()
{
    return true;
}

void wifi_connection::disconnect()
{}

#endif /// ARDUINO
