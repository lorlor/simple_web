#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <string>
#include <cstdlib>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>
// #include <opencv2/videostab/videostab.hpp>

#include <ros/ros.h>
#include <std_msgs/String.h>

#include <json.hpp>
#include <curl/curl.h>

extern "C"{
#include <unistd.h>
}

using json = nlohmann::json;

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

class WSServer
{
public:
    WSServer() : m_private_nh("~")
    {
        m_private_nh.getParam("ws_port", m_ws_port);
        m_private_nh.getParam("debug_flag", m_debug_flag);
        m_private_nh.getParam("cam_id", m_cam_id);

        if(m_debug_flag){
            std::cout << "websocke port number: " << m_ws_port << std::endl;
            std::cout << "camera ID: " << m_cam_id << std::endl;
        }
    }

    ~WSServer()
    {
        ros::shutdown();
    }

    void run()
    {
        m_server.set_access_channels(websocketpp::log::alevel::all);
        m_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
        m_server.init_asio();

        m_server.set_message_handler(bind(&WSServer::on_message, this, &m_server, ::_1, ::_2));
        m_server.set_open_handler(bind(&WSServer::on_open, this, &m_server, ::_1));

        m_server.listen(m_ws_port);

        m_server.start_accept();
        m_server.run();
    }

    void on_open(server *s, websocketpp::connection_hdl hdl)
    {
        std::cout << "Already Open WS connection" << std::endl;
        m_hdl = hdl;
    }

    void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg)
    {
        std::cout << "on_message called with hdl: " << hdl.lock().get()
          << " and message: " << msg->get_payload()
          << std::endl;

        // 打开相机
        if(msg->get_payload() == "command:video_start"){
            m_cap.open(m_cam_id);
            if(!m_cap.isOpened()){
                std::cout << "Failed to open camera" << std::endl;
            }
            else{
                std::string video_rdy_json = "{\"command\":\"video_ready\"}";
                s->send(hdl, video_rdy_json, websocketpp::frame::opcode::TEXT);
            }
        }

        // 传输图像
        if(msg->get_payload() == "command:video_tick")
        {
            cv::Mat img;
            m_cap >> img;
            std::vector<uchar> data_encode;
            cv::imencode(".jpg", img, data_encode);
            std::string img_encode_str(data_encode.begin(), data_encode.end());
            std::string img_base64 = base64_encode(reinterpret_cast<const unsigned char*>(img_encode_str.c_str()), img_encode_str.length());
            std::string img_json = "{";
            img_json += "\"command\": \"image\",";
            img_json += "\"payload\":\"" + img_base64 + "\"";
            img_json += "}";

            s->send(m_hdl, img_json, websocketpp::frame::opcode::TEXT);
        }

        // 关闭相机
        if(msg->get_payload() == "command:video_stop"){
            m_cap.release();
        }
    }

    std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
    {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--){
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3){
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; (i <4) ; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if(i){
            for(j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while((i++ < 3))
                ret += '=';

        }

        return ret;
    }

private:
    ros::NodeHandle m_nh;
    ros::NodeHandle m_private_nh;

    int m_debug_flag;
    int m_ws_port;
    int m_cam_id;
    cv::VideoCapture m_cap;

    websocketpp::connection_hdl m_hdl;
    server m_server;
};

int main(int argc, char **argv)
{
    ros::init(argc, argv, "wsserver");

    WSServer server;
    server.run();

    return 0;
}
