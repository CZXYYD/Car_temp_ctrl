/*
   wifiduino/ESP8266 通过HTTP方式连接到OneNet物联网平台
   实现对数据的上传和读取
   借鉴实例为上传DHT11采集到的温度和湿度数据，读取平台LIGHT数据流的数值
   By: 学二106宿舍
   2020/11/1
*/

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
//#include <DHT.h>


#define DEBUG 1
#define LED 2 
#define DHTPIN 3                         // 传感器连接到D3 -- Sensor to D3
#define DHTTYPE DHT11                     // DHT 11 

const char ssid[]     = "IOT_test";         // 使用时请修改为当前你的 wifi 名称 -- Please use your own wifi ssid
const char password[] = "qwertyuiop";     //  使用时请修改为当前你的 wifi 密码 -- Please use your own wifi password
const char OneNetServer[] = "api.heclouds.com";
const char APIKEY[] = "nOa=1z1nshTPcj9a8iPTyKBKoLw=";    // 使用时请修改为你的API KEY -- Please use your own API KEY
int32_t DeviceId = 643781453;                             // 使用时请修改为你的设备ID -- Please use your own device ID

const char DS_Temp[] = "TEMP";                        // 数据流 温度TEMP -- Stream "TEMP"
const char DS_Hum[] = "HUMI";                          // 数据流 湿度HUMI -- Stream "HUMI"
const size_t MAX_CONTENT_SIZE = 1024;                  // 最大内容长度 -- Maximum content size
const unsigned long HTTP_TIMEOUT = 2100;                // 超时时间 -- Timeout

float temp;                           //用于存放传感器温度的数值 -- Saving the temperature value of DH11
float humi;                           //用于存放传感器湿度的数值 -- Saving the humidity value of DH11

float ctrl_temp;                      //用于存放设定温度的数值

WiFiClient client;
const int tcpPort = 80;

//DHT dht(DHTPIN, DHTTYPE);

struct UserData 
{
    int errno_val;                // 错误返回值 -- Return error code
    char error[32];               // 错误返回信息 -- Return error information
    int recived_val;             // 接收数据值 -- Recived data 
    char udate_at[32];            // 最后更新时间及日期 -- Last time for update
};

//
//跳过 HTTP 头，使我们在响应正文的开头 -- Skip HTTP headers so that we are at the beginning of the response's body
//
bool skipResponseHeaders() 
{
    char endOfHeaders[] = "\r\n\r\n";
    client.setTimeout(HTTP_TIMEOUT);
    bool ok = client.find(endOfHeaders);
    if (!ok) 
    {
      Serial.println("No response or invalid response!"); //未响应 -- No response
    }
    return ok;
}
//
//从HTTP服务器响应中读取正文 -- Read the body of the response from the HTTP server
//
void readReponseContent(char* content, size_t maxSize) 
{
    //  size_t length = client.peekBytes(content, maxSize);
    size_t length = client.readBytes(content, maxSize);
    delay(20);
    Serial.println(length);
    Serial.println("Get the data from Internet!"); //获取到数据 -- Get the data
    content[length] = 0;
    Serial.println(content);
    Serial.println("Read Over!");
}
//
// 解析数据存储到传入的结构体中 -- Save data to userData struct
//

bool parseUserData_test(char* content, struct UserData* userData) 
{
    // 根据我们需要解析的数据来计算JSON缓冲区最佳大小 -- Compute optimal size of the JSON buffer according to what we need to parse.
    // 如果你使用StaticJsonBuffer时才需要 -- This is only required if you use StaticJsonBuffer.
    const size_t BUFFER_SIZE = 1024;
    // 在堆栈上分配一个临时内存池 -- Allocate a temporary memory pool on the stack
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    // 如果堆栈的内存池太大，使用 DynamicJsonBuffer jsonBuffer 代替
    // --If the memory pool is too big for the stack, use this instead:
    //  --DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(content);
    if (!root.success()) 
    {
      Serial.println("JSON parsing failed!");
      return false;
    }
    // 复制需要的信息到userData结构体中 -- Here were copy the strings we're interested in
    userData->errno_val = root["errno"];
    strcpy(userData->error, root["error"]);
    // 成功返回0 -- Number 0 represents successful 
    if ( userData->errno_val == 0 ) 
    {
      userData->recived_val = root["data"]["datastreams"][0]["datapoints"][0]["value"];
      strcpy(userData->udate_at, root["data"]["datastreams"][0]["datapoints"][0]["at"]);
      Serial.print("Recived Value : ");
      Serial.print(userData->recived_val);
      Serial.print("\t The last update time : ");
      Serial.println(userData->udate_at);
    }
    Serial.print("errno : ");
    Serial.print(userData->errno_val);
    Serial.print("\t error : ");
    Serial.println(userData->error);
  
    return true;
}


//
// 读取数据 -- Read data
//
int readData(int dId, char dataStream[])
{
    // 创建发送请求的URL -- We now create a URI for the request
    String url = "/devices/";
    url += String(dId);
    url += "/datapoints?datastream_id=";
    url += dataStream;

    // 创建发送指令 -- We now combine the request to the server
    String send_data = String("GET ") + url + " HTTP/1.1\r\n" +
                     "api-key:" + APIKEY + "\r\n" +
                     "Host:" + OneNetServer + "\r\n" +
                     "Connection: close\r\n\r\n";
    // 发送指令 -- This will send the request to server
    client.print(send_data);
    // 调试模式串口打印发送的指令 -- The request will be printed if we choose the DEBUG mode
    if (DEBUG)
    {
      Serial.println(send_data);
    }
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
      if (millis() - timeout > 2000) 
      {
        Serial.println(">>> Client Timeout !");
        client.stop();
        break;
      }      
    }

    if (skipResponseHeaders())  
    { 
      char response[MAX_CONTENT_SIZE];
      // 从服务器读取信息后解析 -- We now parse the information after we recived the data
      readReponseContent(response, sizeof(response));
      UserData userData_LEDstatus;
      if (parseUserData_test(response, &userData_LEDstatus)) 
      {
        Serial.println("Data parse OK!");
        return userData_LEDstatus.recived_val;
      }
     }
 }
//
// 上传温传数据 -- Post data
//
void postData(int dId, float val_t, float val_h) 
{
    // 创建发送请求的URL -- We now create a URI for the request
    String url = "/devices/";
    url += String(dId);
    url += "/datapoints?type=3";           
    String data = "{\"" + String(DS_Temp) + "\":" + String(val_t) + ",\"" +
                  String(DS_Hum) + "\":" + String(val_h) + "}";
    // 创建发送指令 -- We now combine the request to the server
    String post_data = "POST " + url + " HTTP/1.1\r\n" +
                       "api-key:" + APIKEY + "\r\n" +
                       "Host:" + OneNetServer + "\r\n" +
                       "Content-Length: " + String(data.length()) + "\r\n" +                     //发送数据长度
                       "Connection: close\r\n\r\n" +
                       data;
  
    // 发送指令 -- This will send the request to server
    client.print(post_data);
    // 调试模式串口打印发送的指令 -- The request will be printed if we choose the DEBUG mode
    if (DEBUG)
    {
        Serial.println(post_data);  
    }
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
      if (millis() - timeout > 2000) 
      {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
}


void setup() 
{
    WiFi.mode(WIFI_AP_STA);               //设置工作模式 -- set work mode:  WIFI_AP /WIFI_STA /WIFI_AP_STA
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    delay(10);

    Serial.println("");
    Serial.print("Trying to connect to ");
    Serial.println(ssid);
    // 连接到wifi -- We start by connecting to a wifi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // 传感器打开 -- We start the DHT11
//    dht.begin();
}

void loop() 
{
    delay(2000);
    // 默认摄氏度 -- Read temperature as Celsius (the default)
    temp = 29; //dht.readTemperature(); 
    humi = 80; //dht.readHumidity();
    // 检查传感器数据是否正确 -- Check if any reads failed and exit early (to try again).
    if (isnan(temp) || isnan(humi)) 
    {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    //建立连接并判断 -- Connecting to server
    if (!client.connect(OneNetServer, tcpPort)) 
    {
      Serial.println("connection failed");
      return;
    }
    //上传数据 -- post value
    postData(DeviceId, temp, humi);
    Serial.println("closing connection");

    //控制数据刷新时间间隔 1000=1s
    delay(10000);
    
    //建立连接并判断 -- Connecting to server
    if (!client.connect(OneNetServer, tcpPort)) 
    {
      Serial.println("connection failed");
      return;
    }
    //从云端获取值并存于ctrl_temp -- get data from server
    ctrl_temp=readData(DeviceId, "ctrl_temp");
//    analogWrite(LED, stream1);
    Serial.println("closing connection");
}
