#include <curl/curl.h>
#include <CLI/CLI.hpp>
#include <tinyxml2.h>
#include <yaml-cpp/yaml.h>
#include <json-c/json.h>

#include <sstream>
#include <string>

char errorBuffer[CURL_ERROR_SIZE];

enum class BuildStatus
{
    STATUS_UNKNOWN,
    STATUS_SUCCESS,
    STATUS_FAILURE
};

struct Configuration
{
    std::string auth_token;
    std::string build_url;
    std::vector<std::pair<std::string, std::string>> build_str_to_id;

    std::string notif_url;
    std::string notif_token;
    std::string notif_user;

    std::string url_link;
};

int writer(char* data, size_t size, size_t nmemb, std::string* writerData)
{
    if (writerData == nullptr)
    {
        return 0;
    }

    writerData->append(data, size * nmemb);

    return static_cast<int>(size * nmemb);
}

void cleanup_conn(CURL*& conn)
{
    curl_easy_cleanup(conn);
    conn = nullptr;
}

bool curl_init(CURL*& conn, const std::string& url, std::string& buffer, const std::string& auth_token = {})
{
    CURLcode code;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    conn = curl_easy_init();

    if (conn == nullptr)
    {
        std::cerr << "Failed to create CURL connection" << std::endl;;
        exit(EXIT_FAILURE);
    }

    code = curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, errorBuffer);
    if (code != CURLE_OK)
    {
        std::cerr << "Failed to set error buffer [" << code << "]" << std::endl;
        return false;
    }

    code = curl_easy_setopt(conn, CURLOPT_URL, url.c_str());
    if (code != CURLE_OK)
    {
        std::cerr << "Failed to set URL [" << errorBuffer << "]" << std::endl;
        return false;
    }

    code = curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L);
    if (code != CURLE_OK)
    {
        std::cerr << "Failed to set redirect option [" << errorBuffer << "]" << std::endl;
        return false;
    }

    code = curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, writer);
    if (code != CURLE_OK)
    {
        std::cerr << "Failed to set writer [" << errorBuffer << "]" << std::endl;
        return false;
    }

    code = curl_easy_setopt(conn, CURLOPT_WRITEDATA, &buffer);
    if (code != CURLE_OK)
    {
        std::cerr << "Failed to set write data [" << errorBuffer << "]" << std::endl;
        return false;
    }

    if(!auth_token.empty())
    {
        struct curl_slist* chunk = nullptr;
        std::stringstream ss;
        ss << "Authorization: Bearer " << auth_token;
        chunk = curl_slist_append(chunk, ss.str().c_str());

        code = curl_easy_setopt(conn, CURLOPT_HTTPHEADER, chunk);
        if (code != CURLE_OK)
        {
            std::cerr << "Failed to set http error [" << errorBuffer << "]" << std::endl;
            return false;
        }
    }

    return true;
}

bool fetch_data(CURL* conn, const std::string& url, std::string& buffer)
{
    buffer.clear();
    if (curl_easy_perform(conn) != CURLE_OK)
    {
        cleanup_conn(conn);
        std::cerr << "Failed to get '%s' [" << errorBuffer << "]" << std::endl, url.c_str();
        return false;
    }

    return true;
}

bool parse_source(const std::string& source, std::string& buildName, int& buildNumber)
{
    auto marker1 = source.find(';');
    auto marker2 = source.find(',');

    if (marker1 == std::string::npos || marker2 == std::string::npos)
    {
        return false;
    }

    std::string reason = source.substr(0, marker1);
    std::string buildInfo = source.substr(marker2 + 2);

    auto numberIndex = buildInfo.find('#');
    if (numberIndex == std::string::npos)
    {
        return false;
    }

    buildName = source.substr(marker1 + 2, marker2 - (marker1 + 2));

    const char* str = &buildInfo.c_str()[numberIndex + 1];
    buildNumber = atoi(str);

    return true;
}

int cli_config(int argc, char** argv, std::string& source, bool& verbose)
{
    CLI::App app("Command line application for triggering pushover notifications from teamcity");

    app.add_option("-s,--source", source, "Build trigger source");
    app.add_flag("-v,--verbose", verbose, "Whether the output (and notification) should be verbose");

    CLI11_PARSE(app, argc, argv);

    return 0;
}

int yaml_read(Configuration& config)
{
    YAML::Node config_node = YAML::LoadFile("config.yaml");

    config.auth_token = config_node["auth_token"].as<std::string>();
    config.build_url = config_node["build_url"].as<std::string>();

    YAML::Node node = config_node["mappings"];

    for (std::size_t i = 0; i < node.size(); i++) 
    {
        const YAML::Node& entry = node[i];
        std::string name = entry["string"].as<std::string>();
        std::string id = entry["id"].as<std::string>();

        config.build_str_to_id.push_back(std::make_pair(name, id));
    }

    config.notif_url = config_node["notif_url"].as<std::string>();
    config.notif_token = config_node["notif_token"].as<std::string>();
    config.notif_user = config_node["notif_user"].as<std::string>();

    return 0;
}

int find_build_info(const std::string& xml_data, const std::string& build_id, const int build_num, BuildStatus& build_status, std::string& web_url)
{
    build_status = BuildStatus::STATUS_UNKNOWN;

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.Parse(xml_data.c_str());
    if (err != tinyxml2::XML_SUCCESS)
    {
        return err;
    }

    tinyxml2::XMLElement* builds_elem = doc.FirstChildElement("builds");
    if (builds_elem != nullptr)
    {
        for (tinyxml2::XMLElement* build_elem = builds_elem->FirstChildElement("build"); build_elem != nullptr; build_elem = build_elem->NextSiblingElement("build"))
        {
            const tinyxml2::XMLAttribute* build_type_attr = build_elem->FindAttribute("buildTypeId");
            const tinyxml2::XMLAttribute* build_num_attr = build_elem->FindAttribute("number");
            const tinyxml2::XMLAttribute* build_status_attr = build_elem->FindAttribute("status");
            const tinyxml2::XMLAttribute* web_url_attr = build_elem->FindAttribute("webUrl");

            if (build_type_attr == nullptr || build_num_attr == nullptr || build_status_attr == nullptr || web_url_attr == nullptr)
            {
                return tinyxml2::XML_ERROR_PARSING;
            }

            if (strcmp(build_type_attr->Value(), build_id.c_str()) == 0)
            {
                const int num = atoi(build_num_attr->Value());
                if (num == build_num)
                {
                    web_url = web_url_attr->Value();
                    if (strcmp(build_status_attr->Value(), "SUCCESS") == 0)
                    {
                        build_status = BuildStatus::STATUS_SUCCESS;
                    }
                    else if (strcmp(build_status_attr->Value(), "FAILURE") == 0)
                    {
                        build_status = BuildStatus::STATUS_FAILURE;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        return tinyxml2::XML_ERROR_PARSING;
    }

    if (build_status == BuildStatus::STATUS_UNKNOWN)
    {
        return EXIT_FAILURE;
    }

    return 0;
}

int send_notification(const Configuration& config, const BuildStatus& build_status, const std::string& build_name)
{
    std::string  buffer;
    CURL* conn = nullptr;
    if (!curl_init(conn, config.notif_url, buffer))
    {
        std::cerr << "Connection initialization failed" << std::endl;
        return EXIT_FAILURE;
    }

    struct curl_httppost* post = nullptr;
    struct curl_httppost* last = nullptr;
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "token", CURLFORM_COPYCONTENTS, config.notif_token.c_str(), CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "user", CURLFORM_COPYCONTENTS, config.notif_user.c_str(), CURLFORM_END);

    if (build_status == BuildStatus::STATUS_FAILURE)
    {
        std::stringstream ss;
        ss << build_name << " : Build Failed!";
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message", CURLFORM_COPYCONTENTS, ss.str().c_str(), CURLFORM_END);
    }
    else if (build_status == BuildStatus::STATUS_SUCCESS)
    {
        std::stringstream ss;
        ss << build_name << " : Build Succeeded!";
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message", CURLFORM_COPYCONTENTS, ss.str().c_str(), CURLFORM_END);
    }
    else if (build_status == BuildStatus::STATUS_UNKNOWN)
    {
        std::stringstream ss;
        ss << build_name << " : Unknown Build Status";
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message", CURLFORM_COPYCONTENTS, ss.str().c_str(), CURLFORM_END);
    }

    if (!config.url_link.empty())
    {
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "url", CURLFORM_COPYCONTENTS, config.url_link.c_str(), CURLFORM_END);
    }

    CURLcode rc = curl_easy_setopt(conn, CURLOPT_HTTPPOST, post);
    if (rc != CURLE_OK)
    {
        std::cerr << "curl_easy_setopt() failed" << curl_easy_strerror(rc) << std::endl;
        cleanup_conn(conn);
        return EXIT_FAILURE;
    }

    rc = curl_easy_perform(conn);
    curl_formfree(post);
    if (rc != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed" << curl_easy_strerror(rc) << std::endl;
        cleanup_conn(conn);
        return EXIT_FAILURE;
    }

    cleanup_conn(conn);

    struct json_object* jobj = json_tokener_parse(buffer.c_str());
    bool status_ok = false;

    json_object_object_foreach(jobj, key, val)
    {
        if (strcmp(key, "status") == 0)
        {
            auto status = json_object_get_int(val);
            status_ok = status == 1;
            break;
        }
    }

    return status_ok ? 0 : EXIT_FAILURE;
}

int scrub_port_from_url(std::string& web_url)
{
    std::string original_url = web_url;

    const size_t protocolEnd = original_url.find("://");
    web_url = original_url.substr(0, protocolEnd + 3);

    const size_t nextColon = original_url.find(':', protocolEnd + 3);
    if (nextColon == std::string::npos)
    {
        web_url = original_url;
    }
    else
    {
        web_url.append(original_url.substr(protocolEnd + 3, nextColon - (protocolEnd + 3)));
        const size_t firstForwardSlash = original_url.find('/', nextColon);
        web_url.append(original_url.substr(firstForwardSlash, original_url.size() - firstForwardSlash));
    }

    return 0;
}

int main(int argc, char** argv)
{
    std::string source;
    bool verbose = false;
    int rc = cli_config(argc, argv, source, verbose);
    if (rc != 0)
    {
        return EXIT_FAILURE;
    }

    Configuration config;
    rc = yaml_read(config);

    std::string build_name;
    int build_num;
    parse_source(source, build_name, build_num);

    CURL* conn = nullptr;
    std::string buffer;

    if (!curl_init(conn, config.build_url, buffer, config.auth_token))
    {
        std::cerr << "Connection initialization failed" << std::endl;
        return EXIT_FAILURE;
    }

    if (!fetch_data(conn, config.build_url, buffer))
    {
        return EXIT_FAILURE;
    }

    cleanup_conn(conn);

    std::string build_id;
    for (const std::pair<std::string, std::string>& pair : config.build_str_to_id)
    {
        if (std::get<0>(pair) == build_name)
        {
            build_id = std::get<1>(pair);
            break;
        }
    }

    BuildStatus build_status;
    std::string web_url;
    rc = find_build_info(buffer, build_id, build_num, build_status, web_url);
    if (rc == 0 && (verbose || build_status == BuildStatus::STATUS_FAILURE || build_status == BuildStatus::STATUS_UNKNOWN) && scrub_port_from_url(web_url) == 0)
    {
        config.url_link = web_url;
        rc = send_notification(config, build_status, build_name);
    }

    return rc;
}