
class wifi_connection
{
public:
    struct wifi_settings
    {
        std::string ssid = "TP-LINK_EC830C";
        std::string password = "02947297";
        bool reconnect = true;
        ZAX_JSON_SERIALIZABLE(wifi_settings, JSON_PROPERTY(ssid), JSON_PROPERTY(password), JSON_PROPERTY(reconnect))
        /// {"ssid":"TP-LINK_EC830C", "password":"02947297"}
    };

private:
    wifi_settings ap_settings_;

public:
    wifi_connection();
    int connect(const char* ip);
    void disconnect();
    bool is_connected();
    void set_ap_settings(wifi_settings& ap_settings);
};
