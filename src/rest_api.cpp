#include <inttypes.h>
#include <climits>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <functional>
using namespace std;

#include "ZaxJsonParser.h"
#include "rest_api.h"

class rest_api: public i_rest_api
{
    struct rest_api_basic_message
    {
        string command;
        string parameters;
        ZAX_JSON_SERIALIZABLE(rest_api_basic_message, JSON_PROPERTY(command), JSON_PROPERTY(parameters))
    };

    struct rest_api_basic_message_list
    {
        vector<rest_api_basic_message> requests;
        ZAX_JSON_SERIALIZABLE(rest_api_basic_message_list, JSON_PROPERTY(requests))
    };

    struct rest_api_help_msg
    {
        map<string, string> commands_and_descriptions;
        ZAX_JSON_SERIALIZABLE(rest_api_help_msg, JSON_PROPERTY(commands_and_descriptions))
    };

    map<string, std::function<const char* (const char*, size_t&)>> handlers_;
    rest_api_help_msg help_msg_;
    string help_msg_string_;
    string concatenated_response_;

public:
    rest_api()
    {
        add_handler("help", "Displays this information.", [this](const char* msg, size_t& response_len)
        {
            response_len = help_msg_string_.size();
            return help_msg_string_.c_str();
        });
    }

    virtual void add_handler(const char* command, const char* description, std::function<const char* (const char*, size_t&)> handler) final
    {
        handlers_.insert(make_pair(command, handler));
        if (description)
        {
            help_msg_.commands_and_descriptions.insert(make_pair(command, description));
            help_msg_string_ = help_msg_.zax_to_json();
        }
    }

    const char* handle_api_msg(const char* msg, size_t& response_len) override
    {
        concatenated_response_ = "{ \"responses\":[";
        rest_api_basic_message_list requests = msg;
        for (unsigned int i = 0; i < requests.requests.size(); ++i)
        {
            auto& message = requests.requests[i];
            auto found = handlers_.find(message.command);
            if (i)
                concatenated_response_ += ",";
            if (found != handlers_.end())
                concatenated_response_ += found->second(message.parameters.c_str(), response_len);
            else
            {
                concatenated_response_ += "\"command not supported\"";
                printf("REST API: command not supported: %s \n", msg);
            }
        }
        concatenated_response_ += "]}";
        response_len = concatenated_response_.length();
        return concatenated_response_.c_str();
    }

    virtual ~rest_api() = default;
};

i_rest_api* i_rest_api::new_instance()
{
    return new rest_api();
}

void i_rest_api::delete_instance(i_rest_api* instance)
{
    delete((rest_api*)instance);
}
